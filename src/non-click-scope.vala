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
  const string[] NON_CLICK_DESKTOPS =
  {
    "webbrowser-app.desktop"
  };

  const string ICON_PATH = Config.DATADIR + "/icons/unity-icon-theme/places/svg/";
  const string GENERIC_SCOPE_ICON = ICON_PATH + "service-generic.svg";
  const string SCOPES_MODEL = "com.canonical.Unity.SmartScopes.RemoteScopesModel";

  private Dee.Model remote_scopes_model;

  public NonClickScope ()
  {
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
      filter.add_option ("scopes", _("Search plugins"));
      filter.add_option ("system", _("System"));

      filters.add (filter);
    }
    this.filter_set = filters;

    /* get all the remote scopes */
    var shared_model = new Dee.SharedModel (SCOPES_MODEL);
    shared_model.set_schema ("s", "s", "s", "s", "s", "as");
    shared_model.end_transaction.connect (this.remote_scopes_changed);
    remote_scopes_model = shared_model;

    set_search_async_func (this.dispatch_search);
    set_preview_async_func (this.dispatch_preview);
  }

  private void remote_scopes_changed ()
  {
    this.results_invalidated (SearchType.DEFAULT);
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
  }

  private async Preview? perform_preview (ResultPreviewer previewer)
  {
    return null;
  }
}

