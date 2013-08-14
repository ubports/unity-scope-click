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

const string CLICK_ROOT = "/opt/click.ubuntu.com";
const string CLICK_ROOT_ARG = "--root=" + CLICK_ROOT;

public errordomain ClickError {
    EXEC_FAILURE,
    CLICK_FAILURE
}

public class ClickInterface : GLib.Object {
    delegate void ProcessLineFunc (string line);

    void spawn (string working_dir, string[] args) throws SpawnError {
        debug ("launching %s", string.joinv(" ", args));
        Process.spawn_async (working_dir, args, Environ.get (), SpawnFlags.SEARCH_PATH, null, null);
    }

    async void spawn_readlines (string[] args, ProcessLineFunc line_func) throws SpawnError {
        Pid pid;
        int stdout_fd;

        Process.spawn_async_with_pipes ("/", args, Environ.get (),
            SpawnFlags.SEARCH_PATH | SpawnFlags.DO_NOT_REAP_CHILD,
            null, out pid, null, out stdout_fd, null);

        IOChannel output = new IOChannel.unix_new (stdout_fd);
        output.add_watch (IOCondition.IN, (chan, condition) => {
            try {
                string line;
                chan.read_line (out line, null, null);
                line_func (line);
            } catch (IOChannelError e) {
                debug ("%s: IOChannelError: %s\n", args[0], e.message);
                return false;
            } catch (ConvertError e) {
                debug ("%s: ConvertError: %s\n", args[0], e.message);
                return false;
            }
            return true;
        });

        ChildWatch.add (pid, (pid, status) => {
            Process.close_pid (pid);
            spawn_readlines.callback ();
        });
        yield;
    }

    public async Gee.ArrayList<string> get_installed () throws ClickError {
        var result = new Gee.ArrayList<string>();

        try {
            string[] args = {"click", "list", "--all", CLICK_ROOT_ARG};

            yield spawn_readlines (args, (line) => {
                debug ("installed packages: %s", line.strip());
                var line_parts = line.strip().split("\t");
                result.add(line_parts[0]);
            });
        } catch (SpawnError e) {
            debug ("get_installed, error: %s\n", e.message);
            throw new ClickError.CLICK_FAILURE(e.message);
        }
        return result;
    }

    const string ARG_DESKTOP_FILE_HINT = "--desktop_file_hint";

    public async string get_dotdesktop (string app_id) throws ClickError {
        int stdout_fd;
        string?[] args = {"click", "list", "--manifest", null};

        try {
            Process.spawn_async_with_pipes (null, args, null, SpawnFlags.SEARCH_PATH, null, null, null, out stdout_fd, null);
        } catch (SpawnError e) {
            var msg = "Problem spawning 'click list --manifest': %s".printf(e.message);
            throw new ClickError.EXEC_FAILURE(msg);
        }
        var uis = new GLib.UnixInputStream (stdout_fd, true);
        var parser = new Json.Parser ();
        try {
            yield parser.load_from_stream_async (uis);
            var manifests = parser.get_root().get_array();
            foreach (var element in manifests.get_elements()) {
                var manifest = element.get_object();
                var pkg_name = manifest.get_string_member("name");
                if (pkg_name == app_id) {
                    var version = manifest.get_string_member("version");
                    var hooks = manifest.get_object_member("hooks");
                    foreach (var app_name in hooks.get_members()) {
                        // FIXME: "Primary app" is not defined yet, so we take the first one
                        return "%s_%s_%s.desktop".printf(pkg_name, app_name, version);
                    }
                }
            }
        } catch (GLib.Error e) {
            var msg = "Problem parsing manifest: %s".printf(e.message);
            throw new ClickError.EXEC_FAILURE(msg);
        }
        var msg = "No manifest found for app_id: %s".printf(app_id);
        throw new ClickError.EXEC_FAILURE(msg);
    }

    public async void execute (string app_id) throws ClickError {
        var dotdesktop_filename = yield get_dotdesktop (app_id);
        var parsed_dotdesktop = new GLib.KeyFile ();
        string exec;
        string working_folder;
        try {
            var dotdesktop_folder = Environment.get_user_data_dir () + "/applications/";
            var dotdesktop_fullpath = dotdesktop_folder + dotdesktop_filename;
            parsed_dotdesktop.load_from_file (dotdesktop_fullpath, KeyFileFlags.NONE);
            exec = parsed_dotdesktop.get_string ("Desktop Entry", "Exec");
            working_folder = parsed_dotdesktop.get_string ("Desktop Entry", "Path");
            debug ( "Exec line: %s", exec );
        } catch (GLib.KeyFileError e) {
            var msg = "Error using keyfile %s: %s".printf(dotdesktop_filename, e.message);
            throw new ClickError.EXEC_FAILURE (msg);
        } catch (GLib.FileError e) {
            var msg = "Error using keyfile %s: %s".printf(dotdesktop_filename, e.message);
            throw new ClickError.EXEC_FAILURE (msg);
        }

        string[] parsed_args;
        try {
            Shell.parse_argv (exec, out parsed_args);
        } catch (GLib.ShellError e) {
            var msg = "Error parsing arguments: %s".printf(e.message);
            throw new ClickError.EXEC_FAILURE (msg);
        }

        var args = new Gee.ArrayList<string?> ();
        foreach (var a in parsed_args) {
            args.add (a);
        }
        // TODO: the following are needed for the device, but not for the desktop
        // so, if DISPLAY is not set, we add the extra args.
        var environ = Environ.get ();
        var display = Environ.get_variable (environ, "DISPLAY");
        if (display == null) {
            var hint = ARG_DESKTOP_FILE_HINT + "=" + dotdesktop_filename;
            debug (hint);
            args.add (hint);
        }

        try {
            args.add (null); // spawn and joinv expect this at the end of the vala array
            spawn (working_folder, args.to_array ());
        } catch (SpawnError e) {
            debug ("spawn, error: %s\n", e.message);
        }
    }
}
