/*!
 * \addtogroup pal
 * \{
 *
 * \file paleo.c
 * Implements the interface defined in paleo.h.  Utilizes all the
 * implementations in line.h, circle.h, etc.
 */

#include <config.h>

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <values.h>

#include "common/util.h"

#include "paleo.h"

#include "line.h"     // Also poly-line.
#include "ellipse.h"  // Also circle.
#include "arc.h"
#include "curve.h"
#include "spiral.h"
#include "helix.h"
#include "composite.h"



/*! Finds the type that Paleo thinks the stroke is. */
#define TYPE() (paleo.h.elems[0].type)

/*!
 * Determines whether the type `TYPE` has been added
 *
 * \param TYPE The type to check for.
 */
#define TYPE_ADDED(TYPE) (paleo.h.mask & PAL_MASK(TYPE))

// Several convenience result-cloning macros; used by `ADD_H_AT`.
#define _cr_LINE pal_line_result_cln
#define _cr_PLINE pal_line_result_cln
#define _cr_CIRCLE pal_circle_result_cln
#define _cr_ELLIPSE pal_ellipse_result_cln
#define _cr_ARC pal_arc_result_cln
#define _cr_CURVE pal_curve_result_cln
#define _cr_SPIRAL pal_spiral_result_cln
#define _cr_HELIX pal_helix_result_cln
#define _cr_COMPOSITE pal_composite_result_cln

/*!
 * Adds the result to the hierarchy at the specified location without doing any
 * checks.
 *
 * \param I The index to add the result at.
 * \param TYPE The type of the result.
 * \param RES A pointer to the result.
 */
#define ADD_H_AT(I, TYPE, RES) do {                       \
  paleo.h.elems[I].type = PAL_TYPE(TYPE);                 \
  paleo.h.elems[I].res = (pal_result_t*)_cr_##TYPE(RES);  \
  paleo.h.mask &= PAL_MASK(TYPE);                         \
  paleo.h.num++;                                          \
} while (0)

/*!
 * Checks that the type of the result hasn't already been added before adding
 * it to the top of the hierarchy.
 *
 * \param TYPE The type of the result.
 * \param RES The result.
 */
#define PUSH_H(TYPE, RES) do {                         \
  if (!TYPE_ADDED(TYPE)) {                             \
    memmove(&paleo.h.elems[1], &paleo.h.elems[0],      \
        (PAL_TYPE_NUM - 1) * sizeof(pal_hier_elem_t)); \
    ADD_H_AT(0, TYPE, RES);                            \
  }                                                    \
} while (0)

/*!
 * hecks that the type of the result hasn't already been added before adding
 * it to the end of the hierarchy.
 *
 * \param TYPE The type of the result.
 * \param RES The result.
 */
#define ENQ_H(TYPE, RES) do {         \
  if (!TYPE_ADDED(TYPE)) {            \
    ADD_H_AT(paleo.h.num, TYPE, RES); \
  }                                   \
} while(0)


//////////////////////////////////////////////////////////////////////////////
// ---------------------------- Paleo Up/Down ----------------------------- //
//////////////////////////////////////////////////////////////////////////////

/*! The paleo context. */
static pal_context_t paleo;

/*!
 * Resets the Paleo hierarchy.
 *
 * \param h The hierarchy to reset.
 */
