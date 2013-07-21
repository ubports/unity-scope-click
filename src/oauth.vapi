/* Minimal vapi for OAuth */
[CCode (cheader_filename="oauth.h")]
namespace OAuth {

    [CCode (cheader_filename="oauth.h", cprefix="OA_")]
    public enum Method {
        HMAC,
        RSA,
        PLAINTEXT
    }

    [CCode (cheader_filename="oauth.h", cprefix="oauth_")]
    public string sign_url2(string url, out string postargs,
                            OAuth.Method method,
                            string? http_method,
                            string c_key, string c_secret,
                            string t_key, string t_secret);

}
