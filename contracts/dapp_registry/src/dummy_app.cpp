#include <dummy_app/dummy_app.hpp>

namespace eosio {

dummy_app::dummy_app(name s, name code, datastream<const char *> ds)
    : eosio::contract(s, code, ds),
      _config_singleton(get_self(), get_self().value)
{
    _config = _config_singleton.get_or_create(_self, configuration{});
}

dummy_app::~dummy_app()
{
    _config_singleton.set( _config, get_self() );
}

void dummy_app::setdappsacc( const name& dapp_registry_account )
{
    require_auth( get_self() );

    check( eosio::is_account(dapp_registry_account), "account doesn`t exist" );
    _config.dapp_registry_account = dapp_registry_account;
}

void dummy_app::transfer( const name& from, const name& to, const eosio::asset& amount, const std::string& memo )
{
    check( from != get_self(), "cannot transfer from self account" );
    check( to == get_self(), "cannot transfer to other account" );
    check( eosio::is_account(_config.dapp_registry_account), "account doesn`t exist" );

    require_recipient(_config.dapp_registry_account);
}

void dummy_app::greetings( name user ) {
    require_auth( user );

    print( "Hello, ", user);
}


extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (code == receiver)
    {
        switch (action)
        {
        EOSIO_DISPATCH_HELPER( dummy_app, (setdappsacc) )
        }
    }
    else if (code == token_account.value && action == transfer_action.value) {
        execute_action( name(receiver), name(code), &dummy_app::transfer );
    }
}
} /// extern "C"

} /// namespace eosio