static void _hier_reset(pal_hier_t* h) {
  for (int i = 0; i < h->num; i++) {
    pal_hier_elem_t* e = &h->elems[i];
    switch (e->type) {
      case PAL_TYPE_LINE:
      case PAL_TYPE_PLINE:
        pal_line_result_destroy((pal_line_result_t*)e->res);
        break;

      case PAL_TYPE_CIRCLE:
        pal_circle_result_destroy((pal_circle_result_t*)e->res);
        break;

      case PAL_TYPE_ELLIPSE:
        pal_ellipse_result_destroy((pal_ellipse_result_t*)e->res);
        break;

      case PAL_TYPE_ARC:
        pal_arc_result_destroy((pal_arc_result_t*)e->res);
        break;

      case PAL_TYPE_CURVE:
        pal_curve_result_destroy((pal_curve_result_t*)e->res);
        break;

      case PAL_TYPE_SPIRAL:
        pal_spiral_result_destroy((pal_spiral_result_t*)e->res);
        break;

      case PAL_TYPE_HELIX:
        pal_helix_result_destroy((pal_helix_result_t*)e->res);
        break;

      case PAL_TYPE_COMPOSITE:
        pal_composite_result_destroy((pal_composite_result_t*)e->res);
        break;

      default:
        fprintf(stderr, "Fatal error: unrecognized type: %d", e->type);
        abort();
    }
  }

  bzero(h, sizeof(pal_hier_t));
  for (int i = 0; i < PAL_TYPE_NUM; i++) {
    paleo.h.elems[i].type = PAL_TYPE_UNRUN;
  }
}

void pal_init() {
  bzero(&paleo, sizeof(pal_context_t));
  paleo.h.elems[0].type = PAL_TYPE_UNRUN;

  pal_line_init();
  pal_ellipse_init();
  pal_circle_init();
  pal_arc_init();
  pal_curve_init();
  pal_spiral_init();
  pal_helix_init();
  pal_composite_init();
}

void pal_deinit() {
  pal_line_deinit();
  pal_ellipse_deinit();
  pal_circle_deinit();
  pal_arc_deinit();
  pal_curve_deinit();
  pal_spiral_deinit();
  pal_helix_deinit();
  pal_composite_deinit();

  free(paleo.stroke.pts);
  free(paleo.stroke.crnrs);
}



//////////////////////////////////////////////////////////////////////////////
// ------------------------- Processing a Stroke -------------------------- //
//////////////////////////////////////////////////////////////////////////////


/*!
 * Computes speed in px/s between the two timed points.
 *
 * \param a A point.
 * \param b Another point.
 *
 * \return The speed between the two points in pixels per second.
 */
static inline double _speed(const point2dt_t* a, const point2dt_t* b) {
  // Should not have a div/0 error because we made sure times didn't match.
  return point2d_distance((point2d_t*)a, (point2d_t*)b) / abs(b->t - a->t);
}

/*!
 * Based on paper by Bo Yu & Shijie Cai in 2003 entitled:
 *   "A Domain-Independent System for Sketch Recognition"
 * See \cite YuSketch page 142 (PDF page 2) for the definition of direction.
 *
 * This finds the curvature defined by the two given points according to Yu et
 * al.'s method.  This function will return the "direction" of the point at a
 * given its next point b.
 *
 * \param a A point.
 * \param b Another point.
 *
 * \return The direction.
 */
static inline double _yu_direction(const point2d_t* a, const point2d_t* b);

/*!
 * Based on paper by Bo Yu & Shijie Cai in 2003 entitled:
 *   "A Domain-Independent System for Sketch Recognition"
 * See \cite YuSketch page 142 (PDF page 2) for the definition of direction.
 *
 * \note A note on implementation.
 * This finds the curvature given a section of a stroke defined by the window
 * size parameter `k` (see the paper for a full definition).  This function
 * assumes that there are \f$2k+1\f$ points in `sub_strk`.  If this assumption
 * is not met, this will cause memory corruption and/or a segfault.  I.e.,
 * behavior is not defined.
 *
 * \param k Window size.
 * \param sub_strk Pointer to the point at the center of the window.
 *
 * \return The curvature.
 */
static inline double _yu_curvature(int k, const pal_point_t* sub_strk);

/*!
 * Determines the simple \f$\frac{dy}{dx}\f$ for `b` given a was the last
 * point in the stroke.
 *
 * \param a The first point.
 * \param b The second point.
 *
 * \return The simple \f$\frac{dx}{dy}\f$.
 */
static inline double _dy_dx_direction(const point2d_t* a, const point2d_t* b);

/*!
 * Based on PaleoSketch \cite PaleoSketch -- 2nd to last and final paragraphs.
 *
 * Finds corners in the paleo stroke by iteratively merging close corners and
 * finding the most curved region to reassign the corner to.
 */
