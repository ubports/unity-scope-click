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

class ClickInterface : GLib.Object {
    delegate void ProcessLineFunc (string line);

    async void spawn (string[] args, ProcessLineFunc line_func) throws SpawnError {
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
            spawn.callback ();
        });
        yield;
    }

    public async Gee.ArrayList<string> get_installed () {
        var result = new Gee.ArrayList<string>();

        try {
            string[] args = {"click", "list", "--root=/tmp/fake_root"};

            yield spawn (args, (line) => {
                debug ("installed packages: %s", line.strip());
                var line_parts = line.strip().split("\t");
                result.add(line_parts[0]);
            });
        } catch (SpawnError e) {
            debug ("get_installed, error: %s\n", e.message);
        }
        return result;
    }
}
