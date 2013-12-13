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

const string DOWNLOAD_APP_ID_KEY = "app_id";
const string DOWNLOAD_COMMAND_KEY = "post-download-command";
const string[] DOWNLOAD_CMDLINE = {"pkcon", "-p", "install-local", "$file"};

[DBus (name = "com.canonical.applications.Download")]
interface Download : GLib.Object {
    [DBus (name = "start")]
    public abstract void start () throws IOError;

    [DBus (name = "started")]
    public signal void started (bool success);
    [DBus (name = "paused")]
    public signal void paused (bool success);
    [DBus (name = "resumed")]
    public signal void resumed (bool success);
    [DBus (name = "canceled")]
    public signal void canceled (bool success);

    [DBus (name = "finished")]
    public signal void finished (string path);
    [DBus (name = "error")]
    public signal void error (string error);
    [DBus (name = "progress")]
    public signal void progress (uint64 received, uint64 total);
}

struct DownloadStruct {
    public string url;
    public string hash;
    public string hash_algorithm;
    GLib.HashTable<string, Variant> metadata;
    GLib.HashTable<string, string> headers;
}

[DBus (name = "com.canonical.applications.DownloadManager")]
interface DownloadManager : GLib.Object {
    public abstract GLib.ObjectPath createDownload (
        DownloadStruct download
    ) throws IOError;

    public abstract GLib.ObjectPath[] getAllDownloadsWithMetadata (string name, string value) throws IOError;

    [DBus (name = "downloadCreated")]
    public signal void downloadCreated (GLib.ObjectPath path);
}

DownloadManager? get_downloader () throws IOError {
    try {
        return Bus.get_proxy_sync (BusType.SESSION, "com.canonical.applications.Downloader",
            "/", DBusProxyFlags.DO_NOT_AUTO_START);
    } catch (IOError e) {
        warning ("Can't connect to DBus: %s", e.message);
        throw e;
    }
}

Download? get_download (GLib.ObjectPath object_path) throws IOError {
    try {
        return Bus.get_proxy_sync (BusType.SESSION, "com.canonical.applications.Downloader",
            object_path, DBusProxyFlags.DO_NOT_AUTO_START);
    } catch (IOError e) {
        warning ("Can't connect to DBus: %s", e.message);
        throw e;
    }
}

public errordomain DownloadError {
    DOWNLOAD_ERROR,
    INVALID_CREDENTIALS
}

public string? get_download_progress (string app_id) {
    try {
        var downloads = get_downloader().getAllDownloadsWithMetadata (DOWNLOAD_APP_ID_KEY, app_id);
        if (downloads.length > 0) {
            return downloads[0];
        } else {
            return null;
        }
    } catch (IOError e) {
        warning ("Cannot get download progress for %s, ignoring: %s", app_id, e.message);
        return null;
    }
}

public class SignedDownload : GLib.Object {
    const string CLICK_TOKEN_HEADER = "X-Click-Token";

    const string CONSUMER_KEY = "consumer_key";
    const string CONSUMER_SECRET = "consumer_secret";
    const string TOKEN = "token";
    const string TOKEN_SECRET = "token_secret";

    HashTable<string, string> credentials;
    internal Soup.SessionAsync http_session;

    construct {
        http_session = WebClient.get_webclient ();
    }

    public SignedDownload (HashTable<string, string> credentials) {
        this.credentials = credentials;
    }

    string sign_url (string method, string url) {
        return OAuth.sign_url2(url, null, OAuth.Method.HMAC, method,
            credentials[CONSUMER_KEY], credentials[CONSUMER_SECRET],
            credentials[TOKEN], credentials[TOKEN_SECRET]);
    }

    async string fetch_click_token(string download_url) throws DownloadError {
        string click_token = null;
        const string HEAD = "HEAD";
        DownloadError error = null;

        var message = new Soup.Message (HEAD, sign_url(HEAD, download_url));
        http_session.queue_message (message, (session, message) => {
            click_token = message.response_headers[CLICK_TOKEN_HEADER];
            if (message.status_code == Soup.KnownStatusCode.OK && click_token != null) {
                debug ("Click token received");
            } else {
                switch (message.status_code) {
                case Soup.KnownStatusCode.OK:
                    var msg = "No X-Click-Token header received. Url was: %s".printf(download_url);
                    debug (msg);
                    error = new DownloadError.DOWNLOAD_ERROR (msg);
                    break;
                case Soup.KnownStatusCode.UNAUTHORIZED:
                    var msg = "The Ubuntu One credentials are invalid, please log in again.";
                    debug (msg);
                    error = new DownloadError.INVALID_CREDENTIALS (msg);
                    break;
                default:
                    var msg = "Web request failed: HTTP %u %s - %s".printf(
                           message.status_code, message.reason_phrase, download_url);
                    debug (msg);
                    error = new DownloadError.DOWNLOAD_ERROR (message.reason_phrase);
                    break;
                }
            }
            fetch_click_token.callback ();
        });
        yield;
        if (error != null) {
            throw error;
        }
        return click_token;
    }

    public async GLib.ObjectPath start_download (string uri, string app_id) throws DownloadError {
        debug ("Starting download");

        var click_token = yield fetch_click_token (uri);

        var metadata = new HashTable<string, Variant> (str_hash, str_equal);
        metadata[DOWNLOAD_COMMAND_KEY] = DOWNLOAD_CMDLINE;
        metadata[DOWNLOAD_APP_ID_KEY] = app_id;
        var headers = new HashTable<string, string> (str_hash, str_equal);
        headers[CLICK_TOKEN_HEADER] = click_token;

        try {
            var download_struct = DownloadStruct(){
                url = uri,
                hash = "",  // means no hash check
                hash_algorithm = "",  // will be ignored
                metadata = metadata,
                headers = headers
            };
            var download_object_path = get_downloader().createDownload(download_struct);
            debug ("Download created, path: %s", download_object_path);

            var download = get_download (download_object_path);
            download.start ();
            debug ("Download started");

            return download_object_path;
        } catch (GLib.IOError e) {
            warning ("Cannot start download: %s", e.message);
            throw new DownloadError.DOWNLOAD_ERROR(e.message);
        }
    }
}