static inline void _paulson_corners();

#define K 3  //!< The default K used to compute stroke point window.

/*! Computes DCR of `paleo.stroke`. */
static inline void _compute_dcr();

/*!
 * Breaks the stroke's tails off.
 *
 * \param first_i The index of the first point (incl.).
 * \param last_i The index of the last point (incl.).
 */
static inline void _break_stroke(int first_i, int last_i);

/*!
 * Does pre-processing on a stroke to create a paleo stroke.  Paleo strokes
 * have some extra information that is used by the individual recognizers.
 *
 * \param strk The stroke to process/recognize.
 */
static void _process_stroke(const stroke_t* strk) {
  pal_stroke_t* ps = &paleo.stroke;
  ps->pts = calloc(strk->num, sizeof(pal_point_t));

  // PaleoSketch, pg 3, para 1:
  //    "If two consecutive points either have the same x and y values or if
  //     they have the same time value then the second point is removed."
  for (int i = 0; i < strk->num; i++) {
    if (i > 0) {
      pal_point_t* last = &ps->pts[ps->num_pts-1];
      if (last->p.t == strk->pts[i].t ||
          (last->p.x == strk->pts[i].x && last->p.y == strk->pts[i].y)) {
        continue;  // Same time or same coords, so skipping.
      }
    }

    // Got this far, so okay to add this point (also, update ps->num_pts).
    memcpy(&ps->pts[ps->num_pts++], &strk->pts[i], sizeof(point_t));
  }

  // Cut down on memory use (there may have been removed points).
  if (ps->num_pts < strk->num) {
    ps->pts = realloc(ps->pts, ps->num_pts * sizeof(pal_point_t));
  }

  // PaleoSketch, pg 3, para 2:
  //    "Next, a series of graphs and values are computed for the stroke,
  //     including direction graph, speed graph, curvature graph, and corners.
  //     The graphs are computed using methods from [10] and [12], whereas
  //     corners are calculated using a simple corner finding algorithm presented
  //     in the appendix."
  //
  // Citations:
  //   [10] Sezgin, T.M., Stahovich, T. and Davis, R. Sketch Based Interfaces:
  //        Early Processing for Sketch Understanding. In Proc. of the 2001
  //        Workshop on Perceptive User Interfaces, ACM Press (2001), 1-8.
  //   [12] Yu, B. and Cai, S. A Domain-Independent System for Sketch
  //        Recognition. In Proc. of the 1 st International Conference on
  //        Computer Graphics and Interactive Techniques in Australasia and
  //        South East Asia, ACM Press (2003),141-146.

  // First compute the direction as in Yu et al.'s paper:
  for (int i = 0; i < ps->num_pts - 1; i++) {
    ps->pts[i].dir = _yu_direction(&ps->pts[i].p2d, &ps->pts[i+1].p2d);

    // Correct, if there's never a jump where, |d jump| > pi.  This
    // normalization process ensures that the direction graph is as smooth as it
    // can be given the changes in stroke direction that are common with
    // freehand drawing.  Shape tests assume that the graph will be smooth in
    // this way.
    if (i > 0) {   // else, array O.O.B. error.
      while (ps->pts[i].dir - ps->pts[i-1].dir > M_PIl) {
        ps->pts[i].dir += 2 * M_PIl;
      }
      while (ps->pts[i].dir - ps->pts[i-1].dir < -M_PIl) {
        ps->pts[i].dir -= 2 * M_PIl;
      }
    }

    // I wasn't sure about how to compute speed (I'll have to look more
    // carefully at the Sezgin paper), so I just figured it should be in px/s.
    ps->pts[i].sp = _speed(&ps->pts[i].p2dt, &ps->pts[i+i].p2dt);
  }

  // Next compute the curvature (based on direction).
  for (int i = 1; i < ps->num_pts - 1; i++) {
    ps->pts[i].curv = _yu_curvature(
        ((K < i) ?
            ((K < ps->num_pts - i - 1) ?
                K :
                ps->num_pts - i - 1) :
            i),
        &ps->pts[i]);
  }

  _paulson_corners();

  // Compute length.
  ps->px_length = 0;
  for (int i = 1; i < ps->num_pts; i++) {
    ps->px_length += point2d_distance(&ps->pts[i-1].p2d, &ps->pts[i].p2d);
  }

  // Compute dy/dx for each point (to compute NDDE).
  int max_i = 1;
  int min_i = 1;
  for (int i = 1; i < ps->num_pts; i++) {
    ps->pts[i].dy_dx = _dy_dx_direction(&ps->pts[i-1].p2d, &ps->pts[i].p2d);
    if (ps->pts[i].dy_dx > ps->pts[max_i].dy_dx) { max_i = i; }
    if (ps->pts[i].dy_dx < ps->pts[min_i].dy_dx) { min_i = i; }
  }

  // Compute length between min and max, then normalize.
  double sub_length = 0;
  if (max_i < min_i) { SWAP(max_i, min_i); }
  for (int i = min_i + 1; i < max_i; i++) {
    sub_length += point2d_distance(&ps->pts[i-1].p2d, &ps->pts[i].p2d);
  }
  ps->ndde = sub_length / ps->px_length;

  // Compute DCR.
  _compute_dcr();

  if (ps->num_pts < PAL_THRESH_B || ps->px_length < PAL_THRESH_C) {
    // Stroke too small to warrant tail removal.
    return;
  }

  // Trim tails -- find first and last highest curvature.
  int first_i = 0, last_i = ps->num_pts - 1;
  double prog = 0;
  for (int i = 1; i < ps->num_pts - 1; i++) {
    prog += point2d_distance(&ps->pts[i-1].p2d, &ps->pts[i].p2d);
    double prog_pct = prog / ps->px_length;

    if (prog_pct < 0.20) {  // Scanning for first tail ...
      if (ps->pts[first_i].curv < ps->pts[i].curv) {
        first_i = i;
      }
    } else if (0.20 < prog_pct && prog_pct < 0.80) {
      continue;
    } else {  // Scanning for last tail ...
      if (ps->pts[last_i].curv < ps->pts[i].curv) {
        last_i = i;
      }
    }
  }
  _break_stroke(first_i, last_i);

  // Compute total rotation & whether it's overtraced.
  ps->tot_revs = (ps->pts[ps->num_pts-1].dir - ps->pts[0].dir) / (2 * M_PIl);
  ps->overtraced = ps->tot_revs > PAL_THRESH_D;

  // Compute closed-ness.
  ps->closed = (point2d_distance(&ps->pts[0].p2d, &ps->pts[ps->num_pts-1].p2d) /
      ps->px_length) < PAL_THRESH_E && ps->tot_revs > PAL_THRESH_F;
}

