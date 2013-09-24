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

private const string ACTION_INSTALL_CLICK = "install_click";
private const string ACTION_DOWNLOAD_COMPLETED = "download_completed";
private const string ACTION_OPEN_CLICK = "open_click";
private const string ACTION_PIN_TO_LAUNCHER = "pin_to_launcher";
private const string ACTION_UNINSTALL_CLICK = "uninstall_click";
private const string ACTION_CLOSE_PREVIEW = "close_preview";

public const string METADATA_APP_ID = "app_id";
public const string METADATA_TITLE = "title";
public const string METADATA_PRICE = "price";

private const int CATEGORY_INSTALLED = 0;
private const int CATEGORY_SUGGESTIONS = 1;
private const int CATEGORY_UPDATES = 2;

errordomain ClickScopeError {
    INSTALL_ERROR
}

class ClickPreviewer: Unity.ResultPreviewer
{
    ClickScope scope;

    public ClickPreviewer (ClickScope scope)
    {
        this.scope = scope;
    }

    public override Unity.AbstractPreview? run () {
        return null;
    }

    public override void run_async (Unity.AbstractPreviewCallback async_callback) {
        scope.build_default_preview.begin(result, (obj, res) => {
            var preview = scope.build_default_preview.end(res);
            async_callback (this, preview);
        });
    }
}

class ClickScope: Unity.AbstractScope
{
  const string HINT_SCREENSHOTS = "more-screenshots";
  const string HINT_KEYWORDS = "keywords";
  const string HINT_RATING = "rating";
  const string HINT_RATED = "rated";
  const string HINT_REVIEWS = "reviews";
  const string HINT_COMMENTS = "comments";

  const string LABEL_SCREENSHOTS = "Screenshots";
  const string LABEL_KEYWORDS = "Keywords";
  const string LABEL_RATING = "Rating";
  const string LABEL_RATED = "Rated";
  const string LABEL_REVIEWS = "Reviews";
  const string LABEL_COMMENTS = "Comments";

  public ClickScope ()
  {
  }

  public Variant fake_comments () {
    Variant comments[3] = {
      new Variant("(siss)", "gatox", 5, "Dec 28", "This is a great app!"),
      new Variant("(siss)", "mandel", 3, "Jan 29", "This is a fantastique app!"),
      new Variant("(siss)", "alecu", 1, "Jan 30", "Love the icons...")
    };
    return new Variant.array(new VariantType("(siss)"), comments);
  }

  Unity.Preview build_error_preview (string message) {
    var preview = new Unity.GenericPreview("Error", message, null);
    preview.add_action (new Unity.PreviewAction (ACTION_CLOSE_PREVIEW, ("Close"), null));
    return preview;
  }

  internal async Unity.Preview build_app_preview(string app_id) {
    var webservice = new ClickWebservice();
    try {
        // TODO: add caching of this webservice call
        var details = yield webservice.get_details(app_id);
        var icon = new FileIcon(File.new_for_uri(details.icon_url));
        var screenshot = new FileIcon(File.new_for_uri(details.main_screenshot_url));
        var preview = new Unity.ApplicationPreview (details.title, "subtitle", details.description, icon, screenshot);
        preview.license = details.license;
        preview.add_info(new Unity.InfoHint.with_variant(HINT_SCREENSHOTS, LABEL_SCREENSHOTS, null, new Variant.strv(details.more_screenshot_urls)));
        preview.add_info(new Unity.InfoHint.with_variant(HINT_KEYWORDS, LABEL_KEYWORDS, null, new Variant.strv(details.keywords)));
        preview.add_info(new Unity.InfoHint.with_variant(HINT_RATING, LABEL_RATING, null, new Variant.int32(5)));
        preview.add_info(new Unity.InfoHint.with_variant(HINT_RATED, LABEL_RATED, null, new Variant.int32(3)));
        preview.add_info(new Unity.InfoHint.with_variant(HINT_REVIEWS, LABEL_REVIEWS, null, new Variant.int32(15)));
        // TODO: get the proper reviews from the rnr webservice:
        preview.add_info(new Unity.InfoHint.with_variant(HINT_COMMENTS, LABEL_COMMENTS, null, fake_comments ()));
        // TODO: get the proper reviews from the rnr webservice ^^^^^^^^^^^^^^^^^^^^^^^^^
        return preview;
    } catch (WebserviceError e) {
        debug ("Error calling webservice: %s", e.message);
        return build_error_preview (e.message);
    }
  }

    bool uri_is_click_install (string uri) {
        return uri.has_prefix (App.CLICK_INSTALL_SCHEMA);
    }

    async void launch_application (string app_id) throws ClickError {
        var click_if = new ClickInterface ();
        yield click_if.execute(app_id);
    }

    async Unity.Preview build_uninstalled_preview (string app_id) {
        Unity.Preview preview = yield build_app_preview (app_id);
        preview.add_action (new Unity.PreviewAction (ACTION_INSTALL_CLICK, ("Install"), null));
        return preview;
    }

