#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

#include "index.h"

namespace click
{

void PackageList::loadJson(const std::string& json)
{
    std::istringstream is(json);

    boost::property_tree::ptree pt;
    boost::property_tree::read_json(is, pt);

    BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt)
    {
        assert(v.first.empty()); // array elements have no names
        auto node = v.second;
        Package p;
        p.name = node.get<std::string>("name");
        p.title = node.get<std::string>("title");
        p.price = node.get<std::string>("price");
        p.icon_url = node.get<std::string>("icon_url");
        p.url = node.get<std::string>("resource_url");
        this->push_back(p);
    }
}

Index::Index(const QSharedPointer<click::web::Service>& service) : service(service)
{

}

void Index::search (const std::string& query, std::function<void(click::PackageList)> callback)
{
    click::web::CallParams params;
    params.add(click::QUERY_ARGNAME, query.c_str());
    QSharedPointer<click::web::Response> response = service->call(click::SEARCH_PATH, params);
    QObject::connect(response.data(), &click::web::Response::finished, [&, callback](QString reply){
        Q_UNUSED(response); // so it's still in scope
        click::PackageList s;
        s.loadJson(reply.toUtf8().constData());
        callback(s);
    });
}

Index::~Index()
{
}

} // namespace click
