#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <values.h>
#include <math.h>
#include <assert.h>

#include "common/geom.h"
#include "test_macros.h"
#include "line_test.h"



////////////////////////////////////////////////////////////////////////////////
// ------------------------------- Up & Down -------------------------------- //
////////////////////////////////////////////////////////////////////////////////

// The line test's context (used by most functions here).
static line_test_context_t context;

void line_test_init() { bzero(&context, sizeof(line_test_context_t)); }

void line_test_deinit() { free(context.result); }

// Resets the context for testing.
static inline void _reset(const paleo_stroke_t* stroke) {
  context.stroke = stroke;
}



////////////////////////////////////////////////////////////////////////////////
// ----------------------------- The line tests ----------------------------- //
////////////////////////////////////////////////////////////////////////////////

// Does the line segment test on the ranges provided.
//   first_i: Index (incl.) of the first point to use.
//   last_i: Index (excl.) of the last point to use.
static inline void _line_seg_test(int first_i, int last_i);

const line_test_result_t* line_seg_test(const paleo_stroke_t* stroke) {
  _reset(stroke);
  context.result = realloc(context.result, sizeof(line_test_result_t));
  if (stroke->num_crnrs == 2 || stroke->num_crnrs == 3) {
    _line_seg_test(0, stroke->num_pts);
  }
  return context.result;
}

const line_test_result_t* poly_line_test(const paleo_stroke_t* stroke) {
  _reset(stroke);

  // Check DCR value.
  if (stroke->dcr < PALEO_THRESH_J) {
    SET_FAIL_ARR(0, "Stroke DCR val too low: %.2f < %.2f", stroke->dcr, PALEO_THRESH_J);
    return context.result;
  }

  // Do the line test for each sub-line.
  context.result = realloc(context.result,
      stroke->num_crnrs * sizeof(line_test_result_t));
  double avg_lse = 0;   // also compute average LSE
  for (int i = 1; i < stroke->num_crnrs; i++) {
    _line_seg_test(stroke->crnrs[i-1]->i, stroke->crnrs[i]->i);
    if (!context.result->possible) {  // Each sub-seg must pass.
      SET_FAIL_ARR(0, "Does not pass line test in sub-seg %d", i);
      return context.result;
    }

    memcpy(&context.result[i], context.result, sizeof(line_test_result_t));
    avg_lse += context.result->lse;
  }

  // Compute avg LSE and verify it's okay.
  bzero(context.result, sizeof(line_test_result_t));
  context.result->lse = avg_lse / stroke->num_crnrs;
  if (context.result->lse >= PALEO_THRESH_I) {
    SET_FAIL_ARR(0, "Avg LSE too high: %.2f >= %.2f",
        context.result->lse, PALEO_THRESH_I);
    return context.result;
  }

  // Everything checks out.  Create the line and return it.
  context.result->possible = 1;
  context.result->line.num = stroke->num_crnrs;
  context.result->line.pts = calloc(
      context.result->line.num, sizeof(point2d_t));
  for (int i = 0; i < stroke->num_crnrs; i++) {
    memcpy(&context.result->line.pts[i], stroke->crnrs[i], sizeof(point2d_t));
  }
  return context.result;
}

// Creates the best fit line segment between the two point indexes and stores it
// in the context.
static inline void _best_fit_line_seg(int first_i, int last_i);

// Computes the projection of p to the ideal line.
//   proj: Return value (projection)
//   p: The point to project.
static inline void _projection_to_ideal(point2d_t* proj, const point2d_t* p);

// Computes the orthogonal distance from the point to the ideal line.
static inline double _distance_to_ideal(const point2d_t* p);

