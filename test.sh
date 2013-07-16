pkill -9 click
sleep 1
G_MESSAGES_DEBUG=all ./click-scope &
libunity-tool -n com.canonical.Unity.Scope.Applications.Click -p /com/canonical/unity/scope/applications/click -q sample -r -g