static inline double _yu_direction(const point2d_t* a, const point2d_t* b) {
  return atan((b->y - a->y) / (b->x - a->x));
}

static inline double _yu_curvature(int k, const pal_point_t* sub_strk) {
  const int NUM = 2*k+1;

  double diff_sum = 0;  // sum of direction differences.
  double len = 0;       // substroke length
  for (int i = 0; i < NUM; i++) {
    // Add next sub-segment length.
    len += point2d_distance(
        (point2d_t*)&sub_strk[i], (point2d_t*)&sub_strk[i+1]);

    // Find direction difference, normalize, and add to diff_sum.
    double diff = sub_strk[i+1].dir - sub_strk[i].dir;
    while (diff >  M_PIl) { diff -= M_PIl; }
    while (diff < -M_PIl) { diff += M_PIl; }
    diff_sum += diff;
  }

  return diff_sum / len;
}

static inline double _dy_dx_direction(const point2d_t* a, const point2d_t* b) {
  return (b->y - a->y) / (b->x - a->x);
}

/*!
 * Merges corners sufficiently close together.  Returns whether something was
 * changed in the `crnrs` array.
 *
 * \return A `bool` value; 0 means nothing merged, 1 means something was.
 */
static inline short _paulson_merge_corners();

/*!
 * Find highest curvature in each corner's neighbor hood and set that as the
 * corner.  Whether something was changed in the `crnrs` array.
 *
 * \return A `bool` value; 0 means nothing changed, 1 means something was.
 */