static inline void  _line_seg_test(int first_i, int last_i) {
  // Reset 0th 
  bzero(context.result, sizeof(line_test_result_t));
  context.result->possible = 1;

  _best_fit_line_seg(first_i, last_i);

  double px_len = 0;  // Length of this sub-stroke.
  double od2 = 0;     // Orthogonal distance squared.
  for (int i = first_i; i < last_i; i++) {
    double d = _distance_to_ideal(&context.stroke->pts[i].p2d);
    od2 += d * d;
    if (i > first_i) {
      px_len += point2d_distance(
          &context.stroke->pts[i-1].p2d, &context.stroke->pts[i].p2d);
    }
  }
  context.result->lse = od2 / px_len;
  if (context.result->lse >= PALEO_THRESH_G) {
    SET_FAIL_RTN_ARR(0, "Line LSE too large: %.2f >= %.2f",
        context.result->lse, PALEO_THRESH_G);
  }

  context.result->fa = 0;
  for (int i = first_i + 1; i < last_i; i++) {
    // Compute the projections to the line.
    point2d_t proj_a, proj_b;
    _projection_to_ideal(&proj_a, &context.stroke->pts[i-1].p2d);
    _projection_to_ideal(&proj_b, &context.stroke->pts[i].p2d);

    // Add quad area to feature area (order is important!).
    context.result->fa += geom_quad_area(&proj_b, &proj_a,
        &context.stroke->pts[i-1].p2d, &context.stroke->pts[i].p2d);
  }

  if (context.result->fa / px_len >= PALEO_THRESH_H) {
    SET_FAIL_RTN_ARR(0, "FA too large: %.2f / %.2f = %.2f >= %.2f", context.result->fa,
        px_len, context.result->fa / px_len, PALEO_THRESH_H);
  }

  // Everything checks out.  Create the line and return.
  context.result->line.num = 2;
  context.result->line.pts = calloc(2, sizeof(point2d_t));
  memcpy(&context.result->line.pts[0],
      &context.stroke->pts[first_i], sizeof(point2d_t));
  memcpy(&context.result->line.pts[1],
      &context.stroke->pts[last_i-1], sizeof(point2d_t));
}

static inline void _best_fit_line_seg(int first_i, int last_i) {
  assert(0 <= first_i && first_i < last_i && last_i <= context.stroke->num_pts);

  // Used the algorithm found here:
  //   http://faculty.cs.niu.edu/~hutchins/csci230/best-fit.htm
  long sum_x = 0, sum_y = 0, sum_x2 = 0, sum_xy = 0;
  for (int i = first_i; i < last_i; i++) {
    sum_x += context.stroke->pts[i].x;
    sum_y += context.stroke->pts[i].y;
    sum_x2 += context.stroke->pts[i].x * context.stroke->pts[i].x;
    sum_xy += context.stroke->pts[i].x * context.stroke->pts[i].y;
  }

  const int num = last_i - first_i;
  double x_mean = sum_x / num;
  double y_mean = sum_y / num;

  // The equivalent of the return value -- assigning values to the ideal line.
  double denom = (sum_x2 - sum_x * x_mean);
  if (abs(denom) > 0.0000001) {  // Some number very close to 0.
    context.ideal_line.slope = (sum_xy - sum_x * y_mean) / denom;
    context.ideal_line.y_int = y_mean - context.ideal_line.slope * x_mean;
    context.ideal_line.p0.x = 0;
    context.ideal_line.p0.y = context.ideal_line.y_int;
  } else {  // Avoid div-by-0.
    context.ideal_line.slope = DBL_MAX;
    context.ideal_line.y_int = DBL_MAX;

    // X coordinate of this line should be average of all the points in the
    // line.
    context.ideal_line.p0.x = 0;
    context.ideal_line.p0.y = LONG_MAX;
    for (int i = first_i; i < last_i; i++) {
      context.ideal_line.p0.x += context.stroke->pts[i].x;
    }
    context.ideal_line.p0.x /= num;
  }

  context.ideal_line.theta = atan2(
    context.stroke->pts[last_i].y - context.stroke->pts[first_i].y,
    context.stroke->pts[last_i].x - context.stroke->pts[first_i].x);
}

static inline double _distance_to_ideal(const point2d_t* p) {
  point2d_t proj;   // projection return value.
  _projection_to_ideal(&proj, p);
  return point2d_distance(&proj, p);
}

static inline void _projection_to_ideal(point2d_t* proj, const point2d_t* p) {
  assert(context.ideal_line.slope != 0 || context.ideal_line.y_int != 0);

  // Avoid div-by-0.
  if (context.ideal_line.slope >= DBL_MAX) {
    proj->x = context.ideal_line.p0.x;
    proj->y = p->y;
  } else {
    const double m_x = 1;
    const double m_y = context.ideal_line.slope;

    // This is the dividend of two dot products:
    //    v . s / s . s
    // where v is the vector from p0 to p, and u is the vector defined by the
    // slope: i.e., <1,m>
#define S (context.ideal_line.slope)
#define P (context.ideal_line.p0)
    double dist = ((p->x - P.x) + (p->y - P.y) * S) / (1 + S * S);
    proj->x = P.x + dist;
    proj->y = P.y + dist * S;
#undef P
#undef S
  }
}
