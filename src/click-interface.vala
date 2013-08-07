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

    string find_dot_desktop (string folder) throws ClickError {
        try {
            Dir dir = Dir.open (folder, 0);
            string name;

            while ((name = dir.read_name ()) != null) {
                if (name.has_suffix (".desktop")) {
                    return folder + "/" + name;
                }
            }
        } catch (GLib.FileError e) {
            var msg = "Error opening folder %s: %s".printf(folder, e.message);
            throw new ClickError.EXEC_FAILURE(msg);
        }
        var msg = "Cannot find *.desktop in %s".printf(folder);
        throw new ClickError.EXEC_FAILURE(msg);
    }

    const string ARG_DESKTOP_FILE_HINT = "--desktop_file_hint";
    // const string[] EXTRA_ARGS = "--stage_hint=main_stage"

    async string get_click_folder (string app_id) throws ClickError {
        string folder = null;
        try {
            string[] args = {"click", "pkgdir", app_id};

            debug ("calling: click pkgdir %s", app_id);
            yield spawn_readlines (args, (line) => {
                folder = line.strip();
                debug ("click folder found: %s", folder);
            });
            if (folder != null) {
                return folder;
            } else {
                var msg = "No installation folder found for app: %s".printf(app_id);
                throw new ClickError.EXEC_FAILURE (msg);
            }
        } catch (SpawnError e) {
            var msg = "get_click_folder, error: %s".printf(e.message);
            throw new ClickError.EXEC_FAILURE(msg);
        }
    }

    public async void execute (string app_id) throws ClickError {
        var click_folder = yield get_click_folder (app_id);
        var dotdesktop_filename = find_dot_desktop (click_folder);
        var parsed_dotdesktop = new GLib.KeyFile ();
        string exec;
        try {
            parsed_dotdesktop.load_from_file (dotdesktop_filename, KeyFileFlags.NONE);
            exec = parsed_dotdesktop.get_string ("Desktop Entry", "Exec");
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
            spawn (click_folder, args.to_array ());
        } catch (SpawnError e) {
            debug ("spawn, error: %s\n", e.message);
        }
    }
}
