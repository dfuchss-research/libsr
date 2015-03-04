#include <check.h>

#include "point.h"
#include "geom.h"



//////////////////////////////////////////////////////////////////////////////
// ------------------------------ Vec Tests ------------------------------- //
//////////////////////////////////////////////////////////////////////////////

// Tests that ||<x1,y1> x <x2,y2>|| == e.
static inline void _test_vec_cross_prod(double e,
    double x1, double y1, double x2, double y2) {
  point2d_t a = { x1, y1 }, b = { x2, y2 };
  double r = vec_cross_prod(&a, &b);
  ck_assert_msg(r == e, "Expected %.2f, got %.2f", e, r);
}

START_TEST(c_vec_cross_prod) {
  _test_vec_cross_prod(-4400, 20, 40, 90, -40);
} END_TEST

START_TEST(c_vec_cross_prod_same) {
  _test_vec_cross_prod(0, 20, 40, 20, 40);
} END_TEST

START_TEST(c_vec_cross_prod_opp) {
  _test_vec_cross_prod(0, 40, 20, -40, -20);
} END_TEST


// ---- Vector Subtraction ------------

static inline void _test_vec_sub(double xe, double ye,
    double x1, double y1, double x2, double y2) {
  point2d_t a = { x1, y1 }, b = { x2, y2 }, r;
  vec_sub(&r, &a, &b);
  ck_assert_msg(r.x == xe, "Expected %.2f, got %.2f", xe, r.x);
  ck_assert_msg(r.y == ye, "Expected %.2f, got %.2f", ye, r.y);
}

START_TEST(c_vec_sub) {
  _test_vec_sub(60,50, 40,80, -20,30);
} END_TEST

START_TEST(c_vec_sub_same) {
  _test_vec_sub(0,0, 40,-40, 40,-40);
} END_TEST

START_TEST(c_vec_sub_opp) {
  ck_assert_msg(80,-40, -40,20, 40,-20);
} END_TEST


//////////////////////////////////////////////////////////////////////////////
// ------------------------------ Area Tests ------------------------------ //
//////////////////////////////////////////////////////////////////////////////

static inline void _test_triangle_area(double e,
    double x1, double y1, double x2, double y2, double x3, double y3) {
  point2d_t a = { x1, y1 }, b = { x2, y2 }, c = { x3, y3 };
  double r = geom_triangle_area(&a, &b, &c);
  ck_assert_msg(r == e, "Expected %.2f, got %.2f", e, r);
}

START_TEST(c_triangle_area_0) {
  _test_triangle_area(0, 0,0, 0,0, 0,0);
} END_TEST

START_TEST(c_triangle_area_big) {
  //_test_triangle_area(0, 40,40, 80,90, 200,500);
  ck_assert_msg(0, "not impl");
} END_TEST

START_TEST(c_triangle_area_small) {
  ck_assert_msg(0, "not impl");
} END_TEST

START_TEST(c_triangle_area_iso) {
  ck_assert_msg(0, "not impl");
} END_TEST

START_TEST(c_triangle_area_right) {
  ck_assert_msg(0, "not impl");
} END_TEST

START_TEST(c_triangle_area_eq) {
  ck_assert_msg(0, "not impl");
} END_TEST


// ---- Quad Area ------------

static inline void _test_quad_area(double e,
    double x1, double y1, double x2, double y2,
    double x3, double y3, double x4, double y4) {
  point2d_t a = { x1, y1 }, b = { x2, y2 }, c = { x3, y3 }, d = { x4, y4 };
  double r = geom_quad_area(&a, &b, &c, &d);
  ck_assert_msg(r == e, "Expected %.2f, got %.2f", e, r);
}

START_TEST(c_quad_area_0) {
  _test_quad_area(0, 0,0, 0,0, 0,0, 0,0);
} END_TEST

START_TEST(c_quad_area_crossing) {
  _test_quad_area(400, 20,100, 60,60, 60,80, 20,80);
} END_TEST

START_TEST(c_quad_area_square) {
  ck_assert_msg(0, "not impl");
} END_TEST

START_TEST(c_quad_area_deg) {
  ck_assert_msg(0, "not impl");
} END_TEST

START_TEST(c_quad_area_ovlp) {
  ck_assert_msg(0, "not impl");
} END_TEST

START_TEST(c_quad_area_normal) {
  ck_assert_msg(0, "not impl");
} END_TEST



//////////////////////////////////////////////////////////////////////////////
// ---------------------------- Line / Segment ---------------------------- //
//////////////////////////////////////////////////////////////////////////////

// ---- geom_seg_seg_intersect ------------

static inline void _test_seg_seg_intersect(char e,
    double x1, double y1, double x2, double y2,
    double x3, double y3, double x4, double y4) {
  point2d_t a = { x1, y1 }, b = { x2, y2 }, c = { x3, y3 }, d = { x4, y4 };
  char r = geom_seg_seg_intersect(&a, &b, &c, &d);
  ck_assert_msg(r == e, "Expected %.2f, got %.2f", e, r);
}

