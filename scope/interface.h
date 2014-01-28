#ifndef UNITY_CLICK_INTERFACE_H
#define UNITY_CLICK_INTERFACE_H

#include <list>
#include <string>
#include "clickwebservice.h"

namespace Unity {
namespace Click {

class Application
{
public:
    void get_package(AsyncResult<Package> callback);
//    void get_manifest(AsyncResult<JsonNode> callback); // pending json library
    void get_dotdesktop(AsyncResult<std::string> callback);
    void can_uninstall(AsyncResult<bool> callback);
    void uninstall(AsyncResult<bool> callback);
};

class Interface
{
public:
    Interface();
    std::string get_arch();
    std::string get_frameworks();
//    void get_manifests(AsyncResult<list<JsonNode>> callback); // pending json library
    void get_installed(std::string search_query, AsyncResult<std::list<Application>> callback);
};

} // namespace Click
} // namespace Unity

#endif // UNITY_CLICK_INTERFACE_H
