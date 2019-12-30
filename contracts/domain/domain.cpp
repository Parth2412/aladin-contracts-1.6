#include <alaio/alaio.hpp>

using namespace alaio;

class[[alaio::contract("domain")]] domain : public alaio::contract
{

public:
domain(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds){}

[[alaio::action]]
void upsert(name user, std::string  account_id, std::string bns_name, std::string zone_hash, std::string zone_file,std::string isExpired){
require_auth(user);
address_index addresses(get_self(), get_first_receiver().value);
auto iterator = addresses.find(user.value);
if (iterator == addresses.end())
{
addresses.emplace(user, [&](auto &row) {
row.key = user;
row.account_id = account_id;
row.bns_name = bns_name;
row.zone_hash = zone_hash;
row.zone_file = zone_file;
row.isExpired = isExpired;
});
}
else
{
addresses.modify(iterator, user, [&](auto &row) {
row.key = user;
row.account_id = account_id;
row.bns_name = bns_name;
row.zone_hash = zone_hash;
row.zone_file = zone_file;
row.isExpired = isExpired;
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
print(p.bns_name);
print(p.zone_hash);
print(p.zone_file);
print(p.isExpired);

}


private : 
struct [[alaio::table]] person {
name key;
std::string account_id;
std::string bns_name;
std::string zone_hash;
std::string zone_file;
std::string isExpired;
uint64_t primary_key() const { return key.value; }
};
typedef alaio::multi_index<"people"_n, person> address_index;
};
