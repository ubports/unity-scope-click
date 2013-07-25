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

    public async Gee.ArrayList<string> get_installed () {
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
        }
        return result;
    }

    string? find_dot_desktop (string folder) {
        Dir dir = Dir.open (folder, 0);
        string name;

        while ((name = dir.read_name ()) != null) {
            if (name.has_suffix (".desktop")) {
                return folder + "/" + name;
            }
        }
        return null;
    }

    const string ARG_DESKTOP_FILE_HINT = "--desktop_file_hint";
    // const string[] EXTRA_ARGS = "--stage_hint=main_stage"

    public async void execute (string app_id) {
        var click_folder = CLICK_ROOT + "/" + app_id + "/current/";
        var dotdesktop_filename = find_dot_desktop (click_folder);

        if (dotdesktop_filename == null) {
            debug ("Cannot find *.desktop in %s", click_folder);
        }

        var parsed_dotdesktop = new GLib.KeyFile ();
        parsed_dotdesktop.load_from_file (dotdesktop_filename, KeyFileFlags.NONE);
        var exec = parsed_dotdesktop.get_string ("Desktop Entry", "Exec");
        debug ( "Exec line: %s", exec );

        string[] parsed_args;
        Shell.parse_argv (exec, out parsed_args);

        var args = new Gee.ArrayList<string> ();
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
