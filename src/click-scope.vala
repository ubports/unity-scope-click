private const string ACTION_INSTALL_CLICK = "install_click";
private const string ACTION_OPEN_CLICK = "open_click";
private const string ACTION_PIN_TO_LAUNCHER = "pin_to_launcher";
private const string ACTION_UNINSTALL_CLICK = "uninstall_click";

class ClickPreviewer: Unity.ResultPreviewer
{
    private const string DETAILS_URL = "https://search.apps.staging.ubuntu.com/api/v1/package/%s";
    internal Soup.SessionAsync http_session;
    Unity.AbstractPreviewCallback async_callback;
    ClickScope scope;

    public ClickPreviewer (ClickScope scope)
    {
        this.scope = scope;
        http_session = new Soup.SessionAsync ();
        http_session.user_agent = "%s/%s (libsoup)".printf("UbuntuClickScope", "0.1");
    }


    public override Unity.AbstractPreview? run () {
        return null;
    }

    public override void run_async (Unity.AbstractPreviewCallback async_callback) {
        var app_id = result.metadata.get("app_id").get_string();
        string url = DETAILS_URL.printf(app_id);
        debug ("calling %s", url);
        var message = new Soup.Message ("GET", url);
        http_session.queue_message (message, message_queued);
        this.async_callback = async_callback;
    }

    public void message_queued(Soup.Session http_session, Soup.Message message)
    {
/*
        // TODO: revert fake data
        message.status_code = Soup.KnownStatusCode.OK;
        message.response_body.truncate();
        message.response_body.append_take(FAKE_JSON_PACKAGE_DETAILS.data);
        // TODO: revert fake data ^^^^
*/

        if (message.status_code != Soup.KnownStatusCode.OK) {
            debug ("Web request failed: HTTP %u %s",
                   message.status_code, message.reason_phrase);
        } else {
            message.response_body.flatten ();
            var response = (string) message.response_body.data;
            debug ("response is %s", response);

            var preview = scope.build_app_preview(response);
            preview.add_action (new Unity.PreviewAction (ACTION_INSTALL_CLICK, ("Install"), null));
            preview.add_action (new Unity.PreviewAction (ACTION_PIN_TO_LAUNCHER, ("Pin to launcher"), null));
            async_callback (this, preview);
        }
    }

}

class ClickScope: Unity.AbstractScope
{

  public ClickScope ()
  {
  }

  public Unity.ApplicationPreview build_app_preview(string json) {
    var details = new AppDetails.from_json (json);
    var icon = new FileIcon(File.new_for_uri(details.icon_url));
    var screenshot = new FileIcon(File.new_for_uri(details.main_screenshot_url));
    var preview = new Unity.ApplicationPreview (details.title, "subtitle", details.description, icon, screenshot);
    //preview.image_source_uri = "http://backyardbrains.com/about/img/slashdot-logo.png";
    preview.license = details.license;
    preview.add_info(new Unity.InfoHint.with_variant("more-screenshots", "Screenshots", null, new Variant.strv(details.more_screenshot_urls)));
    preview.add_info(new Unity.InfoHint.with_variant("keywords", "Keywords", null, new Variant.strv(details.keywords)));
    return preview;

  }

  public override Unity.ActivationResponse? activate (Unity.ScopeResult result, Unity.SearchMetadata metadata, string? action_id)
  {
      if (action_id == ACTION_INSTALL_CLICK) {
        var app_id = result.metadata.get("app_id");
        debug ("################## INSTALLATION started: %s, %s", action_id, app_id.get_string());
        install_app (app_id.get_string());
        return null;
      } else if (action_id == ACTION_PIN_TO_LAUNCHER) {
        var preview = build_app_preview(FAKE_JSON_PACKAGE_DETAILS);
        preview.add_action (new Unity.PreviewAction (ACTION_OPEN_CLICK, ("Open"), null));
        preview.add_action (new Unity.PreviewAction (ACTION_PIN_TO_LAUNCHER, ("Pin to launcher"), null));
        preview.add_action (new Unity.PreviewAction (ACTION_UNINSTALL_CLICK, ("Uninstall"), null));
        debug ("######## RETURNING PREVIEW ########## ACTION started: %s", action_id);
        return new Unity.ActivationResponse.with_preview(preview);
      } else {
        debug ("################## ACTION started: %s", action_id);
        return null;
      }
  }

