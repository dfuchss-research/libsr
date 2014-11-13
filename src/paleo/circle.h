#ifndef __circle_h__
#define __circle_h__

#include "point.h"

// A circle with some number of joints.
typedef struct {
  long r;     // The radius of the circle.
  point_t* c;  // The center of the circle.
} circle_t;

// Creates a new circle at 0,0 with radius 0.
circle_t* circle_create();

// Creates a circle with the given center and radius.
//   r: The radius.
//   x: X coordinate.
//   y: Y coordinate.
circle_t* circle_create_full(long r, long x, long y);

// Creates a circle with the given center and radius.  The passed center will
// be taken over by the circle -- in other words, it will not be cloned in
// this function; it will be freed on a call to circle_destroy(circle_t*).
//   r: The radius.
//   c: The center.
circle_t* circle_create_with_point(long r, point_t* c);

// Destroys the circle by freeing its memory.
void circle_destroy(circle_t*);

#endif // __circle_h__
