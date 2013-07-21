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
const string[] DOWNLOAD_CMDLINE = {"click", "install", "--root=/tmp/fake_root", "--force-missing-framework", "$file"};

[DBus (name = "com.canonical.applications.Download")]
interface Download : GLib.Object {
    public abstract uint64 totalSize () throws IOError;

    [DBus (name = "progress")]
    public abstract uint64 getProgress () throws IOError;
    public abstract GLib.HashTable<string, Variant> metadata () throws IOError;

    public abstract void setThrottle (uint64 speed) throws IOError;
    public abstract uint64 throttle () throws IOError;

    [DBus (name = "start")]
    public abstract void start () throws IOError;
    [DBus (name = "pause")]
    public abstract void pause () throws IOError;
    [DBus (name = "resume")]
    public abstract void resume () throws IOError;
    [DBus (name = "cancel")]
    public abstract void cancel () throws IOError;

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

[DBus (name = "com.canonical.applications.DownloaderManager")]
interface DownloaderManager : GLib.Object {
    public abstract GLib.ObjectPath createDownload (
        string url,
        GLib.HashTable<string, Variant> metadata,
        GLib.HashTable<string, string> headers
    ) throws IOError;

    public abstract GLib.ObjectPath createDownloadWithHash (
        string url,
        string algorithm,
        string hash,
        GLib.HashTable<string, Variant> metadata,
        GLib.HashTable<string, string> headers
    ) throws IOError;

    public abstract GLib.ObjectPath[] getAllDownloads () throws IOError;
    public abstract GLib.ObjectPath[] getAllDownloadsWithMetadata (string name, string value) throws IOError;

    public abstract void setDefaultThrottle (uint64 speed) throws IOError;
    public abstract uint64 defaultThrottle () throws IOError;

    [DBus (name = "downloadCreated")]
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

public string? get_download_progress (string app_id) {
    var downloads = get_downloader().getAllDownloadsWithMetadata (DOWNLOAD_APP_ID_KEY, app_id);
    if (downloads.length > 0) {
        return downloads[0];
    } else {
        return null;
    }
}

class SignedDownload : GLib.Object {
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

    async string fetch_click_token(string download_url) {
        string click_token = null;
        const string HEAD = "HEAD";

        var message = new Soup.Message (HEAD, sign_url(HEAD, download_url));
        http_session.queue_message (message, (session, message) => {
            click_token = message.response_headers[CLICK_TOKEN_HEADER];
            if (message.status_code == Soup.KnownStatusCode.OK && click_token != null) {
                debug ("Click token: %s", click_token);
            } else {
                if (click_token == null) {
                    debug ("No X-Click-Token header received from download url: %s", download_url);
                    click_token = "fake token";
                } else {
                    debug ("Web request failed: HTTP %u %s - %s",
                           message.status_code, message.reason_phrase, download_url);
                    click_token = "fake token";
                }
                //error = new PurchaseError.PURCHASE_ERROR (message.reason_phrase);
            }
            fetch_click_token.callback ();
        });
        yield;
        return click_token;
    }

    public async GLib.ObjectPath start_download (string uri, string app_id) {
        debug ("Starting download");

        // TODO: remove fake
        uri = "http://alecu.com.ar/test/click/demo.php";
        // TODO: remove fake ^^^^^^^^^^^^

        var click_token = yield fetch_click_token (uri);

        var metadata = new HashTable<string, Variant> (str_hash, str_equal);
        metadata[DOWNLOAD_COMMAND_KEY] = DOWNLOAD_CMDLINE;
        metadata[DOWNLOAD_APP_ID_KEY] = app_id;
        var headers = new HashTable<string, string> (str_hash, str_equal);
        headers[CLICK_TOKEN_HEADER] = click_token;

        var download_object_path = get_downloader().createDownload(uri, metadata, headers);
        debug ("Download created, path: %s", download_object_path);

        var download = get_download (download_object_path);
        download.start ();
        debug ("Download started");

        return download_object_path;
    }
}