  public void install_app (string app_id) {
      var downloader = get_downloader ();
      var download_metadata = new HashTable<string, Variant>(str_hash, str_equal);
      download_metadata["app_id"] = app_id;
      var headers = new HashTable<string, string>(str_hash, str_equal);
      debug ("about to start download.");
      var download_path = downloader.createDownload("http://slashdot.org", download_metadata, headers);
      var download = get_download(download_path);
      download.start();
      debug ("started download, object path: %s", download_path);
  }

  public override Unity.ScopeSearchBase create_search_for_query (Unity.SearchContext ctx)
  {
    var search = new ClickSearch ();
    search.set_search_context (ctx);
    return search;
  }

  public override Unity.ResultPreviewer create_previewer (Unity.ScopeResult result, Unity.SearchMetadata metadata)
  {
    debug ("creating previewer *********************************88");
    var cpp = new ClickPreviewer(this);
    cpp.set_scope_result(result);
    cpp.set_search_metadata(metadata);
    return cpp;
  }

  public override Unity.CategorySet get_categories ()
  {
    var categories = new Unity.CategorySet ();
    var icon = new FileIcon(File.new_for_path("/usr/share/icons/unity-icon-theme/places/svg/service-openclipart.svg"));
    categories.add (new Unity.Category("global", "Applications", icon, Unity.CategoryRenderer.HORIZONTAL_TILE));
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
  private const string SEARCH_URL = "https://search.apps.staging.ubuntu.com/api/v1/search?q=%s";

  internal Soup.SessionAsync http_session;
  internal Unity.ScopeSearchBaseCallback async_callback;

  public ClickSearch ()
  {
    http_session = new Soup.SessionAsync ();
    http_session.user_agent = "%s/%s (libsoup)".printf("UbuntuClickScope", "0.1");
  }

  private void add_result (App app)
  {
    File icon_file = File.new_for_uri (app.icon_url);
    var icon = new FileIcon (icon_file);

    var result = Unity.ScopeResult ();
    result.icon_hint = app.icon_url;
    result.category = 0;
    result.result_type = Unity.ResultType.DEFAULT;
    result.mimetype = "inode/folder";
    result.title = app.title;
    result.comment = "no comment";
    result.dnd_uri = "test::uri";
    result.metadata = new HashTable<string, Variant> (str_hash, str_equal);
    debug (app.title);
    //result.uri = DETAILS_URL.printf(app.app_id);
    result.metadata.insert("app_id", new GLib.Variant.string(app.app_id));
    result.metadata.insert("title", new GLib.Variant.string(app.title));
    result.metadata.insert("price", new GLib.Variant.string(app.price));
    debug (app.price);
    search_context.result_set.add_result (result);
  }


  public override void run ()
  {
  }

  public void message_queued(Soup.Session http_session, Soup.Message message)
  {
/*
    // TODO: revert fake data
    foreach (var app in new AvailableApps.from_json (FAKE_JSON_SEARCH_RESULT)) {
        add_result (app);
    }
    async_callback(this);
    return;
    // TODO: revert fake data ^^^^^^
*/

    if (message.status_code != Soup.KnownStatusCode.OK) {
        debug ("Web request failed: HTTP %u %s",
               message.status_code, message.reason_phrase);
        //error = new PurchaseError.PURCHASE_ERROR (message.reason_phrase);
    } else {
        message.response_body.flatten ();
        var response = (string) message.response_body.data;
        debug ("response is %s", response);
        var apps = new AvailableApps.from_json (response);
        foreach (var app in apps) {
            add_result (app);
        }
    }
    async_callback(this);
  }

  public override void run_async (Unity.ScopeSearchBaseCallback async_callback)
  {
    // TODO: revert fake query
    //search_context.search_query = "a*";

    string url = SEARCH_URL.printf(search_context.search_query);
    debug ("calling %s", url);
    var message = new Soup.Message ("GET", url);
    http_session.queue_message (message, message_queued);
    this.async_callback = async_callback;
  }
}


int main ()
{
    var scope = new ClickScope();
    var exporter = new Unity.ScopeDBusConnector (scope);
    exporter.export ();
    Unity.ScopeDBusConnector.run ();
    return 0;
}
