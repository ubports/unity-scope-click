#ifndef CLICKWEBSERVICE_H
#define CLICKWEBSERVICE_H

#include <string>
#include <list>

namespace Unity {
namespace Click {

template <class T>
struct AsyncResult
{
    virtual void operator()(T result) = 0;
};

struct Package
{
    std::string name; // formerly app_id
    std::string title;
    std::string price;
    std::string icon_url;
    std::string url;
    void matches (std::string query, AsyncResult<bool> callback);
};

struct PackageDetails
{
    std::string name; // formerly app_id
    std::string icon_url;
    std::string title;
    std::string description;
    std::string download_url;
    std::string rating;
    std::string keywords;
    std::string terms_of_service;
    std::string license;
    std::string publisher;
    std::string main_screenshot_url;
    std::string more_screenshots_urls;
    std::string binary_filesize;
    std::string version;
    std::string framework;
    static void from_json(std::string json);
};

class Webservice
{
public:
    Webservice();
    void search (std::string query, AsyncResult<std::list<Package>> callback);
};

} // namespace Click
} // namespace Unity

#endif // CLICKWEBSERVICE_H

