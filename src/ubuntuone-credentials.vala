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
        CredentialsError error = null;
        var u1_schema = new Secret.Schema ("com.ubuntu.one.Credentials", Secret.SchemaFlags.DONT_MATCH_NAME,
                                           "token-name", Secret.SchemaAttributeType.STRING,
                                           "key-type", Secret.SchemaAttributeType.STRING);

        var attributes = new GLib.HashTable<string, string> (str_hash, str_equal);
        attributes["key-type"] = "Ubuntu SSO credentials";

        Secret.password_lookupv.begin (u1_schema, attributes, null, (obj, async_res) => {
            try {
                encoded_creds = Secret.password_lookupv.end (async_res);
            } catch (GLib.Error e) {
                debug ("error getting u1 tokens: %s", e.message);
                error = new CredentialsError.CREDENTIALS_ERROR (e.message);
            }
            get_credentials.callback ();
        });
        yield;
        if (error != null) {
            throw error;
        }
        return Soup.Form.decode (encoded_creds);
    }
}