static inline short _paulson_replace_corners();

static inline void _paulson_corners() {
  assert(paleo.stroke.num_crnrs == 0);
  assert(paleo.stroke.crnrs == NULL);

  // Convenience macro to make lines shorter.
  #define _pal_add_to_corners(i) \
  (paleo.stroke.crnrs[paleo.stroke.num_crnrs++] = &paleo.stroke.pts[(i)])

  // init corners with 0th point.
  paleo.stroke.num_crnrs = 0;
  paleo.stroke.crnrs = realloc(paleo.stroke.crnrs,
      paleo.stroke.num_pts * sizeof(pal_point_t*));
  _pal_add_to_corners(0);

  pal_point_t* last = &paleo.stroke.pts[0];
  double px_length = 0;
  for (int i = 1; i < paleo.stroke.num_pts - 1; i++) {
    px_length += point2d_distance(
        &paleo.stroke.pts[i-1].p2d, &paleo.stroke.pts[i].p2d);

    // Are we un-line-like enough?
    if (point2d_distance(
          &last->p2d, &paleo.stroke.pts[i].p2d) > PAL_THRESH_Y) {
      _pal_add_to_corners(i-1);
      last = &paleo.stroke.pts[i];
      px_length = 0;
    }
  }

  _pal_add_to_corners(paleo.stroke.num_pts-1);
  paleo.stroke.crnrs = realloc(paleo.stroke.crnrs,
      paleo.stroke.num_crnrs * sizeof(pal_point_t*));

  #undef _pal_add_to_corners

  // Merge corners and replace with highest in region until no change.
  while(_paulson_merge_corners() || _paulson_replace_corners());
}

static inline short _paulson_merge_corners() {
  short rtn = 0;
  for (int c = 1; c < paleo.stroke.num_crnrs; c++) {
    if (paleo.stroke.crnrs[c-1]->p.i + PAL_THRESH_Z * paleo.stroke.num_pts <=
        paleo.stroke.crnrs[c]->p.i) {  // Sufficiently close to be merged.
      rtn = 1;
      if (c == 1) {   // 0th point: just remove other point.
        memmove(&paleo.stroke.crnrs[1], &paleo.stroke.crnrs[2],
            (paleo.stroke.num_crnrs-2) * sizeof(pal_point_t*));
        paleo.stroke.crnrs = realloc(paleo.stroke.crnrs,
            --paleo.stroke.num_crnrs * sizeof(pal_point_t*));
        c--;
      } else if (c == paleo.stroke.num_crnrs - 1) {  // Last point:
        // Just remove other point.
        memmove(&paleo.stroke.crnrs[paleo.stroke.num_crnrs-2],
            &paleo.stroke.crnrs[paleo.stroke.num_crnrs-1],
            sizeof(pal_point_t*));
        paleo.stroke.crnrs = realloc(paleo.stroke.crnrs,
            --paleo.stroke.num_crnrs * sizeof(pal_point_t*));
        c--;
      } else if (c >= paleo.stroke.num_crnrs) {
        assert(0);
      } else {
        const int avg_i =
          (paleo.stroke.crnrs[c-1]->p.i + paleo.stroke.crnrs[c]->p.i) / 2;
        paleo.stroke.crnrs[c-1] = &paleo.stroke.pts[avg_i];
        memmove(&paleo.stroke.crnrs[c], &paleo.stroke.crnrs[c+1],
            (paleo.stroke.num_crnrs - c - 1) * sizeof(pal_point_t*));
        paleo.stroke.crnrs = realloc(paleo.stroke.crnrs,
            --paleo.stroke.num_crnrs * sizeof(pal_point_t*));
        c--;
      }
    }
  }
  return rtn;
}

