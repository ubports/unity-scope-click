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
private const string ACTION_BUY_CLICK = "buy_click";
private const string ACTION_DOWNLOAD_COMPLETED = "download_completed";
private const string ACTION_DOWNLOAD_FAILED = "download_failed";
private const string ACTION_PURCHASE_SUCCEEDED = "purchase_succeeded";
private const string ACTION_PURCHASE_FAILED = "purchase_failed";
private const string ACTION_OPEN_CLICK = "open_click";
private const string ACTION_PIN_TO_LAUNCHER = "pin_to_launcher";
private const string ACTION_UNINSTALL_CLICK = "uninstall_click";
private const string ACTION_CONFIRM_UNINSTALL = "confirm_uninstall";
private const string ACTION_CLOSE_PREVIEW = "close_preview";
private const string ACTION_OPEN_ACCOUNTS = "open_accounts";

private const string ACCOUNT_SETTINGS_URL = "settings:///system/online-accounts";

public const string METADATA_APP_ID = "app_id";
public const string METADATA_TITLE = "title";
public const string METADATA_PRICE = "price";

private const int CATEGORY_INSTALLED = 0;
private const int CATEGORY_SUGGESTIONS = 1;

public errordomain ClickScopeError {
    INSTALL_ERROR,
    LOGIN_ERROR,
}

public class ClickPreviewer: Unity.ResultPreviewer
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

public class ClickScope: Unity.AbstractScope
{
  const string HINT_SCREENSHOTS = "more-screenshots";
  const string HINT_PUBLISHER = "publisher";
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

  public ClickInterface click_if = new ClickInterface ();
  public UbuntuoneCredentials u1creds = new UbuntuoneCredentials ();
  public ClickWebservice webservice = new ClickWebservice ();
  RNRClient rnrClient = new RNRClient();

  public ClickScope ()
  {
  }
  
  // Wrapper to be overridden for tests:
  protected virtual SignedDownload get_signed_download (HashTable<string, string> credentials) {
      return new SignedDownload (credentials);
  }

  Unity.Preview build_error_preview (string message) {
    var preview = new Unity.GenericPreview("Error", message, null);
    preview.add_action (new Unity.PreviewAction (ACTION_CLOSE_PREVIEW, ("Close"), null));
    return preview;
  }

  Unity.Preview build_login_error_preview (string message) {
    var preview = new Unity.GenericPreview ("Login Error", message, null);
    preview.add_action (new Unity.PreviewAction (ACTION_OPEN_ACCOUNTS, ("Go to Accounts"), null));
    return preview;
  }

  Unity.Preview build_uninstall_confirmation_preview () {
    var message = "Uninstall this app will delete all the related information. Are you sure you want to uninstall?";
    var preview = new Unity.GenericPreview("Confirmation", message, null);
    preview.add_action (new Unity.PreviewAction (ACTION_CLOSE_PREVIEW, ("Not anymore"), null));
    preview.add_action (new Unity.PreviewAction (ACTION_CONFIRM_UNINSTALL, ("Yes Uninstall"), null));
    return preview;
  }

  internal async Unity.Preview build_app_preview(Unity.ScopeResult result) {
    var app_id = result.metadata.get(METADATA_APP_ID).get_string();
    try {
        var details = yield webservice.get_details(app_id);
        var icon = new FileIcon(File.new_for_uri(details.icon_url));
        var screenshot = new FileIcon(File.new_for_uri(details.main_screenshot_url));
        var preview = new Unity.ApplicationPreview (details.title, "subtitle", details.description, icon, screenshot);
        preview.license = details.license;
        preview.add_info(new Unity.InfoHint.with_variant(HINT_PUBLISHER, "Publisher", null, new Variant.string(details.publisher)));
        preview.add_info(new Unity.InfoHint.with_variant(HINT_SCREENSHOTS, LABEL_SCREENSHOTS, null, new Variant.strv(details.more_screenshot_urls)));
        preview.add_info(new Unity.InfoHint.with_variant(HINT_KEYWORDS, LABEL_KEYWORDS, null, new Variant.strv(details.keywords)));
        // get the proper ratings and reviews from the rnr web service
        var reviews = yield rnrClient.get_reviews(details);
        if (reviews != null) {
            preview.add_info(new Unity.InfoHint.with_variant(HINT_REVIEWS, LABEL_REVIEWS, null, reviews));
            debug("Got %d reviews for %s", (int)reviews.n_children(), details.package_name);
        }
        return preview;
    } catch (WebserviceError e) {
        debug ("Error calling webservice: %s", e.message);
        return build_error_preview (e.message);
    }
  }

