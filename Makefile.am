AUTOMAKE_OPTIONS = foreign

include_HEADERS = g_lock_manager.h

lib_LTLIBRARIES = libg_lock_manager.la
libg_lock_manager_la_SOURCES = g_lock_manager.c
libg_lock_manager_la_CFLAGS = $(GLIB_CFLAGS)
libg_lock_manager_la_LDFLAGS = $(GLIB_LIBS) -version-info 0:0:0

test:
	py.test -s tests
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage_html
