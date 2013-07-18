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
  click-scope

unity_scope_click_CPPFLAGS = \
  -DDATADIR=\"$(DATADIR)\" \
  -DPKGDATADIR=\"$(PKGDATADIR)\" \
  -DG_LOG_DOMAIN=\"unity-scope-click\" \
  $(SCOPE_DAEMON_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(srcdir) \
  -g

unity_scope_click_VALAFLAGS = \
  -C \
  --pkg unity \
  --pkg gee-1.0 \
  --pkg json-glib-1.0 \
  --pkg libsoup-2.4 \
  $(MAINTAINER_VALAFLAGS)

unity_scope_click_LDADD = \
  $(SCOPE_DAEMON_LIBS) \
  $(NULL)

unity_scope_click_VALASOURCES = \
  click-scope.vala \
  click-webservice.vala \
  fake-data.vala \
  download-manager.vala
  $(NULL)

unity_scope_click_SOURCES = \
  $(unity_scope_click_VALASOURCES:.vala=.c) \
  $(NULL)

BUILT_SOURCES += \
  unity_scope_click.vala.stamp

EXTRA_DIST += \
  unity_scope_click.vala.stamp \
  $(unity_scope_click_VALASOURCES) \
  tdb.vapi \
  tdb.deps \
  $(NULL)

unity_scope_click.vala.stamp: $(unity_scope_click_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(unity_scope_click_VALAFLAGS) $^
	touch unity_scope_click.vala.stamp

CLEANFILES += \
  *.stamp \
  $(unity_scope_click_VALASOURCES:.vala=.c) \
  $(NULL)
