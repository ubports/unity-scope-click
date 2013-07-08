class ClickPackagesPreviewer: Unity.ResultPreviewer
{
    private const string DETAILS_URL = "http://search.apps.staging.ubuntu.com/apps/v1/package?q=%s";
    internal Soup.SessionAsync http_session;
    Unity.AbstractPreviewCallback async_callback;

    public ClickPackagesPreviewer ()
    {
        http_session = new Soup.SessionAsync ();
        http_session.user_agent = "%s/%s (libsoup)".printf("UbuntuClickPackagesScope", "0.1");
    }


    public override Unity.AbstractPreview? run () {
        return null;
    }

    public override void run_async (Unity.AbstractPreviewCallback async_callback) {
        var app_id = result.metadata.get("app_id");
        string url = DETAILS_URL.printf(app_id);
        debug ("calling %s", url);
        var message = new Soup.Message ("GET", url);
        http_session.queue_message (message, message_queued);
        this.async_callback = async_callback;
    }

    public void message_queued(Soup.Session http_session, Soup.Message message)
    {
        if (message.status_code != Soup.KnownStatusCode.OK) {
            debug ("Web request failed: HTTP %u %s",
                   message.status_code, message.reason_phrase);
        } else {
            message.response_body.flatten ();
            var response = (string) message.response_body.data;
            debug ("response is %s", response);
            var details = new AppDetails.from_json (response);

            //var icon = new FileIcon(File.new_for_uri(result.icon_hint));
            //var screenshot = new FileIcon(File.new_for_uri(details.main_screenshot_url));
            var icon = new FileIcon(File.new_for_uri("http://backyardbrains.com/about/img/slashdot-logo.png"));
            var screenshot = new FileIcon(File.new_for_uri("http://backyardbrains.com/about/img/slashdot-logo.png"));
            var preview = new Unity.ApplicationPreview (result.title, "subtitle", details.description, icon, screenshot);
            preview.image_source_uri = "http://backyardbrains.com/about/img/slashdot-logo.png";
            preview.license = details.license;

            var purchase_action = new Unity.PreviewAction ("install_application", ("Install App"), icon);
            preview.add_action (purchase_action);
            async_callback (this, preview);
        }
    }
}

class ClickPackagesScope: Unity.AbstractScope
{

  public ClickPackagesScope ()
  {
  }

  public override Unity.ActivationResponse? activate (Unity.ScopeResult result, Unity.SearchMetadata metadata, string? action_id)
  {
      debug ("about to run with the money.");
      debug (action_id);
      return null;
  }

  public override Unity.ScopeSearchBase create_search_for_query (Unity.SearchContext ctx)
  {
    var search = new ClickPackageSearch ();
    search.set_search_context (ctx);
    return search;
  }

  public override Unity.ResultPreviewer create_previewer (Unity.ScopeResult result, Unity.SearchMetadata metadata)
  {
    debug ("creating previewer *********************************88");
    var cpp = new ClickPackagesPreviewer();
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
    return "com.canonical.Unity.Scope.Applications.ClickPackages";
  }

  public override string get_unique_name ()
  {
    return "/com/canonical/unity/scope/applications/click_packages";
  }
}

class ClickPackageSearch: Unity.ScopeSearchBase
{
  private const string SEARCH_URL = "http://search.apps.staging.ubuntu.com/apps/v1/search?q=%s";

  internal Soup.SessionAsync http_session;
  internal Unity.ScopeSearchBaseCallback async_callback;

  public ClickPackageSearch ()
  {
    http_session = new Soup.SessionAsync ();
    http_session.user_agent = "%s/%s (libsoup)".printf("UbuntuClickPackagesScope", "0.1");
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
    result.metadata.insert("app_id", app.app_id);
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
    string url = SEARCH_URL.printf(search_context.search_query);
    debug ("calling %s", url);
    var message = new Soup.Message ("GET", url);
    http_session.queue_message (message, message_queued);
    this.async_callback = async_callback;
  }
}


int main ()
{
    var scope = new ClickPackagesScope();
    var exporter = new Unity.ScopeDBusConnector (scope);
    exporter.export ();
    Unity.ScopeDBusConnector.run ();
    return 0;
}
