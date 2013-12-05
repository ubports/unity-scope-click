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
	test-click-scope

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

test_click_scope_CPPFLAGS = \
  -DDATADIR=\"$(DATADIR)\" \
  -DPKGDATADIR=\"$(PKGDATADIR)\" \
  -DG_LOG_DOMAIN=\"unity-scope-click\" \
  $(SCOPE_DAEMON_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(srcdir) \
  -g

test_click_scope_VALAFLAGS = \
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

test_click_scope_LDADD = \
  $(SCOPE_DAEMON_LIBS) \
  $(NULL)

test_click_scope_VALASOURCES = \
  test-click-webservice.vala \
  click-scope.vala \
  click-webservice.vala \
  fake-data.vala \
  download-manager.vala \
  click-interface.vala \
  ubuntuone-credentials.vala \
  $(NULL)

test_click_scope_SOURCES = \
  $(test_click_scope_VALASOURCES:.vala=.c) \
  $(NULL)

BUILT_SOURCES += \
  click_scope.vala.stamp \
  test_click_scope.vala.stamp

EXTRA_DIST += \
  click_scope.vala.stamp \
  $(click_scope_VALASOURCES) \
  $(test_click_scope_VALASOURCES) \
  tdb.vapi \
  tdb.deps \
  $(NULL)

click_scope.vala.stamp: $(click_scope_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(click_scope_VALAFLAGS) $^
	touch click_scope.vala.stamp

test_click_scope.vala.stamp: $(test_click_scope_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(test_click_scope_VALAFLAGS) $^
	touch test_click_scope.vala.stamp

check-local: $(noinst_PROGRAMS)
	for program in $(noinst_PROGRAMS); do \
		$(builddir)/$$program; \
	done

CLEANFILES += \
  *.stamp \
  $(click_scope_VALASOURCES:.vala=.c) \
  $(test_click_scope_VALASOURCES:.vala=.c) \
  $(NULL)