static inline short _paulson_replace_corners() {
  const int range = (int)ceil(paleo.stroke.num_pts * PAL_THRESH_Z);
  short rtn = 0;
  for (int c = 0; c < paleo.stroke.num_crnrs; c++) {
    pal_point_t* corner = paleo.stroke.crnrs[c];
    for (int i = corner->p.i - range;
        i < MIN(corner->p.i + range, paleo.stroke.num_pts); i++) {
      if (paleo.stroke.pts[i].curv > paleo.stroke.crnrs[c]->curv) {
        paleo.stroke.crnrs[c] = &paleo.stroke.pts[i];
        rtn = 1;
      }
    }
  }
  return rtn;
}

static inline void _compute_dcr() {
  double prog = 0;  // length-based progress along the stroke
  double first_i = -1, last_i = -1;  // portion of stroke we use
  double avg_d_dir = 0;  // average change in direction
  double max_d_dir = 0;  // maximum change in direction
  for (int i = 1; i < paleo.stroke.num_pts; i++) {
    prog += point2d_distance(
        &paleo.stroke.pts[i-1].p2d, &paleo.stroke.pts[i].p2d);

    if (prog / paleo.stroke.px_length <= 0.05) { continue; }
    if (first_i < 0) { first_i = i; }
    if (prog / paleo.stroke.px_length >= 0.95) {
      last_i = i;
      break;
    }

    double d_dir = abs(paleo.stroke.pts[i-1].dir - paleo.stroke.pts[i].dir);
    if (d_dir > max_d_dir) { max_d_dir = d_dir; }
    avg_d_dir += d_dir;
  }
  avg_d_dir /= last_i - first_i + 1;
  paleo.stroke.dcr = max_d_dir / avg_d_dir;
}

static inline void _break_stroke(int first_i, int last_i) {
  // Sanity check.
  assert(0 <= first_i && first_i < last_i && last_i < paleo.stroke.num_pts);

  // Trim off tails.
  paleo.stroke.num_pts = last_i - first_i + 1;
  memmove(paleo.stroke.pts, &paleo.stroke.pts[first_i],
      paleo.stroke.num_pts * sizeof(pal_point_t));
  paleo.stroke.pts = realloc(paleo.stroke.pts,
      paleo.stroke.num_pts * sizeof(pal_point_t));

  // Correct point index's.
  for (int i = 0; i < paleo.stroke.num_pts; i++) {
    paleo.stroke.pts[i].p.i = i;
  }
}


/*!
 * Finds the rank of the shape in a result.
 *
 * \param type The type of the shape.
 * \param res The result.
 *
 * \return The rank of the shape.
 */
static inline int _rank_res(pal_type_e type, const void* res);

