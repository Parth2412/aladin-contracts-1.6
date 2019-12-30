#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/system.hpp>
#include <eosiolib/time.hpp>


namespace eosio {

   static constexpr eosio::name token_account   = "eosio.token"_n;
   static constexpr eosio::name transfer_action = "transfer"_n;

   class [[eosio::contract]] transfer_notifier : public contract {
   public:
      using contract::contract;

      transfer_notifier( name s, name code, datastream<const char*> ds );
      ~transfer_notifier();

      [[eosio::action]]
      void setdappsacc( const name& dapp_registry_account );

      [[eosio::action]]
      void transfer( const name& from, const name& to, const eosio::asset& amount, const std::string& memo );

   private:

      struct [[eosio::table("config")]] configuration {
         eosio::name dapp_registry_account;
      };
      typedef eosio::singleton< "config"_n, configuration > configuration_singleton;

      configuration_singleton _config_singleton;
      configuration           _config;
   };

} /// namespace eosio