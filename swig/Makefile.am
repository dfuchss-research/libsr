i_file = $(srcdir)/libsr.i
c_src_dir = $(srcdir)/../src
libcommon_a = $(builddir)/../src/common/.libs/libcommon.a
wrap_file = $(srcdir)/libsr_wrap.c
target = $(builddir)/_libsr.so

all : $(target)

$(wrap_file) : $(i_file)
	swig -python -outdir $(builddir) -I$(c_src_dir) $(i_file)

$(target) : $(wrap_file) $(libcommon_a)
	gcc -shared -fPIC -I$(c_src_dir) -I/usr/include/python2.7 $(CFLAGS) $(LDFLAGS) $^ -o $@

clean-local :
	rm -f $(target) $(wrap_file)