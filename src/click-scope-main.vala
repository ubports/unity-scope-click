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


/* The GIO File for logging to. */
static File log_file = null;

/* Method to convert the log level name to a string */
static string _level_string (LogLevelFlags level)
{
	switch (level & LogLevelFlags.LEVEL_MASK) {
	case LogLevelFlags.LEVEL_ERROR:
		return "ERROR";
	case LogLevelFlags.LEVEL_CRITICAL:
		return "CRITICAL";
	case LogLevelFlags.LEVEL_WARNING:
		return "WARNING";
	case LogLevelFlags.LEVEL_MESSAGE:
		return "MESSAGE";
	case LogLevelFlags.LEVEL_INFO:
		return "INFO";
	case LogLevelFlags.LEVEL_DEBUG:
		return "DEBUG";
	}
	return "UNKNOWN";
}

static void ClickScopeLogHandler (string ? domain,
								  LogLevelFlags level,
								  string message)
{
	Log.default_handler (domain, level, message);

    try {
        var log_stream = log_file.append_to (FileCreateFlags.NONE);

        if (log_stream != null) {
            string log_message = "[%s] - %s: %s\n".printf(
                domain, _level_string (level), message);
            log_stream.write (log_message.data);
            log_stream.flush ();
            log_stream.close ();
        }
    } catch (GLib.Error e) {
        // ignore all errors when trying to log to disk
    }
}

int main ()
{
    var scope = new ClickScope();
    var exporter = new Unity.ScopeDBusConnector (scope);
    var exporter2 = new Unity.ScopeDBusConnector (new NonClickScope ());
	var cache_dir = Environment.get_user_cache_dir ();
	if (FileUtils.test (cache_dir, FileTest.EXISTS | FileTest.IS_DIR)) {
			var log_path = Path.build_filename (cache_dir,
												"unity-scope-click.log");
			log_file = File.new_for_path (log_path);
			Log.set_handler ("unity-scope-click", LogLevelFlags.LEVEL_MASK,
							 ClickScopeLogHandler);
	}

    try {
        exporter.export ();
        exporter2.export ();
    } catch (GLib.Error e) {
        error ("Cannot export scope to DBus: %s", e.message);
    }
    Unity.ScopeDBusConnector.run ();

    return 0;
}
