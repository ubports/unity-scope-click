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

using Unity;

class NonClickScope: Unity.SimpleScope
{
  /* List of app IDs that aren't proper click packages yet */
  const string[] NON_CLICK_DESKTOPS =
  {
    "address-book-app.desktop",
    "calendar-app.desktop",
    "camera-app.desktop",
    "click-update-manager.desktop",
    "dialer-app.desktop",
    "friends-app.desktop",
    "gallery-app.desktop",
    "mediaplayer-app.desktop",
    "messaging-app.desktop",
    "music-app.desktop",
    "notes-app.desktop",
    "rssreader-app.desktop",
    "ubuntu-calculator-app.desktop",
    "ubuntu-clock-app.desktop",
    "ubuntu-filemanager-app.desktop",
    "ubuntu-system-settings.desktop",
    "ubuntu-terminal-app.desktop",
    "ubuntu-weather-app.desktop",
    "webbrowser-app.desktop"
  };

  const string[] invisible_scope_ids =
  {
    "home.scope",
    "applications-scopes.scope",
    "applications-non-click.scope"
  };

  const string LIBUNITY_SCHEMA = "com.canonical.Unity.Lenses";
  const string ICON_PATH = Config.DATADIR + "/icons/unity-icon-theme/places/svg/";
  const string GENERIC_SCOPE_ICON = ICON_PATH + "service-generic.svg";
  const string SCOPES_MODEL = "com.canonical.Unity.SmartScopes.RemoteScopesModel";

  private enum RemoteScopesColumn
  {
    SCOPE_ID,
    NAME,
    DESCRIPTION,
    ICON_HINT,
    SCREENSHOT_URL,
    KEYWORDS
  }

  private enum AppsColumn
  {
    APP_URI,
    NAME,
    ICON_HINT,
    DESCRIPTION,
    SCREENSHOT_URL
  }

  private enum ResultCategory
  {
    INSTALLED,
    SCOPES
  }

  private Dee.Model remote_scopes_model;
  private Dee.Model local_scopes_model;
  private Dee.Index remote_scopes_index;
  private Dee.Index local_scopes_index;
  private Dee.Model apps_model;
  private Dee.Index apps_index;

  private HashTable<unowned string, unowned string> disabled_scope_ids;
  private HashTable<string, bool> locked_scope_ids;

  public NonClickScope ()
  {
    Object ();
  }

  construct
  {
    this.group_name = "com.canonical.Unity.Scope.Applications.Click";
    this.unique_name = "/com/canonical/unity/scope/applications/non_click";

    /* set schema */
    var schema = new Schema ();
    schema.add_field ("scope_disabled", "u", Schema.FieldType.OPTIONAL);
    this.schema = schema;

    Category cat;
    var icon_dir = File.new_for_path (ICON_PATH);

    /* set categories */
    var categories = new CategorySet ();
    cat = new Unity.Category ("installed", _("Installed"),
                              new FileIcon (icon_dir.get_child ("group-installed.svg")));
    categories.add (cat);

    cat = new Unity.Category ("scopes", _("Dash plugins"),
                              new FileIcon (icon_dir.get_child ("group-installed.svg")));
    categories.add (cat);
    this.category_set = categories;

    /* set filters */
    var filters = new FilterSet ();
    {
      var filter = new CheckOptionFilter ("type", _("Type"));
      filter.sort_type = Unity.OptionsFilter.SortType.DISPLAY_NAME;

      filter.add_option ("accessories", _("Accessories"));
      filter.add_option ("education", _("Education"));
      filter.add_option ("game", _("Games"));
      filter.add_option ("graphics", _("Graphics"));
      filter.add_option ("internet", _("Internet"));
      filter.add_option ("fonts", _("Fonts"));
      filter.add_option ("office", _("Office"));
      filter.add_option ("media", _("Media"));
      filter.add_option ("customization", _("Customization"));
      filter.add_option ("accessibility", _("Accessibility"));
      filter.add_option ("developer", _("Developer"));
      filter.add_option ("science-and-engineering", _("Science & engineering"));
      filter.add_option ("scopes", _("Dash plugins"));
      filter.add_option ("system", _("System"));

      filters.add (filter);
    }
    this.filter_set = filters;

    /* get all the remote scopes */
    var shared_model = new Dee.SharedModel (SCOPES_MODEL);
    shared_model.set_schema ("s", "s", "s", "s", "s", "as");
    shared_model.end_transaction.connect (this.remote_scopes_changed);
    remote_scopes_model = shared_model;

    remote_scopes_index = Utils.prepare_index (remote_scopes_model,
                                               RemoteScopesColumn.NAME,
                                               (model, iter) =>
    {
      unowned string name = model.get_string (iter, RemoteScopesColumn.NAME);
      return "%s\n%s".printf (_("scope"), name);
    }, null);

    local_scopes_model = new Dee.SequenceModel ();
    // use similar schema as remote_scopes_model, so we can share code
    local_scopes_model.set_schema ("s", "s", "s", "s", "s");

    local_scopes_index = Utils.prepare_index (local_scopes_model,
                                              RemoteScopesColumn.NAME,
                                              (model, iter) =>
    {
      unowned string name = model.get_string (iter, RemoteScopesColumn.NAME);
      return "%s\n%s".printf (_("scope"), name);
    }, null);

    /* monitor scopes dconf keys */
    disabled_scope_ids = new HashTable<unowned string, unowned string> (str_hash, str_equal);
    var pref_man = PreferencesManager.get_default ();
    pref_man.notify["disabled-scopes"].connect (update_disabled_scopes_hash);
    update_disabled_scopes_hash ();

    locked_scope_ids = new HashTable<string, bool> (str_hash, str_equal);
    var settings = new Settings (LIBUNITY_SCHEMA);
    foreach (unowned string scope_id in settings.get_strv ("locked-scopes"))
    {
      locked_scope_ids[scope_id] = true;
    }

    apps_model = new Dee.SequenceModel ();
    // uri, app title, description, screenshot?
    apps_model.set_schema ("s", "s", "s", "s", "s");
    apps_model.set_column_names ("uri", "name", "icon", "description", "screenshot");

    apps_index = Utils.prepare_index (apps_model, AppsColumn.NAME, (model, iter) =>
    {
      return model.get_string (iter, AppsColumn.NAME);
    }, null);

    build_scope_index.begin ();
    build_apps_data ();

    /* set the search functions */
    set_search_async_func (this.dispatch_search);
    set_preview_async_func (this.dispatch_preview);
    set_activate_func (this.activate);
  }