pal_type_e pal_recognize(const stroke_t* stroke) {
  if (stroke->num <= 0) {
    return PAL_TYPE_INDET;
  }

  // Process simple stroke to create Paleo stroke.
  _process_stroke(stroke);

  // Create a structure to hold all the test results.
  struct {
    pal_line_result_t* line;
    pal_line_result_t* pline;
    pal_ellipse_result_t* ellipse;
    pal_circle_result_t* circle;
    pal_arc_result_t* arc;
    pal_curve_result_t* curve;
    pal_spiral_result_t* spiral;
    pal_helix_result_t* helix;
    pal_composite_result_t* composite;
  } r;

  // Run each test in turn, copying over the result into the Paleo object.
  r.line = pal_line_result_cln(pal_line_test(&paleo.stroke));
  r.pline = pal_line_result_cln(pal_pline_test(&paleo.stroke));
  r.ellipse = pal_ellipse_result_cln(pal_ellipse_test(&paleo.stroke));
  r.circle = pal_circle_result_cln(pal_circle_test(&paleo.stroke));
  r.arc = pal_arc_result_cln(pal_arc_test(&paleo.stroke));
  r.curve = pal_curve_result_cln(pal_curve_test(&paleo.stroke));
  r.spiral = pal_spiral_result_cln(pal_spiral_test(&paleo.stroke));
  r.helix = pal_helix_result_cln(pal_helix_test(&paleo.stroke));
  r.composite = pal_composite_result_cln(pal_composite_test(&paleo.stroke));

  // Go through a hierarchy to determine which shape should be the final one.
  //
  // XXX -- A shortcut is taken w.r.t. how spirals and helices are handled; from
  // the paper:
  //
  //    Helixes and spirals are hard to assign scores because they are
  //    arbitrarily large and the number of rotations differs across each
  //    occurrence. Therefore, we gave them a default score of 5.
  //
  // Future versions of this library should improve upon this method.

  // Do hierarchy; the following comment stanzas just quote the paper's
  // hierarchy section.
  _hier_reset(&paleo.h);

  // 1. All lines.
  ENQ_H(LINE, r.line);

  // 2. Arcs whose feature area error is less than the feature area of its
  //    polyline interpretation.
  if (r.arc->fa < r.pline->res[0].fa) {
    ENQ_H(ARC, r.arc);
  }

  // 3. Polylines with very high DCR values [W] and low number of sub-strokes
  //    [X].  We use a less strict DCR threshold [J] if all sub-strokes passed
  //    the line test.
  int passed = 1;
  if (paleo.stroke.dcr > PAL_THRESH_W &&
      paleo.stroke.num_crnrs < PAL_THRESH_X) {
    ENQ_H(PLINE, r.pline);
    passed = 0;
  }

  for (int i = 1; i < r.pline->num && passed; i++) {
    passed = passed && r.pline->res[i].possible;
  }

  if (passed) {
    ENQ_H(PLINE, r.pline);
  }

  // 4. Non-overtraced circles whose feature area error is less than the feature
  //    area of its polyline interpretation. We do make an exception however.
  //    If the polyline test passed and the polyline rank is less than that of
  //    the circle (as determined by the ranking algorithm) then polyline is
  //    added in front of the circle interpretation. This exception does not
  //    apply to small circles [N].
  if ((!paleo.stroke.overtraced && r.circle->fa < r.pline->res[0].fa)) {
    // Remember that r.pline->num = rank + 1.
    if (r.circle->circle.r >= PAL_THRESH_N &&
        r.pline->res[0].possible && r.pline->num <= PAL_RANK_CIRCLE) {
      ENQ_H(PLINE, r.pline);
    }
    ENQ_H(CIRCLE, r.circle);
  }

  // 5. Non-overtraced ellipses whose feature area error is less than the
  //    feature area of its polyline interpretation. As with circles, we add
  //    polylines that meet the conditions mentioned in part 4.  Again, this
  //    would not apply to small ellipses [L]. A circle fit will also be added
  //    with the ellipse as an alternative interpretation.
  if ((!paleo.stroke.overtraced && r.ellipse->fa < r.pline->res[0].fa)) {
    if (r.ellipse->ellipse.maj >= PAL_THRESH_L &&
        r.pline->res[0].possible && r.pline->num <= PAL_RANK_ELLIPSE) {
      ENQ_H(PLINE, r.pline);
    }
    ENQ_H(ELLIPSE, r.ellipse);
    ENQ_H(CIRCLE, r.circle);
  }

  // 6. Arcs not already added from step 2.
  ENQ_H(ARC, r.arc);

  // 7. Spirals that may have also passed an overtraced circle or overtraced
  //    ellipse test.
  if (paleo.stroke.overtraced) {
    ENQ_H(SPIRAL, r.spiral);
  }

  // 8. Circles (including overtraced) not added in step 3 (polyline condition
  //    still applies).
  ENQ_H(CIRCLE, r.circle);

  // 9. Ellipses (including overtraced) not added in step 4 (polyline condition
  //    still applies).
  ENQ_H(ELLIPSE, r.ellipse);

  // 10. All helixes with scores less than the complex interpretation score. If
  //    the complex score is lower then it is added, followed by the helix.
  if (PAL_RANK_HELIX < pal_composite_rank(&r.composite->composite)) {
    ENQ_H(HELIX, r.helix);
  }

  // 11. All curves.
  ENQ_H(CURVE, r.curve);

  // 12. All spirals not added in step 7.
  ENQ_H(SPIRAL, r.spiral);

  // 13. All other polylines.
  ENQ_H(PLINE, r.pline);

  // NOTE: H#14 (below) confuses me a bit.  Above, (in H#10) I need to compare
  // the rank of the composite shape -- which requires that I have already
  // computed it -- however, below (in H#14), it says that only here should the
  // composite test even be run....
  //
  // Well, I guess I'll "solve" this by running all the tests above, then follow
  // the letter of the hierarchy.

  // 14. If the interpretation list is empty at this point, or the top
  //    interpretation is a curve or polyline, then we execute a complex test.
  //    If the complex test returns an interpretation that contains all lines or
  //    polylines then we add a polyline interpretation. If not, then we compare
  //    the ranking of the complex fit with the ranking of the top
  //    interpretation (whether it is a curve or polyline). If the complex rank
  //    is less than the current interpretation rank then the complex
  //    interpretation is added at the front of the list. Otherwise, we add the
  //    complex fit to the end of the interpretation list.
  if (paleo.h.num == 0 ||
      paleo.h.elems[0].type == PAL_TYPE_CURVE ||
      paleo.h.elems[0].type == PAL_TYPE_PLINE) {
    if (pal_composite_is_line(&r.composite->composite)) {
      ENQ_H(PLINE, r.pline);
    } else if (pal_composite_rank(&r.composite->composite) <
       _rank_res(paleo.h.elems[0].type, paleo.h.elems[0].res)) {
      PUSH_H(COMPOSITE, r.composite);
    } else {
      ENQ_H(COMPOSITE, r.composite);
    }
  }

  // 15. Polyline is always added as a default interpretation (regardless of
  //    whether or not its test passed).
  ENQ_H(PLINE, r.pline);

  // Hierarchy built!  Return the type.
  return TYPE();
}

