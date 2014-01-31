#include "index.h"

namespace click
{

Index::Index(const QSharedPointer<click::web::Service>& service) : service(service)
{

}

void Index::search (const std::string& query, std::function<void(std::list<click::Package>)> callback)
{
    Q_UNUSED(callback)
    click::web::CallParams params;
    params.add("q", query.c_str());
    service->call("xx", params);
}

Index::Index()
{
}

Index::~Index()
{
}

} // namespace click
