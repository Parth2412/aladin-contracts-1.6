#include <transfer_notifier/transfer_notifier.hpp>


namespace eosio {

   transfer_notifier::transfer_notifier( name s, name code, datastream<const char*> ds )
      : eosio::contract(s, code, ds),
        _config_singleton(get_self(), get_self().value)
   {
      _config = _config_singleton.get_or_create(_self, configuration{});
   }

   transfer_notifier::~transfer_notifier()
   {
      _config_singleton.set( _config, get_self() );
   }


   void transfer_notifier::setdappsacc( const name& dapp_registry_account )
   {
      require_auth( get_self() );

      check( eosio::is_account(dapp_registry_account), "account doesn`t exist" );
      _config.dapp_registry_account = dapp_registry_account;
   }

   void transfer_notifier::transfer( const name& from, const name& to, const eosio::asset& amount, const std::string& memo )
   {
      check( from != get_self(), "cannot transfer from self account" );
      check( to == get_self(), "cannot transfer to other account" );
      check( eosio::is_account(_config.dapp_registry_account), "account doesn`t exist" );

      if (eosio::is_account(_config.dapp_registry_account)) {
         require_recipient(_config.dapp_registry_account);
      }
   }

   extern "C" {
   void apply(uint64_t receiver, uint64_t code, uint64_t action) {
      if (code == receiver)
      {
         switch (action)
         {
            EOSIO_DISPATCH_HELPER( transfer_notifier, (setdappsacc) )
         }
      }
      else if (code == token_account.value && action == transfer_action.value) {
         execute_action( name(receiver), name(code), &transfer_notifier::transfer );
      }
   }
   }
} /// namespace eosio
