#!/usr/bin/python

## @package tools

## @file
# The UI for the collector.  Ties into libsr Python2.7 package.
#
# The purpose of this tool is to collect strokes for testing.

import os
import sys

from PyQt5 import QtCore
from PyQt5 import QtGui
from PyQt5 import QtWidgets
from PyQt5 import uic

# Do a bit extra for importing libsr, just in case the user installed it.
try:
  import libsr
except ImportError:
  add_path = "/usr/local/lib/python2.7"
  sys.path.append(add_path)
  try:
    import libsr
    print 'Warning, had to add "%s" to path for libsr.' % add_path
  except ImportError as e:
    print 'Could not import libsr!'
    print 'Error message:', e.message
    print 'Did you run make install?'
    sys.exit(-1)

from qt_util import SaveDestroyCancelBox, popup

## The top level widget.  All hail the top level widget!
class Collector(QtWidgets.QMainWindow):
  ## Creates the collector based on the UI file.
  #
  # @param self The object to initialize.
  # @param ui_file (str) The UI file to load.
  def __init__(self, ui_file):
    super(QtWidgets.QMainWindow, self).__init__()
    uic.loadUi(ui_file, self)

    ##########################
    # Message & File Dialogs #
    ##########################

    self.save_file_dialog = QtWidgets.QFileDialog(self, 'Save to Directory')
    self.save_file_dialog.setAcceptMode(QtWidgets.QFileDialog.AcceptSave)
    self.save_file_dialog.setFileMode(QtWidgets.QFileDialog.AnyFile)
    self.save_file_dialog.setViewMode(QtWidgets.QFileDialog.List)
    self.save_file_dialog.fileSelected.connect(self.save_dir_selected)

    self.load_file_dialog = QtWidgets.QFileDialog(self, 'Load from Directory')
    self.load_file_dialog.setAcceptMode(QtWidgets.QFileDialog.AcceptOpen)
    self.load_file_dialog.setFileMode(QtWidgets.QFileDialog.Directory)
    self.load_file_dialog.setViewMode(QtWidgets.QFileDialog.List)
    self.load_file_dialog.fileSelected.connect(self.load_dir_selected)

    self.new_dialog = SaveDestroyCancelBox(
        self, "Save before creating new?", self._handle_new_dialog_click)
    self.quit_dialog = SaveDestroyCancelBox(
        self, "Save before exiting?", self._handle_quit_dialog_click)

    #############
    # File Menu #
    #############

    def save():
      if self.dirty():
        self.save_file_dialog.show()
    self.actionSave.triggered.connect(save)

    def load():
      if self.dirty():
        self.save_file_dialog.show()
      else:
        self.load_file_dialog.show()
    self.actionLoad.triggered.connect(load)

    def new():
      if self.dirty():
        self.new_dialog.show()
      else:
        self._clear_strokes()
    self.actionNew.triggered.connect(new)

    def quit():
      if self.dirty():
        self.quit_dialog.show()
      else:
        sys.exit(0)
    self.actionQuit.triggered.connect(quit)

    ########
    # Pens #
    ########

    self.stroke_pen = QtGui.QPen(QtCore.Qt.SolidLine)
    self.stroke_pen.setColor(QtCore.Qt.black)
    self.stroke_pen.setWidth(1)

    self.canvases = []
    self._add_canvas()
    self.setCentralWidget(self.canvases[0])
    self.setWindowTitle('Collector')
    self.show()

  ## Determines whether any of the canvases in this Collector are dirty.
  # Dirty canvases are those with unsaved changes.
  #
  # @returns (bool) Whether any canvas is dirty.
  def dirty(self):
    for canvas in self.canvases:
      if canvas.dirty():
        return True
    return False

  ## Gets all the strokes in all the canvases in this collector.
  #
  # @returns (list) The list of strokes.
  def get_strokes(self):
    return [s for c in self.canvases for s in c.get_strokes()]

  ## Clears all the canvases and sets the strokes as the contents of a newly
  # created only canvas.
  #
  # @param strokes (list): The list of strokes to set as the contents of a new,
  #                        sole canvas.
  def set_strokes(self, strokes):
    self._clear_strokes()
    self._add_canvas()
    self.canvases[0].set_strokes(strokes)

  ## Saves all the strokes into separate files in the passed directory.
  #
  # @param dname (str) The name of the directory to save to.
  def save_dir_selected(self, dname):
    if not os.path.exists(dname):
      print 'making dirs:', dname
      os.makedirs(dname)
    elif os.path.isfile(dname):
      popup("File exists!  Choose another location.")
      self.save_file_dialog.show()
      return

    for i, canvas in enumerate(self.canvases):
      for j, stroke in enumerate(canvas.get_strokes()):
        fname = self._make_fname(dname, i, j)
        print 'saving to:', fname
        stroke.save(fname)
      canvas.is_dirty = False

  ## Clears the canvas and loads the strokes in the passed directory into the
  # appropriate canvases.
  #
  # @param dname (str) The name of the directory to load from.
  def load_dir_selected(self, dname):
    if not os.path.exists(dname):
      popup(self, "No such directory!")
      return

    if not os.path.isdir(dname):
      popup(self, "Please select a directory, not a file!")
      return

    dname, dirs, files = os.walk(dname).next()
    if len(files) <= 0:
      popup(self, "Directory is empty!")
      return

    fdata = [(os.path.join(dname, fname), int(e[1]), int(e[2][:3]))
             for e in (fname.split('-') for fname in files)
             if len(e) == 3]

    if len(fdata) <= 0:
      popup(self, "Directory is malformatted; no strokes found!")
      return

    self._clear_strokes()
    for fname, i, j in fdata:
      # Ensure we have the canvas with this index.
      while len(self.canvases) <= i:
        self._add_canvas()
      self.canvases[i].add(libsr.Stroke.load(fname))

  ## Resets the collector with strokes from the directory, but does a check that
  # this isn't dirty, first.
  #
  # @param dname (str) The name of the directory to read stroke files from.
  def check_reset_scene(self, dname):
    if self.dirty():
      # save / discard / cancel dialog
      self.save_file_dialog.show()
    self._clear_strokes()
    self._add_canvas()
    self.canvases[0].set_strokes(strokes)


  # ---- Internal Methods ----

  ## Adds a canvas to this tool.
  def _add_canvas(self):
    self.canvases.append(Canvas(self.stroke_pen))

  ## Clears all strokes in all canvases without doing any checks.
  def _clear_strokes(self):
    for c in self.canvases:
      c.clear()

  ## Handles the results of the new dialog.
  #
  # @param dialog (QWidget) The widget that contains the button.
  # @param button (QButton) The button clicked.
  #
  # @raises Exception If there's an unrecognized button.
  def _handle_new_dialog_click(self, dialog, button):
    if button == dialog.b_save:
      def reset():
        self.save_file_dialog.fileSelected.disconnect(reset)
        self._clear_strokes()
      self.save_file_dialog.fileSelected.connect(reset)
      self.save_file_dialog.show()
    elif button == dialog.b_destroy:
      self._clear_strokes()
    elif button != dialog.b_cancel:
      raise Exception('Unrecognized button:' + button.text())

  ## Handles the results of the quit dialog.
  #
  # @param dialog (SaveDestroyCancelBox) The dialog.
  # @param button (QButton) The button clicked.
  #
  # @raises Exception If there's an unrecognized button.
  def _handle_quit_dialog_click(self, dialog, button):
    if button == dialog.b_save:
      self.save_file_dialog.fileSelected.connect(lambda: sys.exit(0))
      self.save_file_dialog.show()
    elif button == dialog.b_destroy:
      sys.exit(0)
    elif button != dialog.b_cancel:
      raise Exception('Unrecognized button:' + button.text())


  # ---- Class Methods ----

  ## Generates the file name of the `j`th stroke in the `i`th canvas.
  #
  # @param dname (str) Directory name.
  # @param i (int) Index of canvas.
  # @param j (int) Index of stroke within canvas.
  #
  # @returns (str) The constructed name of the directory.
  @classmethod
  def _make_fname(cls, dname, i, j):
    return os.path.join(dname, "stroke-%02d-%03d.sr" % (i, j))


