
INCLUDES = \
	-g -w \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	-D_LOG_DOMAIN=\"unity-scope-click\" \
	$(SCOPE_DAEMON_CFLAGS) \
	$(OTHERS_CFLAGS) \
	-I$(srcdir) \
	-I$(builddir)

AM_CPPFLAGS = $(COVERAGE_CFLAGS)
AM_LDFLAGS = $(COVERAGE_LDFLAGS) -R $(PROTOCOLPRIVATELIBDIR)

pkglibexec_PROGRAMS = \
  click-scope

noinst_PROGRAMS = \
	test-click-scope

noinst_LTLIBRARIES = \
	libclickscope.la

libclickscope_la_VALAFLAGS = \
	--thread \
	--vapidir $(srcdir) \
	--pkg unity-protocol \
	--pkg oauth \
	--pkg signon \
	$(SCOPE_DAEMON_VALAFLAGS) \
	-H libclickscope.h \
	--library clickscope \
	--target-glib 2.32

libclickscope_la_LIBADD = \
	$(SCOPE_DAEMON_LIBS) \
	$(OTHERS_LIBS)

libclickscope_la_SOURCES = \
	config.vala \
	click-scope.vala \
	click-webservice.vala \
	download-manager.vala \
	click-interface.vala \
	ubuntuone-credentials.vala \
	non-click-scope.vala \
	utils.vala

click_scope_VALAFLAGS = \
	--thread \
	--vapidir=$(builddir) \
	$(SCOPE_DAEMON_VALAFLAGS) \
	--pkg clickscope

click_scope_LDFLAGS = \
	$(builddir)/libclickscope.la \
	-Wl,-rpath,${libdir}/libunity

click_scope_SOURCES = \
	click-scope-main.vala

test_click_scope_VALAFLAGS = \
	--thread \
	--pkg oauth \
	--pkg signon \
	$(SCOPE_DAEMON_VALAFLAGS) \
	--vapidir=$(srcdir) \
	--vapidir=$(builddir) \
	$(srcdir)/assertions.vapi \
	--pkg clickscope

test_click_scope_LDFLAGS = \
	$(builddir)/libclickscope.la \
	$(SCOPE_DAEMON_LIBS) \
	$(OTHERS_LIBS)

test_click_scope_SOURCES = \
  test-click-webservice.vala \
  fake-data.vala

check-local: $(noinst_PROGRAMS)
	for program in $(noinst_PROGRAMS); do \
		$(builddir)/$$program; \
	done

dist-hook:
	-rm -f $(distdir)/*.c
	-rm -f $(distdir)/*.stamp

CLEANFILES = \
	*.c *.stamp \
	clickscope.vapi

MAINTAINERCLEANFILES = \
  Makefile.in
