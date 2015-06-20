#!/usr/bin/python

import sys
import random

from pprint import pprint
from PyQt4 import QtCore
from PyQt4 import QtGui

import libsr


class Canvas(QtGui.QGraphicsView):
  '''Canvas to collect and display strokes.'''

  mouse_down = False

  def __init__(self):
#    super(Canvas, self).__init__(QtGui.QGraphicsScene())
    super(Canvas, self).__init__()
    self.strokes = []
    self.setBackgroundBrush(QtCore.Qt.white)
    self.setWindowTitle('Canvas')
    self.show()

  def render(self, painter, **kwargs):
    for stroke in self.strokes:
      pen = QtGui.QPen(QtCore.Qt.SolidLine)
      pen.setColor(QtCore.Qt.black)
      pen.setWidth(1)
      painter.setPen(pen)
      pp = QtGui.QPainterPath()
      pp.moveTo(stroke[0].x, stroke[0].y)
      for pt in stroke[1:]:
        pp.lineTo(pt.x, pt.y)
      painter.drawPath(pp)
      pen.setColor(QtCore.Qt.black)
      painter.setPen(pen)

  def _append_pos(self, x, y):
    assert self.mouse_down, 'error: mouse not down'
#    rect = self.scene().items()[0].add(self.mapToScene(x, y))
#    self.scene().invalidate(*rect)
    self.strokes[-1].add(x, y)

  def mousePressEvent(self, e):
    self.mouse_down = True
#    self.scene().addItem(StrokeGraphiscItem(self.mapToScene(e.x(), e.y())))
    self.strokes.append(libsr.Stroke())
    self._append_pos(e.x(), e.y())

  def mouseMoveEvent(self, e):
    self._append_pos(e.x(), e.y())

  def mouseReleaseEvent(self, e):
    self._append_pos(e.x(), e.y())
    self.mouse_down = False


#class StrokeGraphiscItem(QtGui.QGraphicsItem):
#  DRAW_BUF = 5   # Small buffer to get some ... well ... buffer around rects.
#
#  def __init__(self, scene_point):
#    super(StrokeGraphiscItem, self).__init__()
#    self.stroke = libsr.Stroke()
#    self._add_scene_point(scene_point)
#
#  def _add_scene_point(self, scene_point):
#    pt = self.mapFromScene(scene_point)
#    self.stroke.add(int(pt.x()), int(pt.y()))
#    self.prepareGeometryChange()
#
#  def add(self, scene_point):
#    self._add_scene_point(scene_point)
##    x, y, w, h = (min(self.stroke[-1].x, self.stroke[-2].x) - self.DRAW_BUF,
##                  min(self.stroke[-1].y, self.stroke[-2].y) - self.DRAW_BUF,
##                  max(1, abs(self.stroke[-1].x - self.stroke[-2].x)) +
##                    self.DRAW_BUF + self.DRAW_BUF,
##                  max(1, abs(self.stroke[-1].y - self.stroke[-2].y)) +
##                    self.DRAW_BUF + self.DRAW_BUF)
##    return x, y, w, h
#
#  def boundingRect(self):
#    return QtCore.QRectF(*self.stroke.bbox)
#
#  def paint(self, painter, option, widget):
#    pen = QtGui.QPen(QtCore.Qt.SolidLine)
#    pen.setColor(QtCore.Qt.black)
#    pen.setWidth(5)
#    painter.setPen(pen)
#    pp = QtGui.QPainterPath()
#    pp.moveTo(self.stroke[0].x, self.stroke[0].y)
#    for pt in self.stroke[1:]:
#      pp.lineTo(pt.x, pt.y)
#    painter.drawPath(pp)
#    pen.setColor(QtCore.Qt.black)
#    painter.setPen(pen)


def main(*args):
  app = QtGui.QApplication(list(args))
  c = Canvas()
  sys.exit(app.exec_())


if __name__ == '__main__':
  main(*sys.argv)

