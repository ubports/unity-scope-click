NULL =
BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =

DATADIR = $(datadir)
LIBEXECDIR = $(libexecdir)
LIBDIR = $(libdir)

# we're using Vala, C warnings are useless
AM_CFLAGS = -w
AM_CPPFLAGS = $(COVERAGE_CFLAGS)
AM_LDFLAGS = $(COVERAGE_LDFLAGS) -R $(PROTOCOLPRIVATELIBDIR)

pkglibexec_PROGRAMS = \
  click-scope

noinst_PROGRAMS = \
  click-scope-tests

click_scope_CPPFLAGS = \
  -DDATADIR=\"$(DATADIR)\" \
  -DPKGDATADIR=\"$(PKGDATADIR)\" \
  -DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
  -DG_LOG_DOMAIN=\"unity-scope-click\" \
  $(SCOPE_DAEMON_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(srcdir) \
  -g

click_scope_VALAFLAGS = \
  -C \
  --pkg unity \
  --pkg unity-protocol \
  --pkg gee-1.0 \
  --pkg gio-unix-2.0 \
  --pkg json-glib-1.0 \
  --pkg libaccounts-glib \
  --pkg libsoup-2.4 \
  --pkg oauth \
  --pkg signon \
  --pkg libnm-glib \
  --vapidir $(srcdir) \
  $(MAINTAINER_VALAFLAGS)

click_scope_LDADD = \
  $(SCOPE_DAEMON_LIBS) \
  $(NULL)

click_scope_VALASOURCES = \
  config.vala \
  click-scope.vala \
  click-webservice.vala \
  fake-data.vala \
  download-manager.vala \
  click-interface.vala \
  ubuntuone-credentials.vala \
  non-click-scope.vala \
  utils.vala \
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
  --save-temps \
  --pkg unity \
  --pkg gee-1.0 \
  --pkg gio-unix-2.0 \
  --pkg json-glib-1.0 \
  --pkg libaccounts-glib \
  --pkg libsoup-2.4 \
  --pkg oauth \
  --pkg signon \
  --pkg libnm-glib \
  --vapidir $(srcdir) \
  $(srcdir)/assertions.vapi \
  $(MAINTAINER_VALAFLAGS)

click_scope_tests_LDADD = \
  $(SCOPE_DAEMON_LIBS) \
  $(NULL)

click_scope_tests_VALASOURCES = \
  test-click-webservice.vala \
  click-webservice.vala \
  fake-data.vala \
  download-manager.vala \
  click-interface.vala \
  ubuntuone-credentials.vala \
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
