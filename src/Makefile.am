NULL =
BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =

DATADIR = $(datadir)
LIBEXECDIR = $(libexecdir)

# we're using Vala, C warnings are useless
AM_CFLAGS = -w
AM_CPPFLAGS = $(COVERAGE_CFLAGS)
AM_LDFLAGS = $(COVERAGE_LDFLAGS)

pkglibexec_PROGRAMS = \
  click-scope \
  click-scope-tests

click_scope_CPPFLAGS = \
  -DDATADIR=\"$(DATADIR)\" \
  -DPKGDATADIR=\"$(PKGDATADIR)\" \
  -DG_LOG_DOMAIN=\"unity-scope-click\" \
  $(SCOPE_DAEMON_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(srcdir) \
  -g

click_scope_VALAFLAGS = \
  -C \
  --pkg unity \
  --pkg gee-1.0 \
  --pkg json-glib-1.0 \
  --pkg libsoup-2.4 \
  $(MAINTAINER_VALAFLAGS)

click_scope_LDADD = \
  $(SCOPE_DAEMON_LIBS) \
  $(NULL)

click_scope_VALASOURCES = \
  click-scope.vala \
  click-webservice.vala \
  fake-data.vala \
  download-manager.vala \
  $(NULL)

click_scope_SOURCES = \
  $(click_scope_VALASOURCES:.vala=.c) \
  $(NULL)

click_scope_tests_CPPFLAGS = \
  -DDATADIR=\"$(DATADIR)\" \
  -DPKGDATADIR=\"$(PKGDATADIR)\" \
  -DG_LOG_DOMAIN=\"unity-scope-click\" \
  $(SCOPE_DAEMON_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(srcdir) \
  -g

click_scope_tests_VALAFLAGS = \
  -C \
  --pkg unity \
  --pkg gee-1.0 \
  --pkg json-glib-1.0 \
  --pkg libsoup-2.4 \
  $(MAINTAINER_VALAFLAGS)

click_scope_tests_LDADD = \
  $(SCOPE_DAEMON_LIBS) \
  $(NULL)

click_scope_tests_VALASOURCES = \
  test-click-webservice.vala \
  assertions.vapi \
  click-webservice.vala \
  fake-data.vala \
  download-manager.vala \
  $(NULL)

click_scope_tests_SOURCES = \
  $(click_scope_tests_VALASOURCES:.vala=.c) \
  $(NULL)

BUILT_SOURCES += \
  click_scope.vala.stamp \
  click_scope_tests.vala.stamp

EXTRA_DIST += \
  click_scope.vala.stamp \
  $(click_scope_VALASOURCES) \
  $(click_scope_tests_VALASOURCES) \
  tdb.vapi \
  tdb.deps \
  $(NULL)

click_scope.vala.stamp: $(click_scope_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(click_scope_VALAFLAGS) $^
	touch click_scope.vala.stamp

click_scope_tests.vala.stamp: $(click_scope_tests_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(click_scope_tests_VALAFLAGS) $^
	touch click_scope_tests.vala.stamp

CLEANFILES += \
  *.stamp \
  $(click_scope_VALASOURCES:.vala=.c) \
  $(click_scope_tests_VALASOURCES:.vala=.c) \
  $(NULL)
