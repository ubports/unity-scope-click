valac \
    --pkg unity \
    --pkg json-glib-1.0 \
    --pkg libsoup-2.4 \
    --pkg gee-1.0 \
    clickpackages-scope.vala clickpackages-webservice.vala download-manager.vala &&

valac \
    --save-temps \
    --pkg json-glib-1.0 \
    --pkg libsoup-2.4 \
    --pkg gee-1.0 \
    test-clickpackages-webservice.vala assertions.vapi clickpackages-webservice.vala download-manager.vala
