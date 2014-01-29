#ifndef CLICKINDEX_H
#define CLICKINDEX_H

#include <string>
#include <list>
#include <functional>
#include "webclient.h"

namespace click {

struct Package
{
    std::string name; // formerly app_id
    std::string title;
    std::string price;
    std::string icon_url;
    std::string url;
    void matches (std::string query, std::function<bool> callback);
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

class Index
{
protected:
    QSharedPointer<web::Service> service;
public:
    Index();
    Index(const QSharedPointer<click::web::Service>& service);
    void search (std::string query, std::function<void(std::list<Package>)> callback);
    ~Index();
};

} // namespace click

#endif // CLICKINDEX_H

