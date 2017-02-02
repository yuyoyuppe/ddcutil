AM_CPPFLAGS = \
$(GLIB_CFLAGS) \
-I$(top_srcdir)/src



AM_CFLAGS = -Wall 
AM_CFLAGS += -Werror

if ENABLE_CALLGRAPH_COND
AM_CFLAGS += -fdump-rtl-expand
endif


CLEANFILES = \
*expand

clean-local:
	@echo "(src/util/Makefile) clean-local"

mostlyclean-local:
	@echo "(src/util/Makefile) mostlyclean-local"


distclean-local:
	@echo "(src/util/Makefile) distclean-local"

# Intermediate Libraries

noinst_LTLIBRARIES = libutil.la

libutil_la_SOURCES = \
data_structures.c          \
debug_util.c               \
device_id_util.c           \
edid.c                     \
file_util.c                \
glib_util.c                \
multi_level_map.c          \
output_sink.c              \
report_util.c              \
string_util.c              \
subprocess_util.c          \
x11_util.c                 

if ENABLE_FAILSIM_COND
libutil_la_SOURCES += failsim.c
endif

if ENABLE_USB_COND
libutil_la_SOURCES += udev_util.c                
endif

dist-hook:
	@echo "(src/util/Makefile) dist-hook"
