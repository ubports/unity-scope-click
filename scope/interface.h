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
    void get_package(std::function<Package> callback);
//    void get_manifest(std::function<JsonNode> callback); // pending json library
    void get_dotdesktop(std::function<std::string> callback);
    void can_uninstall(std::function<bool> callback);
    void uninstall(std::function<bool> callback);
};

class Interface
{
public:
    Interface();
    std::string get_arch();
    std::string get_frameworks();
//    void get_manifests(std::function<list<JsonNode>> callback); // pending json library
    void get_installed(std::string search_query, std::function<std::list<Application>> callback);
};

} // namespace Click
} // namespace Unity

#endif // UNITY_CLICK_INTERFACE_H
