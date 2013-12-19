/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

public errordomain ClickError {
    EXEC_FAILURE,
    CLICK_FAILURE
}

public class ClickInterface : GLib.Object {
    const string ARG_DESKTOP_FILE_HINT = "--desktop_file_hint";
    private static string arch = null;

    public static string get_arch () {
        if (arch == null) {
            try {
                string standard_output;
                Process.spawn_command_line_sync ("dpkg --print-architecture", out standard_output);
                arch = standard_output.strip();
            } catch (SpawnError e) {
                error("Error starting dpkg: %s", e.message);
            }
        }
        return arch;
    }

    public string get_click_id (string full_app_id) {
        return full_app_id.split("_")[0];
    }

    public async Gee.ArrayList<App> get_installed (string search_query) throws ClickError {
        var result = new Gee.ArrayList<App>();
        var mgr = Unity.AppInfoManager.get_default();

        foreach (var appinfo in GLib.AppInfo.get_all()) {
            var id = appinfo.get_id();
            var path = mgr.get_path(id);

            var dotdesktop = new GLib.KeyFile ();
            try {
                dotdesktop.load_from_file (path, KeyFileFlags.NONE);
                if (dotdesktop.has_key("Desktop Entry", "X-Ubuntu-Application-ID") ) {
                    var full_app_id = dotdesktop.get_string ("Desktop Entry", "X-Ubuntu-Application-ID");
                    debug ("installed apps: %s (%s) - %s", appinfo.get_name(), full_app_id, path);
                    var app = new App();
                    // application name *must* be in path part of URL as host part
                    // might get lowercased
                    app.uri = "application:///" + id;
                    app.title = appinfo.get_name();
                    app.app_id = get_click_id(full_app_id);
                    app.icon_url = dotdesktop.get_string ("Desktop Entry", "Icon");
                    result.add (app);
                }
            } catch (GLib.Error e) {
                // Any error here means just that the icon for the app is not shown
                warning ("Problem reading .desktop for %s: %s", id, e.message);
            }
        }
        return result;
    }

    public virtual async List<unowned Json.Node> get_manifests () throws ClickError {
        int stdout_fd;
        string?[] args = {"click", "list", "--manifest", null};

        try {
            Process.spawn_async_with_pipes (null, args, null, SpawnFlags.SEARCH_PATH, null, null, null, out stdout_fd, null);
        } catch (SpawnError e) {
            var msg = "Problem running 'click list --manifest': %s".printf(e.message);
            throw new ClickError.EXEC_FAILURE(msg);
        }

        try {
            var uis = new GLib.UnixInputStream (stdout_fd, true);
            var parser = new Json.Parser ();
            yield parser.load_from_stream_async (uis);
            return parser.get_root().get_array().get_elements();

        } catch (GLib.Error e) {
            var msg = "Problem parsing manifest: %s".printf(e.message);
            throw new ClickError.EXEC_FAILURE(msg);
        }
    }

    public async Gee.Map<string, string> get_versions () throws ClickError {
        var versions = new Gee.HashMap<string, string>();
        var manifests = yield get_manifests ();
        foreach (var element in manifests) {
            var manifest = element.get_object();
            var package_name = manifest.get_string_member("name");
            var package_version = manifest.get_string_member("version");
            versions[package_name] = package_version;
        }
        return versions;
    }

    public virtual async string get_dotdesktop (string app_id) throws ClickError {
        foreach (var element in yield get_manifests()) {
            var manifest = element.get_object();
            var pkg_name = manifest.get_string_member("name");
            if (pkg_name == app_id) {
                var version = manifest.get_string_member("version");
                var hooks = manifest.get_object_member("hooks");
                foreach (var app_name in hooks.get_members()) {
                    // FIXME: "Primary app" is not defined yet, so we take the first one
                    var ddstr = "%s_%s_%s.desktop".printf(pkg_name, app_name, version);
                    debug ("get_dotdesktop: using first of %ud hooks members, returning %s",
                           hooks.get_members().length(), ddstr);
                    return ddstr;
                }
            }
        }
        var msg = "No manifest found for app_id: %s".printf(app_id);
        throw new ClickError.EXEC_FAILURE(msg);
    }

    public virtual async bool can_uninstall (string app_id) {
        const string REMOVABLE_FIELD = "_removable";
        GLib.List<weak Json.Node> manifests;
        try {
            manifests = yield get_manifests();
        } catch (ClickError e) {
            debug ("Can't get _removable for %s, ignoring", app_id);
            return true;
        }
        foreach (var element in manifests) {
            var manifest = element.get_object();
            var pkg_name = manifest.get_string_member("name");
            if (pkg_name == app_id) {
                if (manifest.has_member(REMOVABLE_FIELD)) {
                    var removable = manifest.get_int_member(REMOVABLE_FIELD);
                    return removable != 0;
                } else {
                    return true;
                }
            }
        }
        return true;
    }

    public async void uninstall (string app_id) throws ClickError {
        string version = null;
        try {
            var versions = yield get_versions();
            version = versions[app_id];
        } catch (GLib.Error e) {
            throw new ClickError.CLICK_FAILURE(e.message);
        }

        var packagekit_id = "%s;%s;all;local:click".printf(app_id, version);
        debug ("Uninstalling package: %s", packagekit_id);
        string?[] args = {"pkcon", "-p", "remove", packagekit_id, null};

        try {
            Pid child_pid;
            int exit_status = -1;
            Process.spawn_async (null, args, null, SpawnFlags.SEARCH_PATH | SpawnFlags.DO_NOT_REAP_CHILD,
                                 null, out child_pid);
            ChildWatch.add (child_pid, (pid, status) => {
                exit_status = status;
                Process.close_pid (pid);
                uninstall.callback ();
            });
            yield;
            Process.check_exit_status(exit_status);
            debug ("uninstall successful.");
        } catch (Error e) {
            var msg = "Problem running: pkcon -p remove %s (%s).".printf(packagekit_id, e.message);
            throw new ClickError.EXEC_FAILURE(msg);
        }
    }
}