## Canvas to collect and display strokes.
class Canvas(QtWidgets.QGraphicsView):
  ## Initializes the canvas.
  #
  # @param stroke_pen (QPen) The pen to use for strokes.
  # @param rect (tuple) Preferred (x, y, w, h) of this canvas.
  def __init__(self, stroke_pen, rect=(0, 0, 800, 600)):
    super(Canvas, self).__init__(StrokeScene())
    self.scene().setSceneRect(*rect)

    self.is_dirty = False
    self.stroke_pen = stroke_pen

    # Generate pen for background drawing.
    self.bg_pen = QtGui.QPen(QtCore.Qt.SolidLine)
    self.bg_pen.setColor(QtCore.Qt.black)
    self.bg_pen.setWidth(2)

  ## Adds a stroke to this canvas.
  #
  # @param stroke (libsr.Stroke) The stroke to add.
  def add(self, stroke):
    self.scene().addItem(StrokeGraphiscItem(self.stroke_pen, 0, 0))
    self.scene().items()[0].set_stroke(stroke)

  ## Clears all the strokes in this canvas, very destructively.
  def clear(self):
    for sgi in self.scene().items()[:]:
      print 'removing:', sgi
      self.scene().removeItem(sgi)

  ## Returns the current _dirty_ state.
  # The canvas is considered _dirty_ when it contains unsaved changes.  Changes
  # include both additions and deletions.
  #
  # @return The dirty state.
  def dirty(self):
    return self.is_dirty

  ## Gets all the strokes in this canvas.
  #
  # @return (list) The list of strokes.
  def get_strokes(self):
    return [sgi.stroke for sgi in self.scene().items()
                       if isinstance(sgi, StrokeGraphiscItem)]

  ## Sets the strokes in this canvas to `strokes`.
  #
  # @param strokes (list(libsr.Stroke)) The strokes to set this to.
  def set_strokes(self, strokes):
    self.is_dirty = False
    for stroke in strokes:
      self.add(stroke)


  # ---- Internal Methods ----

  ## Append the position to the most-recently drawn stroke.
  #
  # @param x (float) X-position of mouse event.
  # @param y (float) Y-position of mouse event.
  def _append_pos(self, x, y):
    self.is_dirty = True
    rect = self.scene().items()[0].add(x, y)


  # ---- Qt Event Handlers ----

  ## See `QGraphicsView.mouseResizeEvent`.
  def resizeEvent(self, e):
    self.scene().setSceneRect(0, 0, e.size().width(), e.size().height())

  ## See `QGraphicsView.mousePressEvent`.
  def mousePressEvent(self, e):
    self.scene().addItem(StrokeGraphiscItem(self.stroke_pen, e.x(), e.y()))

  ## See `QGraphicsView.mouseMoveEvent`.
  def mouseMoveEvent(self, e):
    self._append_pos(e.x(), e.y())

  ## See `QGraphicsView.mouseReleaseEvent`.
  def mouseReleaseEvent(self, e):
    self._append_pos(e.x(), e.y())

  ## Draws a border.
  # @see QWidget.drawBackground
  def drawBackground(self, painter, rect):
    painter.setPen(self.bg_pen)
    painter.drawRect(0, 0, self.scene().width() - 1, self.scene().height() - 1)