    async Unity.Preview build_installed_preview (string app_id) {
        Unity.Preview preview = yield build_app_preview (app_id);
        preview.add_action (new Unity.PreviewAction (ACTION_OPEN_CLICK, ("Open"), null));
        preview.add_action (new Unity.PreviewAction (ACTION_PIN_TO_LAUNCHER, ("Pin to launcher"), null));
        preview.add_action (new Unity.PreviewAction (ACTION_UNINSTALL_CLICK, ("Uninstall"), null));
        return preview;
    }

    async Unity.Preview build_installing_preview (string app_id, string progress_source) {
        Unity.Preview preview = yield build_app_preview (app_id);
        preview.add_action (new Unity.PreviewAction (ACTION_DOWNLOAD_COMPLETED, ("*** download_completed"), null));
        preview.add_info(new Unity.InfoHint.with_variant("show_progressbar", "Progressbar", null, new Variant.boolean(true)));
        preview.add_info(new Unity.InfoHint.with_variant("progressbar_source", "Progress Source", null, progress_source));
        return preview;
    }

    internal async Unity.Preview build_default_preview (Unity.ScopeResult result) {
        var app_id = result.metadata.get(METADATA_APP_ID).get_string();
        if (uri_is_click_install(result.uri)) {
            return yield build_uninstalled_preview (app_id);
        } else {
            return yield build_installed_preview (app_id);
        }
    }

    async Unity.ActivationResponse? activate_async (Unity.ScopeResult result, Unity.SearchMetadata metadata, string? action_id) {
        var app_id = result.metadata.get(METADATA_APP_ID).get_string();
        Unity.Preview preview = null;
        string next_url = null;

        try {
            debug ("action started: %s", action_id);
            if (action_id == null) {
                if (uri_is_click_install(result.uri)) {
                    preview = yield build_uninstalled_preview (app_id);
                } else {
                    yield launch_application (app_id);
                }
            } else if (action_id == ACTION_INSTALL_CLICK) {
                var progress_source = yield install_app(app_id);
                preview = yield build_installing_preview (app_id, progress_source);
            } else if (action_id == ACTION_DOWNLOAD_COMPLETED) {
                results_invalidated(Unity.SearchType.GLOBAL);
                results_invalidated(Unity.SearchType.DEFAULT);
                preview = yield build_installed_preview (app_id);
            } else if (action_id == ACTION_OPEN_CLICK) {
                yield launch_application (app_id);
            } else if (action_id == ACTION_CLOSE_PREVIEW) {
                // default is to close the dash
            } else {
                return null;
            }
        } catch (GLib.Error e) {
            debug ("Error building preview: %s", e.message);
            preview = build_error_preview (e.message);
        }

        if (preview != null) {
            return new Unity.ActivationResponse.with_preview(preview);
        } else {
            return new Unity.ActivationResponse(Unity.HandledType.HIDE_DASH, next_url);
        }
    }

    public override Unity.ActivationResponse? activate (Unity.ScopeResult result, Unity.SearchMetadata metadata, string? action_id) {
        Unity.ActivationResponse? response = null;

        MainLoop mainloop = new MainLoop ();
        activate_async.begin(result, metadata, action_id, (obj, res) => {
            response = activate_async.end(res);
            mainloop.quit ();
        });
        mainloop.run ();

        return response;
    }

    public async GLib.ObjectPath install_app (string app_id) throws ClickScopeError {
        try {
            debug ("getting details for %s", app_id);
            var click_ws = new ClickWebservice ();
            var app_details = yield click_ws.get_details (app_id);
            debug ("got details: %s", app_details.title);
            var u1creds = new UbuntuoneCredentials ();
            debug ("getting creds");
            var credentials = yield u1creds.get_credentials ();
            debug ("got creds: %s", credentials["token"]);
            var signed_download = new SignedDownload (credentials);

            var download_url = app_details.download_url;
            // TODO: this is only valid while click package downloads are unauthenticated
            download_url += "?noauth=1";

            debug ("starting download of %s from: %s", app_id, download_url);
            var download_object_path = yield signed_download.start_download (download_url, app_id);
            debug ("download started: %s", download_object_path);
            return download_object_path;
        } catch (Error e) {
            debug ("cannot install app %s: %s", app_id, e.message);
            throw new ClickScopeError.INSTALL_ERROR (e.message);
        }
    }

  public override Unity.ScopeSearchBase create_search_for_query (Unity.SearchContext ctx)
  {
    var search = new ClickSearch (this);
    search.set_search_context (ctx);
    return search;
  }

  public override Unity.ResultPreviewer create_previewer (Unity.ScopeResult result, Unity.SearchMetadata metadata)
  {
    var cpp = new ClickPreviewer(this);
    cpp.set_scope_result(result);
    cpp.set_search_metadata(metadata);
    return cpp;
  }

  public override Unity.CategorySet get_categories ()
  {
    var categories = new Unity.CategorySet ();
    var icon = new FileIcon(File.new_for_path("/usr/share/icons/unity-icon-theme/places/svg/group-treat-yourself.svg"));
    categories.add (new Unity.Category("installed", "Installed", icon, Unity.CategoryRenderer.GRID));   // CATEGORY_INSTALLED
    categories.add (new Unity.Category("more", "More suggestions", icon, Unity.CategoryRenderer.GRID)); // CATEGORY_SUGGESTIONS
    categories.add (new Unity.Category("updates", "Needing update", icon, Unity.CategoryRenderer.GRID)); // CATEGORY_UPDATES
    return categories;
  }