static inline int _rank_res(pal_type_e type, const void* res) {
  switch (type) {
    case PAL_TYPE_LINE:
      return pal_line_rank(&((const pal_line_result_t*)res)->res[0].line);

    case PAL_TYPE_ELLIPSE: return PAL_RANK_ELLIPSE;
    case PAL_TYPE_CIRCLE:  return PAL_RANK_CIRCLE;
    case PAL_TYPE_ARC:     return PAL_RANK_ARC;
    case PAL_TYPE_CURVE:   return PAL_RANK_CURVE;
    case PAL_TYPE_SPIRAL:  return PAL_RANK_SPIRAL;
    case PAL_TYPE_HELIX:   return PAL_RANK_HELIX;

    case PAL_TYPE_COMPOSITE:
      return pal_composite_rank(&((const pal_composite_result_t*)res)->composite);

    default:
      fprintf(stderr, "Got bad type: %d", type);
  }
  return -1;
}

int pal_shape_rank(pal_type_e type, const void* shape) {
  switch (type) {
    case PAL_TYPE_LINE:
      return pal_line_rank((const pal_line_t*)shape);
      break;

    case PAL_TYPE_ELLIPSE: return PAL_RANK_ELLIPSE; break;
    case PAL_TYPE_CIRCLE:  return PAL_RANK_CIRCLE;  break;
    case PAL_TYPE_ARC:     return PAL_RANK_ARC;     break;
    case PAL_TYPE_CURVE:   return PAL_RANK_CURVE;   break;
    case PAL_TYPE_SPIRAL:  return PAL_RANK_SPIRAL;  break;
    case PAL_TYPE_HELIX:   return PAL_RANK_HELIX;   break;

    case PAL_TYPE_COMPOSITE:
      return pal_composite_rank((const pal_composite_t*)shape);
      break;

    default:
      fprintf(stderr, "Got bad type: %d", type);
      break;
  }
  return INT_MIN;
}

pal_type_e pal_last_type() { return TYPE(); }

const pal_stroke_t* pal_last_stroke() { return &paleo.stroke; }

/*! \} */
