#include <alaio/alaio.hpp>

using namespace alaio;

class[[alaio::contract("subdomain")]] subdomain : public alaio::contract
{

public:
subdomain(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds){}

[[alaio::action]]
void upsert(name user, std::string account_id ,std::string sd_name, std::string domain_name, std::string zone_hash, std::string zone_file){
require_auth(user);
address_index addresses(get_self(), get_first_receiver().value);
auto iterator = addresses.find(user.value);
if (iterator == addresses.end())
{
addresses.emplace(user, [&](auto &row) {
row.key = user;
row.account_id = account_id;
row.sd_name = sd_name;
row.domain_name = domain_name;
row.zone_hash = zone_hash;
row.zone_file = zone_file;
});
}
else
{
addresses.modify(iterator, user, [&](auto &row) {
row.key = user;
row.account_id = account_id;
row.sd_name = sd_name;
row.domain_name = domain_name;
row.zone_hash = zone_hash;
row.zone_file = zone_file;
});
}
}

[[alaio::action]]
void erase(name user) {
require_auth(user);

address_index addresses(get_self(), get_first_receiver().value);

auto iterator = addresses.find(user.value);
check(iterator != addresses.end(), "Record does not exist");
addresses.erase(iterator);
}

[[alaio::action]]
void fetch(name user) {
require_auth(user);

address_index addresses(get_self(), get_first_receiver().value);

auto iterator = addresses.find(user.value);
check(iterator != addresses.end(), "Record does not exist");
const auto& p = *iterator;
print(p.account_id);
print(p.sd_name);
print(p.domain_name);
print(p.zone_file);
print(p.zone_hash);

}

private : 
struct [[alaio::table]] person {
name key;
std::string account_id;
std::string sd_name;
std::string domain_name;
std::string zone_hash;
std::string zone_file;
uint64_t primary_key() const { return key.value; }
};
typedef alaio::multi_index<"people"_n, person> address_index;
};