    public virtual bool uri_is_click_install (string uri) {
        return uri.has_prefix (App.CLICK_INSTALL_SCHEMA);
    }

    public async virtual Unity.Preview build_uninstalled_preview (Unity.ScopeResult result) {
        var price = result.metadata.get(METADATA_PRICE).get_double();
        Unity.Preview preview = yield build_app_preview (result);
        if (!(preview is Unity.GenericPreview)) {
            if (price == 0.0f) {
                preview.add_action (new Unity.PreviewAction (ACTION_INSTALL_CLICK, ("Install"), null));
            } else {
                preview.add_action (new Unity.PreviewAction (ACTION_BUY_CLICK, ("Buy for %0.2f".printf(price)), null));
            }
        }
        return preview;
    }

    public async virtual Unity.Preview build_installed_preview (Unity.ScopeResult result, string application_uri) {
        var app_id = result.metadata.get(METADATA_APP_ID).get_string();
        Unity.Preview preview = yield build_app_preview (result);
        preview.add_action (new Unity.PreviewAction (ACTION_OPEN_CLICK + ":" + application_uri, ("Open"), null));

        if (yield click_if.can_uninstall (app_id)) {
            preview.add_action (new Unity.PreviewAction (ACTION_UNINSTALL_CLICK, ("Uninstall"), null));
        }
        return preview;
    }

    public async virtual Unity.Preview build_installing_preview (Unity.ScopeResult result, string progress_source) {
        Unity.Preview preview = yield build_app_preview (result);

        // When the progressbar is shown by the preview in the dash no buttons should be shown.
        // The two following actions (marked with ***) are not shown as buttons, but instead are triggered by the dash
        // when the download manager succeeds or fails with a given download+installation.
        preview.add_action (new Unity.PreviewAction (ACTION_DOWNLOAD_COMPLETED, ("*** download_completed"), null));
        preview.add_action (new Unity.PreviewAction (ACTION_DOWNLOAD_FAILED, ("*** download_failed"), null));

        preview.add_info(new Unity.InfoHint.with_variant("show_progressbar", "Progressbar", null, new Variant.boolean(true)));
        preview.add_info(new Unity.InfoHint.with_variant("progressbar_source", "Progress Source", null, progress_source));
        return preview;
    }

    public async Unity.Preview build_purchasing_preview (Unity.ScopeResult result) {
        Unity.Preview preview = yield build_app_preview (result);
        var app_id = result.metadata.get(METADATA_APP_ID).get_string();

        // When the purchase overlay is shown by the preview in the dash no buttons should be shown.
        // The two following actions (marked with ***) are not shown as buttons, but instead are triggered by the dash
        // when the purchase service succeeds or fails.
        preview.add_action (new Unity.PreviewAction (ACTION_PURCHASE_SUCCEEDED, ("*** purchase_succeeded"), null));
        preview.add_action (new Unity.PreviewAction (ACTION_PURCHASE_FAILED, ("*** purchase_failed"), null));

        preview.add_info(new Unity.InfoHint.with_variant("show_purchase_overlay", "Show Purchase Overlay", null, new Variant.boolean(true)));
        preview.add_info(new Unity.InfoHint.with_variant("package_name", "Package Name", null, new Variant.string(app_id)));
        return preview;
    }

    public async Unity.Preview build_default_preview (Unity.ScopeResult result) {
        var app_id = result.metadata.get(METADATA_APP_ID).get_string();
        var progress_source = get_progress_source(app_id);

        if (progress_source != null) {
            return yield build_installing_preview (result, progress_source);

        } else if (uri_is_click_install(result.uri)) {
            return yield build_uninstalled_preview (result);

        } else {
            return yield build_installed_preview (result, result.uri);
        }
    }