START_TEST(c_seg_seg_intersect_parallel) {
  ck_assert_msg(0, "not impl");
} END_TEST

START_TEST(c_seg_seg_intersect_overlapping) {
  ck_assert_msg(0, "not impl");
} END_TEST

START_TEST(c_seg_seg_intersect_same) {
  ck_assert_msg(0, "not impl");
} END_TEST

START_TEST(c_seg_seg_intersect_cross) {
  ck_assert_msg(0, "not impl");
} END_TEST

START_TEST(c_seg_seg_intersect_cross_front) {
  ck_assert_msg(0, "not impl");
} END_TEST


// ---- geom_seg_line_intersect ------------

static inline void _test_seg_line_intersect(char e,
    double x1, double y1, double x2, double y2,
    double x3, double y3, double x4, double y4) {
  point2d_t a = { x1, y1 }, b = { x2, y2 }, c = { x3, y3 }, d = { x4, y4 };
  char r = geom_seg_line_intersect(&a, &b, &c, &d);
  ck_assert_msg(r == e, "Expected %.2f, got %.2f", e, r);
}

// TODO


// ---- geom_seg_line_intersection ------------

static inline void _test_seg_line_intersection(point2d_t* e,
    double x1, double y1, double x2, double y2,
    double x3, double y3, double x4, double y4) {
  point2d_t isect = { -1, -1 };
  point2d_t a = { x1, y1 }, b = { x2, y2 }, c = { x3, y3 }, d = { x4, y4 };
  char r = geom_seg_line_intersection(&isect, &a, &b, &c, &d);
  ck_assert_msg((!r) == (!e), "Expected %.2f, got %.2f", e, r);
  if (e) {
    ck_assert_msg(memcmp(&isect, e, sizeof(point2d_t)),
          "Expected (%.2f,%.2f), got (%.2f,%.2f)",
          e->x, e->y, isect.x, isect.y);
  }
}

// TODO


// ---- geom_line_line_intersection ------------

static inline void _test_line_line_intersection(point2d_t* e,
    double x1, double y1, double x2, double y2,
    double x3, double y3, double x4, double y4) {
  point2d_t isect = { -1, -1 };
  point2d_t a = { x1, y1 }, b = { x2, y2 }, c = { x3, y3 }, d = { x4, y4 };
  char r = geom_line_line_intersection(&isect, &a, &b, &c, &d);
  ck_assert_msg((!r) == (!e), "Expected %.2f, got %.2f", e, r);
  if (e) {
    ck_assert_msg(memcmp(&isect, e, sizeof(point2d_t)),
          "Expected (%.2f,%.2f), got (%.2f,%.2f)",
          e->x, e->y, isect.x, isect.y);
  }
}

// TODO



//////////////////////////////////////////////////////////////////////////////
// ------------------------------ Auxiliary ------------------------------- //
//////////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////////
// ----------------------------- Entry Point ------------------------------ //
//////////////////////////////////////////////////////////////////////////////

static Suite* geom_suite() {
  Suite* suite = suite_create("geom");

  TCase* tc = tcase_create("vec cross");
  tcase_add_test(tc, c_vec_cross_prod);
  tcase_add_test(tc, c_vec_cross_prod_same);
  tcase_add_test(tc, c_vec_cross_prod_opp);
  suite_add_tcase(suite, tc);

  tc = tcase_create("vec sub");
  tcase_add_test(tc, c_vec_sub);
  tcase_add_test(tc, c_vec_sub_same);
  tcase_add_test(tc, c_vec_sub_opp);
  suite_add_tcase(suite, tc);

  tc = tcase_create("triangle area");
  tcase_add_test(tc, c_triangle_area_0);
  tcase_add_test(tc, c_triangle_area_big);
  tcase_add_test(tc, c_triangle_area_small);
  tcase_add_test(tc, c_triangle_area_iso);
  tcase_add_test(tc, c_triangle_area_right);
  tcase_add_test(tc, c_triangle_area_eq);
  suite_add_tcase(suite, tc);

  tc = tcase_create("quad area");
  tcase_add_test(tc, c_quad_area_0);
  tcase_add_test(tc, c_quad_area_crossing);
  tcase_add_test(tc, c_quad_area_square);
  tcase_add_test(tc, c_quad_area_deg);
  tcase_add_test(tc, c_quad_area_ovlp);
  tcase_add_test(tc, c_quad_area_normal);
  suite_add_tcase(suite, tc);

  tc = tcase_create("segs intersect");
  tcase_add_test(tc, c_seg_seg_intersect_parallel);
  tcase_add_test(tc, c_seg_seg_intersect_overlapping);
  tcase_add_test(tc, c_seg_seg_intersect_same);
  tcase_add_test(tc, c_seg_seg_intersect_cross);
  tcase_add_test(tc, c_seg_seg_intersect_cross_front);
  suite_add_tcase(suite, tc);

  return suite;
}

int main() {
  int number_failed = 0;
  Suite* suite = geom_suite();
  SRunner* runner = srunner_create(suite);

  srunner_run_all(runner, CK_VERBOSE);
  number_failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
