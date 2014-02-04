#include "index.h"

namespace click
{

Index::Index(const QSharedPointer<click::web::Service>& service) : service(service)
{

}

void Index::search (const std::string& query, std::function<void(std::list<click::Package>)> callback)
{
    click::web::CallParams params;
    params.add(click::QUERY_ARGNAME, query.c_str());
    QSharedPointer<click::web::Response> response = service->call(click::SEARCH_PATH, params);
    QObject::connect(response.data(), &click::web::Response::finished, [&, callback](QString reply){
        Q_UNUSED(reply);
        Q_UNUSED(response);
        std::list<click::Package> s;
        callback(s);
    });
}

Index::Index()
{
}

Index::~Index()
{
}

} // namespace click
