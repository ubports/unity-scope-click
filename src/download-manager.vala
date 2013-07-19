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

[DBus (name = "com.canonical.applications.Download")]
interface Download : GLib.Object {
    public abstract uint32 totalSize () throws IOError;

    [DBus (name = "progress")]
    public abstract uint32 getProgress () throws IOError;
    public abstract GLib.HashTable<string, Variant> metadata () throws IOError;

    public abstract void setThrottle (uint32 speed) throws IOError;
    public abstract uint32 throttle () throws IOError;

    [DBus (name = "start")]
    public abstract void start () throws IOError;
    public abstract void pause () throws IOError;
    public abstract void resume () throws IOError;
    public abstract void cancel () throws IOError;

    public signal void started (bool success);
    public signal void paused (bool success);
    public signal void resumed (bool success);
    public signal void canceled (bool success);

    public signal void finished (string path);
    public signal void error (string error);
    public signal void progress (uint32 received, uint32 total);
}

[DBus (name = "com.canonical.applications.DownloaderManager")]
interface DownloaderManager : GLib.Object {
    public abstract GLib.ObjectPath createDownload (
        string url,
        GLib.HashTable<string, Variant> metadata,
        GLib.HashTable<string, Variant> headers
    ) throws IOError;

    public abstract GLib.ObjectPath createDownloadWithHash (
        string url,
        string algorithm,
        string hash,
        GLib.HashTable<string, Variant> metadata,
        GLib.HashTable<string, Variant> headers
    ) throws IOError;

    public abstract GLib.ObjectPath[] getAllDownloads () throws IOError;
    public abstract GLib.ObjectPath[] getAllDownloadsWithMetadata (string name, string value) throws IOError;

    public abstract void setDefaultThrottle (uint32 speed) throws IOError;
    public abstract uint32 defaultThrottle () throws IOError;

    public signal void downloadCreated (GLib.ObjectPath path);
}

DownloaderManager get_downloader ()
{
    try {
        return Bus.get_proxy_sync (BusType.SESSION, "com.canonical.applications.Downloader",
            "/", DBusProxyFlags.DO_NOT_AUTO_START);
    } catch (IOError e) {
        error ("Can't connect to DBus: %s", e.message);
    }
}

Download get_download (GLib.ObjectPath object_path)
{
    try {
        return Bus.get_proxy_sync (BusType.SESSION, "com.canonical.applications.Downloader",
            object_path, DBusProxyFlags.DO_NOT_AUTO_START);
    } catch (IOError e) {
        error ("Can't connect to DBus: %s", e.message);
    }
}
