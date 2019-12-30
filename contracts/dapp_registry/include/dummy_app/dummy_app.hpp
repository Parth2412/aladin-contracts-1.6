#pragma once

#include <alaiolib/asset.hpp>
#include <alaiolib/alaio.hpp>
#include <alaiolib/singleton.hpp>
#include <alaiolib/system.hpp>
#include <alaiolib/time.hpp>

namespace alaio {
static constexpr alaio::name token_account   = "alaio.token"_n;
static constexpr alaio::name transfer_action = "transfer"_n;

class [[alaio::contract]] dummy_app : public contract {
public:
   using contract::contract;
   
   dummy_app( name s, name code, datastream<const char*> ds );
   ~dummy_app();

   [[alaio::action]]
   void setdappsacc( const name& dapp_registry_account );

   [[alaio::action]]
   void transfer( const name& from, const name& to, const alaio::asset& amount, const std::string& memo );

   [[alaio::action]]
   void greetings( name user );

private:
   struct [[alaio::table("config")]] configuration {
      alaio::name dapp_registry_account;
   };
   typedef alaio::singleton< "config"_n, configuration > configuration_singleton;

   configuration_singleton _config_singleton;
   configuration           _config;
};

} /// namespace alaio