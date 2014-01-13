#include <scopes/QueryBase.h>

using namespace std;
using namespace unity::api::scopes;

class ClickQuery : public QueryBase
{
public:
    ClickQuery(std::string const& query);
    ~ClickQuery();
    virtual void cancelled() override;

    virtual void run(ReplyProxy const& reply) override;

private:
    std::string query_;
};