  private void update_disabled_scopes_hash ()
  {
    disabled_scope_ids.remove_all ();
    var pref_man = PreferencesManager.get_default ();
    foreach (unowned string scope_id in pref_man.disabled_scopes)
    {
      // using HashTable as a set (optimized in glib when done like this)
      disabled_scope_ids[scope_id] = scope_id;
    }
  }

  private async void build_scope_index ()
  {
    try
    {
      var registry = yield Unity.Protocol.ScopeRegistry.find_scopes (null);
      var flattened = new SList<Protocol.ScopeRegistry.ScopeMetadata> ();
      foreach (var node in registry.scopes)
      {
        flattened.prepend (node.scope_info);
        foreach (var subscope_info in node.sub_scopes)
          flattened.prepend (subscope_info);
      }
      flattened.reverse ();
      foreach (var info in flattened)
      {
        if (info.is_master) continue;
        if (info.id in invisible_scope_ids) continue;

        local_scopes_model.append (info.id, info.name, info.description,
                                   info.icon, "");
      }
    }
    catch (Error err)
    {
      warning ("Unable to find scopes: %s", err.message);
    }
  }

  private void build_apps_data ()
  {
    var keyfile = new KeyFile ();
    foreach (unowned string desktop_file_id in NON_CLICK_DESKTOPS)
    {
      bool loaded = false;
      try
      {
        loaded = keyfile.load_from_dirs ("applications/" + desktop_file_id,
                                         Environment.get_system_data_dirs (),
                                         null,
                                         KeyFileFlags.KEEP_TRANSLATIONS);
      }
      catch (Error err)
      {
        loaded = false;
      }
      if (!loaded)
      {
        continue;
      }
      var app_info = new DesktopAppInfo.from_keyfile (keyfile);
      if (app_info == null) continue;

      string? screenshot = null;
      bool is_touch_app = false;
      try
      {
        is_touch_app = keyfile.get_boolean (KeyFileDesktop.GROUP, "X-Ubuntu-Touch");
        screenshot = keyfile.get_string (KeyFileDesktop.GROUP, "X-Screenshot");
      }
      catch (Error err)
      {
        // ignore
      }

      if (!is_touch_app) continue;

      apps_model.append ("application://" + desktop_file_id,
                         app_info.get_display_name (),
                         app_info.get_icon ().to_string (),
                         app_info.get_description (),
                         screenshot);
    }
  }

  private void remote_scopes_changed ()
  {
    this.results_invalidated (SearchType.DEFAULT);
  }

