'''Syntactic sugar for all of libpaleo.  Wraps a Stroke and implements an
iterator for it.'''

import bindings as b

import datetime
import pickle
import random
import sys


# Used to get utime -- seconds since the epoc.
UTC_0 = datetime.datetime(1970, 1, 1)

## A `libsr.Stroke` object wraps a C `stroke_t` (defined in
# [@ref src/common/stroke.h]) into a Python-like class.
class Stroke(object):

  ## Store this here so we can call it when Python starts shutting down.
  stroke_destroy = b.stroke_destroy

  ## Creates a new Stroke.  Will either wrap `_stroke` or a newly-created
  # `stroke_t` object.
  #
  # @param _stroke [@ref stroke_t] (optional) The stroke to wrap.  Defaults to a
  #                new stroke.  Takes ownership of the [@ref stroke_t] object,
  #                so it will destroy it in its [@ref __del__()` method.
  def __init__(self, _stroke=None):
    self._stroke = _stroke or b.stroke_create(40)
    self.bbox = (sys.maxint, sys.maxint, -sys.maxint, -sys.maxint)

  ## Destroys (frees) the underlying `stroke_t` object.
  def __del__(self):
    self.stroke_destroy(self._stroke)

  ## Gets the number of points in this stroke.
  #
  # @return [@ref int] The number of points in the stroke.
  def __len__(self):
    return self._stroke.num

  ## Gets the `i`th point from this stroke.  If `i` is a slice, gets the slice
  # as a generator.
  #
  # @param i [@ref int] The index to the stroke to get.
  #
  # @return [@ref libsr.Point] The `i`th point wrapped as a [libsr.Point].
  def __getitem__(self, i):
    if isinstance(i, slice):
      return (Point(b.stroke_get(self._stroke, j))
              for j in xrange(i.start or 0,
                              i.stop or self._stroke.num,
                              i.step or 1))
    if i < 0:
      i += self._stroke.num
    if i < 0 or self._stroke.num <= i:
      raise IndexError('%d \\nin [0,%d)' % (i, self._stroke.num))
    return Point(b.stroke_get(self._stroke, i))

  ## Adds a point to this stroke.
  #
  # @param x [@ref int] X coordinate.
  # @param y [@ref int] Y coordinate.
  # @param t [@ref int] (default: now) The time the point was made (in micro-s).
  def add(self, x, y, t=None):
    t = t or int((datetime.datetime.utcnow() - UTC_0).total_seconds() * 1e6)
    self.bbox = (
      min(self.bbox[0], x),
      min(self.bbox[1], y),
      max(self.bbox[2], x),
      max(self.bbox[3], y),
    )
    b.stroke_add_timed(self._stroke, x, y, t)

  ## Creates a [@ref libsr.StrokeIter] over this [@ref libsr.Stroke]'s
  # [@ref libsr.Point]s.
  #
  # @return The iterator.
  def __iter__(self):
    return StrokeIter(self)

  ## Saves the underlying stroke to file in a libsr-specific way.
  #
  # @param fname [@ref str] The file name to save to.
  def save(self, fname):
    b.stroke_save(self._stroke, str(fname))

  ## Pickles this [@ref libsr.Stroke].
  #
  # @param fname [@ref str] The file name to pickle to.
  def pickle(self, fname):
    with open(fname, 'w') as f:
      pickle.dump(self, f)

  ## Loads a stroke stroke in a `libsr`-specific way and returns it.
  #
  # @param fname [@ref str] The file name to save to.
  #
  # @return [@ref libsr.Stroke] The stroke.
  @classmethod
  def load(cls, fname):
    return Stroke(b.stroke_from_file(str(fname)))

  ## Unpickles the Stroke in `fname`.
  #
  # @return [@ref libsr.Stroke] The [@ref libsr.Stroke] object in `fname`.
  @classmethod
  def unpickle(self, fname):
    with open(fname, 'r') as f:
      return pickle.load(f)


## An iterator over a Stroke object.
class StrokeIter(object):
  ## Creates a new iterator over a stroke.
  #
  # @param stroke [@ref libsr.Stroke] The stroke to iterate over.
  def __init__(self, stroke):
    self._stroke = stroke._stroke
    self.i = 0

  ## Returns itself.
  #
  # @return [@ref libsr.StrokeIter] `self`
  def __iter__(self):
    return self

  ## Gets the next point in the underlying stroke as an `(x, y, t)` tuple.
  #
  # @return [@ref tuple] The next point as an `(x, y, t)` tuple.
  #
  # @throws [@ref StopIteration] when there are no more points.
  def next(self):
    if self.i >= self._stroke.num:
      raise StopIteration()
    p = b.stroke_get(self._stroke, self.i)
    self.i += 1
    return Point(p)


## Wraps a `point_t` structure (defined in [@ref src/common/point_t.h]) into a
# Pythonic class.
class Point(object):
  # Store this here so we can call it when Python starts shutting down.
  point_destroy = b.point_destroy

  ## Creates a new point_t object wrapped around `_point` or a new `point_t`
  # object.

  # @param _point [@ref point_t] (default: new [@ref point_t]) The
  #               [@ref point_t] to wrap.  If this is `None`, manages its own
  #               memory.
  def __init__(self, _point=None):
    if _point:
      self.created_point = False
      self._point = _point
    else:
      self.created_point = True
      self._point = b.point_create()

  ## Destroys the underlying [@ref point_t] object.
  def __del__(self):
    if self.created_point:
      self.point_destroy(self._point)

  ## Generates and returns a [@ref str] of form `i: (x, y) @t`.
  def __repr__(self):
    return '#%d: (%d,%d) @%d' % (self.i, self.x, self.y, self.t)

  ## Generates and returns a [@ref str] of form `Point(x, y, t, i)`.
  def __str__(self):
    return 'Point(%d, %d, %d, %d)' % (self.x, self.y, self.t, self.i)

  ## Returns the `(x, y)` tuple of this point.
  #
  # @return [@ref tuple] The `(x, y)` coordinates of this point.
  def pos(self):
    return self._point.x, self._point.y

  ## Passed attribute access to the underlying [@ref point_t] object.
  def __getattr__(self, attr):
    return getattr(self._point, attr)


# Simple test to ensure the API seems to work.
if __name__ == '__main__':
  stroke = Stroke()
  for i in xrange(10, 110, 10):
    stroke.add(i, i, i)
  for i in xrange(10, 110, 10):
    stroke.add(i, i)

  for p in stroke:
    print p

  for i in xrange(len(stroke)):
    pt = stroke[i]
    print pt
    print pt._point.x, pt._point.y, pt._point.t, pt._point.i
    print pt.x, pt.y, pt.t, pt.i
    for attr in ('X', 'Y', 'T', 'I', 'foo', 'bar'):
      try:
        print getattr(stroke[i], attr)
        assert False, 'Did not raise error.'
      except AttributeError:
        pass


## Debugging help; generates a random stroke.
def _make_random_stroke(num_pts=20, max_int=100):
  stroke = Stroke()
  for x, y in ((random.randrange(max_int), random.randrange(max_int))
               for i in xrange(num_pts)):
    stroke.add(x, y)
  return stroke