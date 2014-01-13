#include "clickquery.h"
#include <scopes/Annotation.h>
#include <scopes/CategoryRenderer.h>
#include <scopes/CategorisedResult.h>
#include <scopes/Query.h>
#include <scopes/Reply.h>


ClickQuery::ClickQuery(string const& query) :
    query_(query)
{
}

ClickQuery::~ClickQuery()
{
}

void ClickQuery::cancelled()
{
}

void ClickQuery::run(ReplyProxy const& reply)
{
    CategoryRenderer rdr;
    auto cat = reply->register_category("cat1", "Category 1", "", rdr);
    CategorisedResult res(cat);
    res.set_uri("uri");
    res.set_title("scope-A: result 1 for query \"" + query_ + "\"");
    res.set_art("icon");
    res.set_dnd_uri("dnd_uri");
    reply->push(res);

    Query q("scope-A", query_, "");
    Annotation annotation(Annotation::Type::Link);
    annotation.add_link("More...", q);
    reply->push(annotation);

}
