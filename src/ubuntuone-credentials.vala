class UbuntuoneCredentials : GLib.Object {

    public async HashTable<string, string> get_credentials () {
        string encoded_creds = "";
        var u1_schema = new Secret.Schema ("com.ubuntu.one.Credentials", Secret.SchemaFlags.DONT_MATCH_NAME,
                                           "token-name", Secret.SchemaAttributeType.STRING,
                                           "key-type", Secret.SchemaAttributeType.STRING);

        var attributes = new GLib.HashTable<string,string> (str_hash, str_equal);
        //attributes["token-name"] = "Ubuntu One @ bollo";
        attributes["key-type"] = "Ubuntu SSO credentials";

        Secret.password_lookupv.begin (u1_schema, attributes, null, (obj, async_res) => {
            encoded_creds = Secret.password_lookup.end (async_res);
            get_credentials.callback ();
        });
        yield;
        return Soup.Form.decode (encoded_creds);
    }
}