    public virtual string? get_progress_source (string app_id) {
        return get_download_progress(app_id);
    }

    public async Unity.ActivationResponse? activate_async (Unity.ScopeResult result, Unity.SearchMetadata metadata, string? action_id) {
        var app_id = result.metadata.get(METADATA_APP_ID).get_string();
        Unity.Preview preview = null;
        string next_url = null;

        try {
            debug ("action started: %s for app_id: %s", action_id, app_id);
            if (action_id == null) {
                var progress_source = get_progress_source(app_id);
                debug ("Progress source: %s", progress_source);
                if (progress_source != null) {
                    preview = yield build_installing_preview (result, progress_source);
                } else if (uri_is_click_install(result.uri)) {
                    preview = yield build_uninstalled_preview (result);
                } else {
                    debug ("Let the dash launch the app: %s", result.uri);
                    return new Unity.ActivationResponse(Unity.HandledType.NOT_HANDLED);
                }
            } else if (action_id == ACTION_PURCHASE_FAILED){
                preview = yield build_uninstalled_preview (result);
            } else if (action_id == ACTION_BUY_CLICK) {
                preview = yield build_purchasing_preview (result);
            } else if (action_id == ACTION_INSTALL_CLICK || action_id == ACTION_PURCHASE_SUCCEEDED) {
                var progress_source = yield install_app(app_id);
                preview = yield build_installing_preview (result, progress_source);
            } else if (action_id.has_prefix(ACTION_DOWNLOAD_FAILED)) {
                // we don't have access to the error message in SearchMetadata, LP: #1233836
                var errormsg = "please check ubuntu-download-manager.log";
                preview = build_error_preview ("Installation failed: %s".printf(errormsg));
            } else if (action_id == ACTION_DOWNLOAD_COMPLETED) {
                results_invalidated(Unity.SearchType.GLOBAL);
                results_invalidated(Unity.SearchType.DEFAULT);
                var dotdesktop = yield click_if.get_dotdesktop(app_id);
                // application name *must* be in path part of URL as host part
                // might get lowercased
                var application_uri = "application:///" + dotdesktop;
                preview = yield build_installed_preview (result, application_uri);
            } else if (action_id.has_prefix(ACTION_OPEN_CLICK)) {
                var application_uri = action_id.split(":", 2)[1];
                debug ("Let the dash launch the app: %s", application_uri);
                return new Unity.ActivationResponse(Unity.HandledType.NOT_HANDLED, application_uri);
            } else if (action_id == ACTION_UNINSTALL_CLICK) {
                preview = build_uninstall_confirmation_preview();
            } else if (action_id == ACTION_CONFIRM_UNINSTALL) {
                yield click_if.uninstall(app_id);
                results_invalidated(Unity.SearchType.GLOBAL);
                results_invalidated(Unity.SearchType.DEFAULT);
                return new Unity.ActivationResponse(Unity.HandledType.SHOW_DASH);
            } else if (action_id == ACTION_CLOSE_PREVIEW) {
                // default is to close the preview
                return new Unity.ActivationResponse(Unity.HandledType.SHOW_DASH);
            } else if (action_id == ACTION_OPEN_ACCOUNTS) {
                return new Unity.ActivationResponse (Unity.HandledType.NOT_HANDLED,
                                                     ACCOUNT_SETTINGS_URL);
            } else {
                return null;
            }
        } catch (ClickScopeError scope_error) {
            if (scope_error is ClickScopeError.LOGIN_ERROR) {
                preview = build_login_error_preview (scope_error.message);
            } else {
                preview = build_error_preview (scope_error.message);
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
            var app_details = yield webservice.get_details (app_id);
            debug ("got details: %s", app_details.title);
            debug ("getting creds");
            var credentials = yield u1creds.get_credentials ();
            debug ("got creds");
            var signed_download = get_signed_download(credentials);

            var download_url = app_details.download_url;

            debug ("starting download of %s from: %s", app_id, download_url);
            var download_object_path = yield signed_download.start_download (download_url, app_id);
            debug ("download started: %s", download_object_path);
            return download_object_path;
        } catch (DownloadError download_error) {
            debug ("Got DownloadError: %s", download_error.message);
            if (download_error is DownloadError.INVALID_CREDENTIALS) {
                yield u1creds.invalidate_credentials ();
                throw new ClickScopeError.LOGIN_ERROR (download_error.message);
            } else {
                throw new ClickScopeError.INSTALL_ERROR (download_error.message);
            }
        } catch (CredentialsError cred_error) {
            debug ("Got CredentialsError trying to fetch token.");
            throw new ClickScopeError.LOGIN_ERROR (cred_error.message);
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
    categories.add (new Unity.Category("installed", "Installed", icon, Unity.CategoryRenderer.GRID));   // 0
    categories.add (new Unity.Category("more", "More suggestions", icon, Unity.CategoryRenderer.GRID)); // 1
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

// This is the timeout id for app search when network is down, or there is
// an error. We need it to be a global static to avoid parallel timeouts
// across ClickSearch instances.
static uint app_search_id = 0;

public class ClickSearch: Unity.ScopeSearchBase
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
    var uri = app.uri;
    var comment = "";
    var dnd_uri = uri;
    var mimetype = "application/x-desktop";
    var metadata = new HashTable<string, Variant> (str_hash, str_equal);
    if (app.app_id == null || app.title == null) {
        warning ("app.app_id or app.title is null");
        return;
    }
    metadata.insert(METADATA_APP_ID, new GLib.Variant.string(app.app_id));
    metadata.insert(METADATA_TITLE, new GLib.Variant.string(app.title));
    metadata.insert(METADATA_PRICE, new GLib.Variant.double(app.price));

    var result = Unity.ScopeResult.create(uri, app.icon_url, category, Unity.ResultType.DEFAULT, mimetype, app.title, comment, dnd_uri, metadata);

    var download_progress = get_download_progress(app.app_id);
    if (download_progress != null) {
        debug ("download in progress for %s: progress source path = %s", app.app_id, download_progress);
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

  private void setup_find_apps_timeout (string search_query) {
    if (app_search_id != 0) {
        GLib.Source.remove (app_search_id);
    }
    app_search_id = GLib.Timeout.add_seconds (10, () => {
        if (nm_client.get_state () != NM.State.UNKNOWN &&
            nm_client.get_state () != NM.State.CONNECTED_GLOBAL) {
            return true;
        }
        parent_scope.results_invalidated(Unity.SearchType.DEFAULT);
        app_search_id = 0;
        return false;
    });
  }

  async void find_available_apps (string search_query) {
    if (nm_client.get_state () != NM.State.UNKNOWN &&
           nm_client.get_state () != NM.State.CONNECTED_GLOBAL) {
        setup_find_apps_timeout (search_query);
        return;
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
    } catch (WebserviceError e) {
        debug ("Error calling webservice: %s", e.message);
        setup_find_apps_timeout (search_query);
    }
  }

  bool can_search_internet() {
    return Unity.PreferencesManager.get_default().remote_content_search != Unity.PreferencesManager.RemoteContent.NONE;
  }

  async void find_apps (string search_query, Unity.SearchType search_type)
  {
    // we don't provide any results for Home searches for now
    if (search_type != Unity.SearchType.DEFAULT) {
        return;
    }

    yield find_installed_apps (search_query);
    if (can_search_internet()) {
        yield find_available_apps (search_query);
        // TODO: updates coming real soon
        //yield find_available_updates (search_query);
    } else {
        debug ("not showing suggested apps: online search is off");
    }
  }

  public override void run ()
  {
  }

  public override void run_async (Unity.ScopeSearchBaseCallback async_callback)
  {
    find_apps.begin (search_context.search_query, search_context.search_type, (obj, res) => {
        find_apps.end (res);
        async_callback(this);
        debug ("run_async: finished.");
    });
  }
}
