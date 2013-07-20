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

class SignedDownload : GLib.Object {
    const string CLICK_TOKEN_HEADER = "X-Click-Token";

    HashTable<string, string> credentials;
    internal Soup.SessionAsync http_session;

    construct {
        http_session = build_http_session ();
    }

    internal Soup.SessionAsync build_http_session () {
        var session = new Soup.SessionAsync ();
        session.user_agent = "%s/%s (libsoup)".printf("UnityScopeClick", "0.1");
        return session;
    }

    public SignedDownload (HashTable<string, string> credentials) {
        this.credentials = credentials;
    }

    string sign_url (string method, string url) {
        return OAuth.sign_url2(url, null, OAuth.Method.HMAC, method,
                                credentials["consumer_key"],
                                credentials["consumer_secret"],
                                credentials["token"],
                                credentials["token_secret"]);
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
                    error ("No X-Click-Token header received from download url: %s", download_url);
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

    public async Download start_download (string uri) {
        debug ("Download started");
        var click_token = yield fetch_click_token (uri);

        var metadata = new HashTable<string, Variant> (str_hash, str_equal);
        var headers = new HashTable<string, string> (str_hash, str_equal);
        //headers[CLICK_TOKEN_HEADER] = click_token;

        /*
        debug ("got click created");
        var download_object_path = get_downloader().createDownload(uri, metadata, headers);
        debug ("Download created");

        var download = get_download (download_object_path);
        download.started.connect( () => {
            debug ("Download started");
            start_download.callback ();
        });
        debug ("Download starting");
        download.start ();
        yield;
        return download;
        */
        return null;
    }
}