## A graphics scene that holds a bunch of strokes.
class StrokeScene(QtWidgets.QGraphicsScene):
  def __init__(self):
    super(QtWidgets.QGraphicsScene, self).__init__()


## Draws an underlying libsr.Stroke object.
class StrokeGraphiscItem(QtWidgets.QGraphicsItem):
  DRAW_BUF = 5   # Small buffer to get some ... well ... buffer around rects.

  ## Creates a new graphics item (and underlying stroke) starting at (x, y).
  #
  # @param pen (QtGui.QPen) The pen to use to draw.
  # @param x (float) X-position of mouse event.
  # @param y (float) Y-position of mouse event.
  def __init__(self, pen, x, y):
    super(StrokeGraphiscItem, self).__init__()
    self.stroke = libsr.Stroke()
    self._add_scene_point(x, y)
    self.pen = pen

  ## Adds a scene point to the underlying libsr.Stroke object.
  #
  # @param x (float) X-coordinate of mouse event.
  # @param y (float) Y-coordinate of mouse event.
  def _add_scene_point(self, x, y):
    self.stroke.add(int(x), int(y))
    self.prepareGeometryChange()

  ## Adds a the stroke surrounded by a new StrokeGraphiscItem.
  #
  # @param stroke (libsr.Stroke) The stroke to add.
  def set_stroke(self, stroke):
    self.stroke = stroke
    self.prepareGeometryChange()

  ## Adds a new point to the underlying strokes.
  # Also registers that this has added a new point, and thus needs to be
  # redrawn.
  #
  # @param x (float) X-coordinate of mouse event.
  # @param y (float) Y-coordinate of mouse event.
  def add(self, x, y):
    pt = self.mapToScene(x, y)
    self._add_scene_point(int(pt.x()), int(pt.y()))

  ## The rectangle bounding all the points in the underlying stroke.
  #
  # @return The rectangle containing the underlying stroke.
  def boundingRect(self):
    return QtCore.QRectF(*self.stroke.bbox)

  ## See QGraphicsItem.paint()
  def paint(self, painter, option, widget):
    painter.setPen(self.pen)
    painter.setRenderHint(QtGui.QPainter.Antialiasing)
    pp = QtGui.QPainterPath()
    pp.moveTo(self.stroke[0].x, self.stroke[0].y)
    for pt in self.stroke[1:]:
      pp.lineTo(pt.x, pt.y)
    painter.drawPath(pp)


if __name__ == '__main__':
  # Ideally, we could put this in, e.g., a `run()` method.  Unfortunately,
  # there's some kind of bug that causes Qt to segfault (In Python??? I know,
  # right?) on exit.  So, these lines are left here; now there's no segfault.
  #
  # ... and the peasants rejoice.
  app = QtWidgets.QApplication(sys.argv)
  dir_name = os.path.join(
      os.path.dirname(os.path.realpath(__file__)), 'collector.ui')
  c = Collector(dir_name)
  sys.exit(app.exec_())
