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

errordomain CredentialsError {
    CREDENTIALS_ERROR
}

class UbuntuoneCredentials : GLib.Object {

    public async HashTable<string, string> get_credentials () throws CredentialsError {
        string encoded_creds = "";
        Ag.Manager _manager = new Ag.Manager.for_service_type ("ubuntuone");
        GLib.List<uint> _accts = _manager.list_by_service_type ("ubuntuone");

        if (_accts.length () > 0) {
            if (_accts.length () > 1) {
                debug ("Found %u accounts. Using first.", _accts.length ());
            }
            var account = _manager.get_account (_accts.nth_data(0));
            var acct_service = new Ag.AccountService (account, null);
            var auth_data = acct_service.get_auth_data ();
            var extra_params = new Variant("{&sv}");
            var session_data = auth_data.get_login_parameters (extra_params);

            var session = new Signon.AuthSession (auth_data.get_credentials_id (), auth_data.get_method ());
            session.process (session_data, auth_data.get_mechanism (), (data) => {
                encoded_creds = data["secret"];
                get_credentials.callback ();
            });
        }
        yield;
        _manager.list_Free (_accts);
        return Soup.Form.decode (encoded_creds);

    }
}
