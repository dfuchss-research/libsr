#include <assert.h>
#include <math.h>
#include <string.h>
#include <values.h>

#include "common/geom.h"
#include "dollarp.h"

/*! Moves the stroke to be centered at the origin, \f$(0,0)\f$.
 *
 * \param strk The stroke.
 */
static inline void _translate_to_origin(stroke_t* strk) {
  point2d_t c = { 0, 0 };
  for (int i = 0; i < strk->num; i++) {
    c.x += strk->pts[i].x;
    c.y += strk->pts[i].y;
  }

  c.x /= strk->num;
  c.y /= strk->num;

  for (int i = 0; i < strk->num; i++) {
    strk->pts[i].x -= c.x;
    strk->pts[i].y -= c.y;
  }
}

/*! Scales the stroke, i.e., normalizes the size.
 *
 * \param strk The stroke to normalize.
 */
static inline void _scale(stroke_t* strk) {
  point2d_t min = { DBL_MAX, DBL_MAX };
  point2d_t max = { 0, 0 };

  for (int i = 0; i < strk->num; i++) {
    min.x = MIN(min.x, strk->pts[i].x);
    min.y = MIN(min.y, strk->pts[i].y);
    max.x = MAX(max.x, strk->pts[i].x);
    max.y = MAX(max.y, strk->pts[i].y);
  }

  double scale = MAX(max.x - min.x, max.y - min.y);
  for (int i = 0; i < strk->num; i++) {
    strk->pts[i].x = (strk->pts[i].x - min.x) / scale;
    strk->pts[i].y = (strk->pts[i].y - min.y) / scale;
  }
}

/*! Finds the length of the path along the stroke.
 * \param strk The stroke.
 * \return The length along the stroke.
 */
static inline double _path_length(const stroke_t* strk) {
  double len = 0;
  for (int i = 1; i < strk->num; i++) {
    len += point2d_distance(&strk->pts[i].p2d, &strk->pts[i-1].p2d);
  }
  return len;
}

/*! Resamples the stroke to have `n` points.
 *
 * \param strk The stroke to resample.
 * \param n The number of points the stroke should have.
 *
 * \return The resampled stroke.
 */
static inline void _resample(stroke_t* strk, int n) {
  double I = _path_length(strk) / (n - 1);
  double D = 0;
  stroke_t* r_strk = stroke_create(n);
  stroke_add_coords(r_strk, strk->pts[0].x, strk->pts[0].y);
  for (int i = 1; i < strk->num; i++) {
    point2d_t* curr = &strk->pts[i].p2d;
    point2d_t* prev = &strk->pts[i-1].p2d;
    double d = point2d_distance(curr, prev);
    if (D + d >= I) {
      point2d_t q = {
        prev->x + ((I - D) / d) * (curr->x - prev->x),
        prev->y + ((I - D) / d) * (curr->y - prev->y)
      };
      stroke_add_coords(r_strk, q.x, q.y);
      D = 0;

      // Set up for the next curr/prev pair to be q and i-1.
      stroke_insert_at(strk, i, q.x, q.y);
      i--;
    } else {
      D += d;
    }
  }

  // Copy the computed resampled stroke to the input stroke and clean up.
  memcpy(strk, r_strk, sizeof(stroke_t));
  stroke_destroy(r_strk);
}

/*! "Normalizes" the stroke to have just `n` points in it centered at the
 * origin.  Note that this mutates `strk`.
 *
 * \param strk The stroke to normalize
 * \param n The number of points to keep in the stroke.
 */
static inline void _normalize(stroke_t* strk, int n) {
  _resample(strk, n);
  _scale(strk);
  _translate_to_origin(strk);
}

/*! Finds the distance between two clouds.
 *
 * From the paper:
 * <blockquote>
 * Distance between two clouds. Compute the minimum-cost alignment between
 * points and `tmpl` starting with point start. Assign decreasing confidence
 * weights \f$\in [0,1]\f$ to point matchings.
 * </blockquote>
 *
 * \param c1 One point cloud.
 * \param c2 Another point cloud.
 * \param start The start index to search from.
 * \param n The size of the strokes.
 *
 * \return The distance between the clouds.
 */