  private void disable_scope (string scope_id)
  {
    if (scope_id in disabled_scope_ids) return;

    var pref_man = PreferencesManager.get_default ();
    var disabled_scopes = pref_man.disabled_scopes;
    disabled_scopes += scope_id;

    var settings = new Settings (LIBUNITY_SCHEMA);
    settings.set_strv ("disabled-scopes", disabled_scopes);
  }

  private void enable_scope (string scope_id)
  {
    if (!(scope_id in disabled_scope_ids)) return;

    var pref_man = PreferencesManager.get_default ();
    string[] disabled_scopes = {};
    foreach (unowned string disabled_scope_id in pref_man.disabled_scopes)
    {
      if (disabled_scope_id != scope_id)
        disabled_scopes += disabled_scope_id;
    }

    var settings = new Settings (LIBUNITY_SCHEMA);
    settings.set_strv ("disabled-scopes", disabled_scopes);
  }

  private string get_scope_icon (string icon_hint, bool is_disabled)
  {
    try
    {
      Icon base_icon = icon_hint == null || icon_hint == "" ?
        Icon.new_for_string (GENERIC_SCOPE_ICON) :
        Icon.new_for_string (icon_hint);
      var anno_icon = new AnnotatedIcon (base_icon);
      anno_icon.size_hint = IconSizeHint.SMALL;
      // dim disabled icons by decreasing their alpha
      if (is_disabled)
        anno_icon.set_colorize_rgba (1.0, 1.0, 1.0, 0.5);
      return anno_icon.to_string ();
    }
    catch (Error err)
    {
      return "";
    }
  }

  private void add_apps_results (string query, ResultSet result_set)
  {
    var model = apps_index.model;
    var analyzer = apps_index.analyzer;
    var results = Utils.search_index (apps_index, analyzer, query);
    foreach (var iter in results)
    {
      unowned string app_id = model.get_string (iter, AppsColumn.APP_URI);
      unowned string icon_hint = model.get_string (iter, AppsColumn.ICON_HINT);
      unowned string name = model.get_string (iter, AppsColumn.NAME);

      var result = ScopeResult ();
      result.uri = app_id;
      result.category = ResultCategory.INSTALLED;
      result.icon_hint = icon_hint;
      result.result_type = Unity.ResultType.PERSONAL;
      result.mimetype = "application/x-desktop";
      result.title = name;
      result.comment = "";
      result.dnd_uri = app_id;

      result_set.add_result (result);
    }
  }

  /* Models backing the index need to have similar enough schema! */
  private void add_scope_results (Dee.Index index, string query,
                                  ResultSet result_set)
  {
    var model = index.model;
    var analyzer = index.analyzer;
    var results = Utils.search_index (index, analyzer, query);
    foreach (var iter in results)
    {
      unowned string scope_id =
        model.get_string (iter, RemoteScopesColumn.SCOPE_ID);
      unowned string icon_hint =
        model.get_string (iter, RemoteScopesColumn.ICON_HINT);
      bool is_disabled = scope_id in disabled_scope_ids;

      var result = Unity.ScopeResult ();
      result.uri = @"scope://$(scope_id)";
      result.category = ResultCategory.SCOPES;
      result.icon_hint = get_scope_icon (icon_hint, is_disabled);
      result.result_type = Unity.ResultType.DEFAULT;
      result.mimetype = "application/x-unity-scope";
      result.title = model.get_string (iter, RemoteScopesColumn.NAME);
      result.comment = "";
      result.dnd_uri = result.uri;
      result.metadata = new HashTable<string, Variant> (str_hash, str_equal);
      result.metadata["scope_disabled"] = new Variant.uint32 (is_disabled ? 1 : 0);

      result_set.add_result (result);
    }

  }

  private void dispatch_search (ScopeSearchBase search, ScopeSearchBaseCallback cb)
  {
    perform_search.begin (search, (obj, res) =>
    {
      perform_search.end (res);
      cb (search);
    });
  }

  private void dispatch_preview (ResultPreviewer previewer, AbstractPreviewCallback cb)
  {
    perform_preview.begin (previewer, (obj, res) =>
    {
      var preview = perform_preview.end (res);
      cb (previewer, preview);
    });
  }

  private async void perform_search (ScopeSearchBase search)
  {
    var search_type = search.search_context.search_type;
    bool include_scope_results = true;
    if (search.search_context.filter_state != null)
    {
      var filter = search.search_context.filter_state.get_filter_by_id ("type");
      if (filter != null && filter.filtering)
      {
        var of = filter as OptionsFilter;
        include_scope_results = of.get_option ("scopes").active;
      }
    }

    add_apps_results (search.search_context.search_query,
                      search.search_context.result_set);

    if (include_scope_results && search_type == SearchType.DEFAULT)
    {
      add_scope_results (local_scopes_index, search.search_context.search_query,
                         search.search_context.result_set);
      add_scope_results (remote_scopes_index, search.search_context.search_query,
                         search.search_context.result_set);
    }
  }

