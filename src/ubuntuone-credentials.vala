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
        // TODO: get the proper credentials from UOA
        HashTable<string, string> credentials = new HashTable<string, string> (str_hash, str_equal);
        credentials["consumer_key"] = "FAKE_CONSUMER_KEY";
        credentials["consumer_secret"] = "FAKE_CONSUMER_SECRET";
        credentials["token"] = "FAKE_TOKEN";
        credentials["token_secret"] = "FAKE_TOKEN_SECRET";
        return credentials;
    }
}
