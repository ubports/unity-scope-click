#ifndef CLICKINDEX_H
#define CLICKINDEX_H

#include <string>
#include <list>
#include <functional>
#include "webclient.h"

namespace click {

const std::string SEARCH_BASE_URL = "https://search.apps.ubuntu.com/";
const std::string SEARCH_PATH = "api/v1/search";
const std::string SUPPORTED_FRAMEWORKS = "framework:ubuntu-sdk-13.10";
const std::string QUERY_ARGNAME = "q";
const std::string ARCHITECTURE = "architecture:";
const std::string DETAILS_PATH = "api/v1/package/%s";

struct Package
{
    std::string name; // formerly app_id
    std::string title;
    std::string price;
    std::string icon_url;
    std::string url;
    void matches (std::string query, std::function<bool> callback);
};

bool operator==(const Package& lhs, const Package& rhs) {
    return lhs.name == rhs.name &&
            lhs.title == rhs.title &&
            lhs.price == rhs.price &&
            lhs.icon_url == rhs.icon_url &&
            lhs.url == rhs.url;
}

class PackageList : public std::list<Package>
{
public:
    void loadJson(const std::string &json);
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
    void loadJson(const std::string &json);
};

class Index
{
protected:
    QSharedPointer<web::Service> service;
public:
    Index(const QSharedPointer<click::web::Service>& service);
    void search (const std::string &query, std::function<void(PackageList)> callback);
    ~Index();
};

} // namespace click

#endif // CLICKINDEX_H