  private async Preview? perform_preview (ResultPreviewer previewer)
  {
    unowned string uri = previewer.result.uri;
    if (uri.has_prefix ("scope://"))
    {
      // linear search, "yey"!
      string scope_id = uri.substring (8);
      var model = local_scopes_model;
      var iter = model.get_first_iter ();
      var end_iter = model.get_last_iter ();
      var found_iter = end_iter;
      while (iter != end_iter)
      {
        if (model.get_string (iter, RemoteScopesColumn.SCOPE_ID) == scope_id)
        {
          found_iter = iter;
          break;
        }
        iter = model.next (iter);
      }

      if (found_iter == end_iter)
      {
        // try in the remote_scopes_model
        model = remote_scopes_model;
        iter = model.get_first_iter ();
        end_iter = model.get_last_iter ();
        while (iter != end_iter)
        {
          if (model.get_string (iter, RemoteScopesColumn.SCOPE_ID) == scope_id)
          {
            found_iter = iter;
            break;
          }
          iter = model.next (iter);
        }
      }

      if (found_iter == end_iter) return null;

      unowned string name = model.get_string (found_iter, RemoteScopesColumn.NAME);
      unowned string description = model.get_string (found_iter, RemoteScopesColumn.DESCRIPTION);
      unowned string icon = model.get_string (found_iter, RemoteScopesColumn.ICON_HINT);
      unowned string screenshot = model.get_string (found_iter, RemoteScopesColumn.SCREENSHOT_URL);
      Icon? app_icon = null;
      Icon? screenshot_icon = null;
      try
      {
        if (icon != "") app_icon = Icon.new_for_string (icon);
        if (screenshot != "") screenshot_icon = Icon.new_for_string (screenshot);
      }
      catch (Error err) {}

      var preview = new ApplicationPreview (name, "", description,
                                            app_icon, screenshot_icon);
      PreviewAction action;
      bool scope_disabled = scope_id in disabled_scope_ids;
      if (scope_disabled)
      {
        action = new Unity.PreviewAction ("enable-scope", _("Enable"), null);
      }
      else
      {
        action = new Unity.PreviewAction ("disable-scope", _("Disable"), null);
      }
      if (!(scope_id in locked_scope_ids))
      {
        preview.add_action (action);
      }
      return preview;
    }
    else if (uri.has_prefix ("application:"))
    {
      // linear search, "yey"!
      var iter = apps_model.get_first_iter ();
      var end_iter = apps_model.get_last_iter ();
      var found_iter = end_iter;
      while (iter != end_iter)
      {
        if (apps_model.get_string (iter, AppsColumn.APP_URI) == uri)
        {
          found_iter = iter;
          break;
        }
        iter = apps_model.next (iter);
      }
      if (found_iter == end_iter) return null; // uh oh

      unowned string name = apps_model.get_string (found_iter, AppsColumn.NAME);
      unowned string description = apps_model.get_string (found_iter, AppsColumn.DESCRIPTION);
      unowned string icon = apps_model.get_string (found_iter, AppsColumn.ICON_HINT);
      unowned string screenshot = apps_model.get_string (found_iter, AppsColumn.SCREENSHOT_URL);
      Icon? app_icon = null;
      Icon? screenshot_icon = null;
      try
      {
        if (icon != "") app_icon = Icon.new_for_string (icon);
        if (screenshot != "") screenshot_icon = Icon.new_for_string (screenshot);
      }
      catch (Error err) {}
      var preview = new ApplicationPreview (name, "", description,
                                            app_icon, screenshot_icon);
      var launch_action = new Unity.PreviewAction ("launch", _("Launch"), null);
      preview.add_action (launch_action);
      return preview;
    }
    return null;
  }

  private ActivationResponse? activate (ScopeResult result,
                                        SearchMetadata metadata,
                                        string? action_id)
  {
    if (result.uri.has_prefix ("scope://"))
    {
      var scope_id = result.uri.substring (8);
      if (action_id == "enable-scope")
      {
        enable_scope (scope_id);
      }
      else if (action_id == "disable-scope")
      {
        disable_scope (scope_id);
      }
      return new Unity.ActivationResponse (HandledType.SHOW_PREVIEW);
    }

    return null;
  }
}

