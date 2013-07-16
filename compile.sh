valac \
    --pkg unity \
    --pkg json-glib-1.0 \
    --pkg libsoup-2.4 \
    --pkg gee-1.0 \
    click-scope.vala click-webservice.vala fake-data.vala download-manager.vala &&

valac \
    --save-temps \
    --pkg json-glib-1.0 \
    --pkg libsoup-2.4 \
    --pkg gee-1.0 \
    test-click-webservice.vala assertions.vapi click-webservice.vala fake-data.vala download-manager.vala
