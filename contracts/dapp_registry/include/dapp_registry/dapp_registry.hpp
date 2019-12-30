#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/system.hpp>
#include <eosiolib/time.hpp>

#include <eosio.system/eosio.system.hpp>
#include <eosio.token/eosio.token.hpp>

#include <set>


namespace eosio {
//TODO: change permission name or use active if it is OK to let this contract`s code act on behalf of dapp.owner@active
static constexpr eosio::name dapp_owner_permission = "custom"_n;
static constexpr eosio::name active_permission     = "active"_n;
static constexpr eosio::name token_account         = eosiosystem::system_contract::token_account;


class [[eosio::contract]] dapp_registry : public contract {
   struct [[eosio::table("config")]] configuration {
      double reward_rate = 1.0;
      microseconds claim_period = eosio::days( 30 );

      int64_t total_unpaid_users = 0;
      int64_t total_unpaid_transactions = 0;
   };
   typedef eosio::singleton< "config"_n, configuration > configuration_singleton;


   struct [[eosio::table]] dapp_info {
      name           dapp_name;
      name           owner;
      int16_t        preference;
      std::set<name> users;
      int64_t        incoming_transfers_volume = 0;
      int64_t        total_earned = 0;
      time_point     last_claim_time;

      uint64_t primary_key() const { return dapp_name.value; }
      uint64_t by_owner() const { return owner.value; }
   };
   typedef eosio::multi_index< "dapps"_n, dapp_info,
      indexed_by<"owner"_n, const_mem_fun<dapp_info, uint64_t, &dapp_info::by_owner>  >
   > dapp_info_table;


   struct [[eosio::table]] dapp_accounts_info {
      name account;
      name dapp_name;

      uint64_t primary_key() const { return account.value; }
      uint64_t by_dapp() const { return dapp_name.value; }
   };
   typedef eosio::multi_index< "dappaccounts"_n, dapp_accounts_info,
      indexed_by<"dapp"_n, const_mem_fun<dapp_accounts_info, uint64_t, &dapp_accounts_info::by_dapp>  >
   > dapp_accounts_info_table;

public:
   enum class preference_type : int16_t { TokenCirculation = 0, UniqueUsers, MaxVal };
   
public:
   using contract::contract;

   dapp_registry( name s, name code, datastream<const char*> ds );
   ~dapp_registry();

   [[eosio::action]]
   void add( const name& owner, const name& dapp_name, int16_t preference );

   [[eosio::action]]
   void remove( const name& dapp_name );

   [[eosio::action]]
   void linkacc( const name& dapp_name, const name& account );

   [[eosio::action]]
   void unlinkacc( const name& account );

   /* 
   * This is handler for token transfers
   * DApps must implement handler for eosio.token::transfer, too
   * and use redirect transfer to this contract`s account
   */
   [[eosio::action]]
   void transfer( const name& from, const name& to, const eosio::asset& amount, const std::string& memo );

   [[eosio::action]]
   void ontransfer( const name& dapp_name, const name& user, const eosio::asset& amount );

   [[eosio::action]]
   void claim( const name& owner, const name& dapp_name, int64_t paid_rewards );

   [[eosio::action]]
   void setrewrate( uint32_t rate );

   [[eosio::action]]
   void setclaimprd( uint32_t period_in_days );

   using ontransfer_action = eosio::action_wrapper<"ontransfer"_n, &dapp_registry::ontransfer>;
   using claim_action      = eosio::action_wrapper<"claim"_n, &dapp_registry::claim>;

   static dapp_info get_dapp_info( name contract_account, name dapp_owner, name dapp_name )
   {
      dapp_info_table dapps( contract_account, contract_account.value );
      const auto& dapp = dapps.get( dapp_name.value, "DApp with this name doesn`t exist" );

      return dapp;
   }

   static double get_dapp_rewards( name contract_account, name dapp_owner, name dapp_name )
   {
      const auto& dapp = get_dapp_info(contract_account, dapp_owner, dapp_name);
      configuration_singleton config(contract_account, contract_account.value );

      double reward = 0.0;
      // calculate rewards based on preference
      if (dapp.preference == static_cast<int16_t>( preference_type::TokenCirculation )) {
         check( config.get().total_unpaid_transactions > 0, "No unpaid rewards");
         reward = config.get().reward_rate * dapp.incoming_transfers_volume /  config.get().total_unpaid_transactions;
      }
      else if (dapp.preference == static_cast<int16_t>( preference_type::UniqueUsers )) {
         check( config.get().total_unpaid_transactions > 0, "No unpaid rewards");
         reward = config.get().reward_rate * dapp.users.size() / config.get().total_unpaid_users;
      }

      return reward;
   }

private:
   /*
   * Checks if preference type is valid
   * otherwise asserts
   */
   void check_preference(int16_t p) const;


   configuration_singleton _config_singleton;
   configuration           _config;
};

} /// namespace eosio