  public override Unity.FilterSet get_filters ()
  {
    return new Unity.FilterSet ();
  }

  public override Unity.Schema get_schema ()
  {
    return new Unity.Schema ();
  }

  public override string get_group_name ()
  {
    return "com.canonical.Unity.Scope.Applications.Click";
  }

  public override string get_unique_name ()
  {
    return "/com/canonical/unity/scope/applications/click";
  }
}

class ClickSearch: Unity.ScopeSearchBase
{
  NM.Client nm_client;
  Gee.Map<string, App> installed;
  Unity.AbstractScope parent_scope;

  public ClickSearch (Unity.AbstractScope scope) {
    nm_client = new NM.Client ();
    installed = new Gee.HashMap<string, App> ();

    parent_scope = scope;
  }

  private void add_app (App app, int category)
  {
    debug (app.title);
    var uri = app.uri;
    var comment = "";
    var dnd_uri = uri;
    var mimetype = "application/x-desktop";
    var metadata = new HashTable<string, Variant> (str_hash, str_equal);
    metadata.insert(METADATA_APP_ID, new GLib.Variant.string(app.app_id));
    metadata.insert(METADATA_TITLE, new GLib.Variant.string(app.title));
    metadata.insert(METADATA_PRICE, new GLib.Variant.string(app.price));

    var result = Unity.ScopeResult.create(uri, app.icon_url, category, Unity.ResultType.DEFAULT, mimetype, app.title, comment, dnd_uri, metadata);

    var download_progress = get_download_progress(app.app_id);
    debug ("download progress source for app %s: %s", app.app_id, download_progress);
    if (download_progress != null) {
        result.metadata.insert("show_progressbar", new Variant.boolean(true));
        result.metadata.insert("progressbar_source", new GLib.Variant.string(download_progress));
    }
    search_context.result_set.add_result (result);
  }

  async void find_installed_apps (string search_query) {
    var click_interface = new ClickInterface();
    try {
        var apps = yield click_interface.get_installed (search_query);
        foreach (var app in apps) {
            if (app.matches(search_query)) {
                add_app (app, CATEGORY_INSTALLED);
                installed[app.app_id] = app;
            }
        }
    } catch (ClickError e) {
        debug ("Error stating installed apps: %s", e.message);
        // TODO: warn about this some other way, like notifications
    }
  }

  async void find_available_apps (string search_query) {
    var main_ctx = GLib.MainContext.default();
    while (nm_client.get_state () != NM.State.UNKNOWN &&
           nm_client.get_state () != NM.State.CONNECTED_GLOBAL) {
        debug ("find_apps: Network unavailable, waiting.");
        while (main_ctx.pending())
            main_ctx.iteration(false);
        Posix.sleep(10);
    }
    try {
        var webservice = new ClickWebservice();
        var apps = yield webservice.search (search_query);
        foreach (var app in apps) {
            // do not include installed apps in suggestions
            if (!installed.has_key(app.app_id)) {
                add_app (app, CATEGORY_SUGGESTIONS);
            }
        }
        parent_scope.results_invalidated(Unity.SearchType.GLOBAL);
        parent_scope.results_invalidated(Unity.SearchType.DEFAULT);
    } catch (WebserviceError e) {
        debug ("Error calling webservice: %s", e.message);
    }
  }

  async void find_available_updates (string search_query) {
    var main_ctx = GLib.MainContext.default();
    while (nm_client.get_state () != NM.State.UNKNOWN &&
           nm_client.get_state () != NM.State.CONNECTED_GLOBAL) {
        debug ("find_updates: Network unavailable, waiting.");
        while (main_ctx.pending())
            main_ctx.iteration(false);
        Posix.sleep(10);
    }
    try {
        var webservice = new ClickWebservice();
        debug ("installed: %p (%d)", installed, installed.size);
        var apps = yield webservice.find_updates (installed);
        foreach (var app in apps) {
            add_app (app, CATEGORY_UPDATES);
        }
       parent_scope.results_invalidated(Unity.SearchType.GLOBAL);
       parent_scope.results_invalidated(Unity.SearchType.DEFAULT);
    } catch (WebserviceError e) {
        debug ("Error calling webservice: %s", e.message);
    }
  }

  async void find_apps (string search_query) {
    yield find_installed_apps (search_query);
    yield find_available_apps (search_query);
    yield find_available_updates (search_query);
  }

  public override void run ()
  {
  }

  public override void run_async (Unity.ScopeSearchBaseCallback async_callback)
  {
    find_apps.begin (search_context.search_query, (obj, res) => {
        find_apps.end (res);
        async_callback(this);
    });
  }
}


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
    } catch (GLib.Error e) {
        error ("Cannot export scope to DBus: %s", e.message);
    }
    Unity.ScopeDBusConnector.run ();

    return 0;
}
