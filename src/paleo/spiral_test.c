#include <config.h>
#include <values.h>
#include <math.h>

#include "common/util.h"
#include "thresh.h"
#include "test_macros.h"
#include "spiral_test.h"


static paleo_spiral_test_context_t context;



void paleo_spiral_test_init() {
  bzero(&context, sizeof(paleo_spiral_test_context_t));
}

void paleo_spiral_test_deinit() { }

void _reset(const paleo_stroke_t* stroke) {
  bzero(&context, sizeof(paleo_spiral_test_context_t));
  context.stroke = stroke;
}



const paleo_spiral_test_result_t*
paleo_spiral_test(const paleo_stroke_t* stroke) {
  CHECK_RTN_RESULT(stroke->overtraced, "Stroke not overtraced.");
  CHECK_RTN_RESULT(stroke->ndde > PALEO_THRESH_K,
      "NDDE too low: %.2f <= K (%.2f)", stroke->ndde, PALEO_THRESH_K);

  _reset(stroke);

  // Calculate center (center of bbox).
  point2d_t max = { MINLONG, MINLONG }, min = { MAXLONG, MAXLONG };
  for (int i = 0; i < stroke->num_pts; i++) {
    point2d_t* p = &stroke->pts[i].p2d;
    if (p->x < min.x) { min.x = p->x; }
    if (p->y < min.y) { min.y = p->y; }
    if (p->x > max.x) { max.x = p->x; }
    if (p->y > max.y) { max.y = p->y; }
  }
  context.ideal.center.x = (max.x + min.x) / 2;
  context.ideal.center.x = (max.y + min.y) / 2;

  // Calculate radius (same as for circle);
  context.ideal.r = 0;
  for (int i = 0; i < stroke->num_pts; i++) {
    context.ideal.r += point2d_distance(
        &context.ideal.center, &stroke->pts[i].p2d);
  }
  context.ideal.r /= stroke->num_pts;

  // Ensure the bbox radius is 
  const double bbox_rad = (max.x - min.x + max.y - min.y) / 4;
  CHECK_RTN_RESULT(context.ideal.r / bbox_rad < PALEO_THRESH_S,
      "avg (%.2f) / bbox r (%.2f) >= S (%.2f)",
      context.ideal.r, bbox_rad, PALEO_THRESH_S);

  // Break stroke up into 2pi increments.
  const int NP = stroke->num_pts;   // convenience: number of points
  const int NI = floor(             // number of 2pi increments.
      (stroke->pts[NP-1].dir - stroke->pts[0].dir) / 2 * M_PIl);
  int* incs = calloc(NI+1, sizeof(int));
  incs[0] = 0;
  double next_angle = stroke->pts[0].dir + 2 * M_PIl;
  int next_inc = 1;
  for (int i = 1; i < NP; i++) {
    if (stroke->pts[i].dir >= next_angle) {
      next_angle += 2 * M_PIl;
      incs[next_inc++] = i;
    }
  }

  // Sanity check that all incs were assigned, but not too many.
  assert(next_inc == NI+1);

  // Compute radii and centers of the sub-strokes.
  double* radii = calloc(NI, sizeof(double));
  point2d_t* centers = calloc(NI, sizeof(point2d_t));
  for (int i = 0; i < NI; i++) {
    radii[i] = 0;
    bzero(&centers[i], sizeof(point2d_t));
    const int first = stroke->pts[incs[i]].i;
    const int last = stroke->pts[incs[i+1]].i;
    for (int j = first; j < last; j++) {
      const point2d_t* p = &stroke->pts[j].p2d;
      radii[i] += point2d_distance(&context.ideal.center, p);
      centers[i].x += p->x;
      centers[i].y += p->y;
    }
    radii[i] /= first - last;
    centers[i].x /= first - last;
    centers[i].y /= first - last;
  }

  // Ensure radii are either all completely ascending or descending.
  for (int i = 2; i < NI; i++) {
    double diff[] = { radii[i-1] - radii[i-2], radii[i] - radii[i-1] };
    CHECK_RTN_RESULT(diff[0] / abs(diff[0]) == diff[1] / abs(diff[1]),
        "Change in radius trend at %d: %.2f -> %.2f -> %2.f",
        i, radii[i-2], radii[i-1], radii[i]);
  }

  // Find sum of distances between centers and ensure a strange quotient (see
  // paper) is less than a threshold.
  double sum = 0;
  for (int i = 1; i < NI; i++) {
    sum += point2d_distance(&centers[i-1], &centers[i]);
  }
  CHECK_RTN_RESULT(sum / (context.ideal.r * NI) < PALEO_THRESH_T,
      "%.2f / (%.2f * %d) >= %.2f", sum, context.ideal.r, NI, PALEO_THRESH_T);

  // Do stupid O(n^2) to compare all centers' distances from one another.
  int max_i = 0, max_j = 1;
  double max_dist = point2d_distance(&centers[max_i], &centers[max_j]);
  for (int i = 0; i < NI; i++) {
    for (int j = i+1; j < NI; j++) {
      double dist = point2d_distance(&centers[i], &centers[j]);
      if (dist > max_dist) {
        max_i = i;
        max_j = j;
        max_dist = dist;
      }
    }
  }

  // Ensure the centers aren't too far apart.
  CHECK_RTN_RESULT(max_dist < 2 * context.ideal.r,
      "dist (%2.f) >= diam. (%.2f)", max_dist, 2 * context.ideal.r);

  // Check that this doesn't look too helix-like.
  double ep_dist = point2d_distance(
      &stroke->pts[0].p2d, &stroke->pts[NP-1].p2d);
  CHECK_RTN_RESULT(ep_dist / stroke->px_length < PALEO_THRESH_U,
      "ep_dist (%.2f) / px_len (%.2f) >= U (%.2f)",
      ep_dist, stroke->px_length, PALEO_THRESH_U);

  // Seems to check out.  Populate the spiral.
  spiral_t* sp = &context.result.spiral;
  paleo_point_t* p = &stroke->pts[NP-1];
  point2d_t* c = &context.ideal.center;
  memcpy(&sp->center, c, sizeof(point2d_t));
  sp->r = bbox_rad;
  sp->theta_t = p->dir - stroke->pts[0].dir;
  sp->theta_f = atan2(p->y - c->y, p->x - c->x);
  while (sp->theta_f < 0) { sp->theta_f += 2 * M_PIl; }
  sp->cw = SGN(sp->theta_t);
  sp->theta_t = abs(sp->theta_t);
  return &context.result;
}