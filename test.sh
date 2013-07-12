pkill -9 clickpackages
sleep 1
G_MESSAGES_DEBUG=all ./clickpackages-scope &
libunity-tool -n com.canonical.Unity.Scope.Applications.ClickPackages -p /com/canonical/unity/scope/applications/click_packages -q sample -r -g
