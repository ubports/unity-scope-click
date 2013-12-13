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

public errordomain CredentialsError {
    CREDENTIALS_ERROR
}

public class UbuntuoneCredentials : GLib.Object {

    public async HashTable<string, string> get_credentials () throws CredentialsError {
        string encoded_creds = null;
        string error_message = "";

        Ag.Manager _manager = new Ag.Manager.for_service_type ("ubuntuone");
        GLib.List<uint> _accts = _manager.list_by_service_type ("ubuntuone");

        if (_accts.length () > 0) {
            if (_accts.length () > 1) {
                debug ("Found %u accounts. Using first.", _accts.length ());
            }
            var account = _manager.get_account (_accts.nth_data(0));
            debug ("Using account id: %u", _accts.nth_data(0));
            var acct_service = new Ag.AccountService (account, null);
            var auth_data = acct_service.get_auth_data ();
            var session_data = auth_data.get_parameters ();

            var session = new Signon.AuthSession (auth_data.get_credentials_id (), auth_data.get_method ());
            session.process (session_data, auth_data.get_mechanism (),
                             (session, data) => {
                                var value = data.lookup("Secret");
                                if (value == null) {
                                    error_message = "Account found, without token.";
                                    get_credentials.callback ();
                                    return;
                                }
                                encoded_creds = value.get_string();
                                get_credentials.callback ();
                             });
            yield;
        } else {
            error_message = "Please log into your Ubuntu One account in System Settings.";
        }
        if (encoded_creds == null) {
            throw new CredentialsError.CREDENTIALS_ERROR(error_message);
        }
        return Soup.Form.decode (encoded_creds);

    }

    public async void invalidate_credentials () throws CredentialsError {
        Ag.Manager _manager = new Ag.Manager.for_service_type ("ubuntuone");
        GLib.List<uint> _accts = _manager.list_by_service_type ("ubuntuone");

        foreach (var account_id in _accts) {
            debug ("Removing account id: %u", account_id);
            var account = _manager.get_account (account_id);
            account.delete();
            try {
                yield account.store_async();
            } catch (Ag.AccountsError e) {
                string error_message = "Please delete your existing Ubuntu One account and create a new one in System Settings.";
                throw new CredentialsError.CREDENTIALS_ERROR(error_message);
            }
        }
    }
}