static inline double
_cloud_dist(const stroke_t* c1, const stroke_t* c2, int start) {
  // Effective array of booleans.
  int* matched = calloc(c1->num, sizeof(char));
  double sum = 0;
  int i = 0;
  int index = -1;
  do {
    double min = DBL_MAX;
    for (int j = 0; j < c1->num; j++) {
      if (!matched[j]) {
        double d = point2d_distance(&c1->pts[i].p2d, &c2->pts[j].p2d);
        if (d < min) {
          min = d;
          index = j;
        }
      }
    }

    matched[index] = 1;
    double weight = 1 - ((i - start + c1->num) % c1->num) / c1->num;
    sum += weight * min;
    i = (i + 1) % c1->num;
  } while (i != start);

  return sum;
}

/*! Attempt to match two point clouds.
 *
 * From the article:
 * <blockquote>
 * Cloud matching function. Match two clouds (points and template) by performing
 * repeated alignments between their points (each new alignment starts with a
 * different starting point index \f$i\f$).  Parameter \f$\varepsilon \in
 * [0,1]\f$ controls the number of tested alignments (\f$n^\varepsilon \in \{1,
 * 2, \ldots, n\}\f$). Returns the minimum alignment cost.
 * </blockquote>
 *
 * \param self The $P context.
 * \param c1 A point cloud to compare.
 * \param c2 Another point cloud to compare.
 */
static inline double
_greedy_cloud_match(const dp_context_t* self, const stroke_t* c1,
                    const stroke_t* c2) {
  assert(c1->num == c2->num);

  double min = DBL_MAX;
  for (double i = 0; i < self->n; i += self->step) {
    double d[] = {
      _cloud_dist(c1, c2, (int)round(i)),
      _cloud_dist(c2, c1, (int)round(i))
    };
    min = MIN(d[0], d[1]);
  }
  return min;
}



////////////////////////////////////////////////////////////////////////////////
//                             "Public" Functions                             //
////////////////////////////////////////////////////////////////////////////////

//! The amount to increase the template size by, each time it needs increasing.
#define _DP_TMPL_INC 100

dp_context_t* dp_create() {
  dp_context_t* self = calloc(1, sizeof(dp_context_t));
  self->n = DP_DEFAULT_N;
  dp_set_epsilon(self, DP_DEFAULT_EPSILON);
  self->tmpls = calloc(self->cap = _DP_TMPL_INC, sizeof(dp_template_t));
  return self;
}

void dp_add_template(dp_context_t* self, stroke_t* strk, const char* name) {
  // Ensure we have enough space.
  if (self->num >= self->cap) {
    self->cap += _DP_TMPL_INC;
    self->tmpls = realloc(self->tmpls, self->cap * sizeof(dp_template_t));
  }

  _normalize(strk, self->n);

  // Add this template.
  dp_template_t* next = &self->tmpls[self->num++];
  next->strk = strk;
  strncpy(next->name, name, DP_MAX_TMPL_NAME_LEN);
}

dp_result_t dp_recognize(const dp_context_t* self, stroke_t* strk) {
  // Init for recognition.
  _normalize(strk, self->n);

  // Try each template, see which one works.
  dp_template_t* tmpl = NULL;
  double score = DBL_MAX;
  for (int i = 0; i < self->num; i++) {
    double d = _greedy_cloud_match(self, strk, self->tmpls[i].strk);
    if (score > d) {
      score = d;
      tmpl = &self->tmpls[i];
    }
  }

  // Normalize score in [0,1] and return the result.
  dp_result_t result = { tmpl, MAX((2.0 - score) / 2.0, 0) };
  return result;
}

void dp_destroy(dp_context_t* self) {
  for (int i = 0; i < self->num; i++) {
    stroke_destroy(self->tmpls[i].strk);
  }
  free(self->tmpls);
  free(self);
}