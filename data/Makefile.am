dbus_servicesdir = $(DBUSSERVICEDIR)
service_in_files =  \
  unity-scope-click.service.in

dbus_services_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s|\@pkglibexecdir\@|$(pkglibexecdir)|" $< > $@

EXTRA_DIST = \
  $(service_in_files)

CLEANFILES = \
  unity-scope-click.service
