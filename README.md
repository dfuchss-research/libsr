# `libsr`

`libsr` is a sketch recognition library.  What is sketch recognition?  Well,
it's a field of research that attempts to recognize a list of timestamped
coordinates as some number of geometric shapes.


## Building & Installing

`libsr` is still in its infancy!  Please contribute by downloading, compiling,
and installing it, then the stroke collector and sending me the resultant files.

Building `libsr` requires the following libraries (tested on some versions of
Ubuntu only).
* `autoreconf`
* `libtool`
* `libopenblas-dev`
* `liblapacke-dev`

If you'd like its tests to work, you'll also need:
* `check`

And for bindings into other languages (currently only Python):
* `swig`
* Python Qt5 bindings.

Do build its docs, you'll also need:
* `doxygen`
* `graphviz`
* `pdflatex`

To build `libsr` in a separate `build/` directory, start from the location you
checked out `libsr` and run:
```
$ mkdir build
$ cd build
$ autoreconf -i .. && ../configure && make
$ sudo make install
```

If all goes well, `/usr/local/lib/libsr.so` (and if you've installed `swig` and
Python's Qt5 bindings, `/usr/local/lib/python2.7/libsr/`) will be created.


## Contributing

Currently, I'm most in need of stroke data to test against!  To create some
stroke data, download, build, and install `libsr` as described above; make sure
to follow the instructions concerning Python Qt5: 

1. Run `tools/collector.py`
2. Draw some strokes.
3. Save the strokes to a file (with a descriptive name, please) using the File
   --> Save dialog.
4. Post them publicly, somewhere, online.
5. Create an issue at [https://github.com/jamoozy/libsr/issues/new], mentioning
   "@jamoozy" in the text of the issue, with a link to the data.

I'll download it and incorporate the data.

Thank you!
