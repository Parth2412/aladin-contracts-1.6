#pragma once

#include <alaiolib/action.hpp>
#include <alaiolib/public_key.hpp>
#include <alaiolib/print.hpp>
#include <alaiolib/privileged.h>
#include <alaiolib/producer_schedule.hpp>
#include <alaiolib/contract.hpp>
#include <alaiolib/ignore.hpp>

namespace alaiosystem {
   using alaio::name;
   using alaio::permission_level;
   using alaio::public_key;
   using alaio::ignore;

   struct permission_level_weight {
      permission_level  permission;
      uint16_t          weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      ALALIB_SERIALIZE( permission_level_weight, (permission)(weight) )
   };

   struct key_weight {
      alaio::public_key  key;
      uint16_t           weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      ALALIB_SERIALIZE( key_weight, (key)(weight) )
   };

   struct wait_weight {
      uint32_t           wait_sec;
      uint16_t           weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      ALALIB_SERIALIZE( wait_weight, (wait_sec)(weight) )
   };

   struct authority {
      uint32_t                              threshold = 0;
      std::vector<key_weight>               keys;
      std::vector<permission_level_weight>  accounts;
      std::vector<wait_weight>              waits;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      ALALIB_SERIALIZE( authority, (threshold)(keys)(accounts)(waits) )
   };

   struct block_header {
      uint32_t                                  timestamp;
      name                                      producer;
      uint16_t                                  confirmed = 0;
      capi_checksum256                          previous;
      capi_checksum256                          transaction_mroot;
      capi_checksum256                          action_mroot;
      uint32_t                                  schedule_version = 0;
      std::optional<alaio::producer_schedule>   new_producers;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      ALALIB_SERIALIZE(block_header, (timestamp)(producer)(confirmed)(previous)(transaction_mroot)(action_mroot)
                                     (schedule_version)(new_producers))
   };


   struct [[alaio::table("abihash"), alaio::contract("alaio.system")]] abi_hash {
      name              owner;
      capi_checksum256  hash;
      uint64_t primary_key()const { return owner.value; }

      ALALIB_SERIALIZE( abi_hash, (owner)(hash) )
   };

   /*
    * Method parameters commented out to prevent generation of code that parses input data.
    */
   class [[alaio::contract("alaio.system")]] native : public alaio::contract {
      public:

         using alaio::contract::contract;

         /**
          *  Called after a new account is created. This code enforces resource-limits rules
          *  for new accounts as well as new account naming conventions.
          *
          *  1. accounts cannot contain '.' symbols which forces all acccounts to be 12
          *  characters long without '.' until a future account auction process is implemented
          *  which prevents name squatting.
          *
          *  2. new accounts must stake a minimal number of tokens (as set in system parameters)
          *     therefore, this method will execute an inline buyram from receiver for newacnt in
          *     an amount equal to the current new account creation fee.
          */
         [[alaio::action]]
         void newaccount( name             creator,
                          name             name,
                          ignore<authority> owner,
                          ignore<authority> active);


         [[alaio::action]]
         void updateauth(  ignore<name>  account,
                           ignore<name>  permission,
                           ignore<name>  parent,
                           ignore<authority> auth ) {}

         [[alaio::action]]
         void deleteauth( ignore<name>  account,
                          ignore<name>  permission ) {}

         [[alaio::action]]
         void linkauth(  ignore<name>    account,
                         ignore<name>    code,
                         ignore<name>    type,
                         ignore<name>    requirement  ) {}

         [[alaio::action]]
         void unlinkauth( ignore<name>  account,
                          ignore<name>  code,
                          ignore<name>  type ) {}

         [[alaio::action]]
         void canceldelay( ignore<permission_level> canceling_auth, ignore<capi_checksum256> trx_id ) {}

         [[alaio::action]]
         void onerror( ignore<uint128_t> sender_id, ignore<std::vector<char>> sent_trx ) {}

         [[alaio::action]]
         void setabi( name account, const std::vector<char>& abi );

         [[alaio::action]]
         void setcode( name account, uint8_t vmtype, uint8_t vmversion, const std::vector<char>& code ) {}

         using newaccount_action = alaio::action_wrapper<"newaccount"_n, &native::newaccount>;
         using updateauth_action = alaio::action_wrapper<"updateauth"_n, &native::updateauth>;
         using deleteauth_action = alaio::action_wrapper<"deleteauth"_n, &native::deleteauth>;
         using linkauth_action = alaio::action_wrapper<"linkauth"_n, &native::linkauth>;
         using unlinkauth_action = alaio::action_wrapper<"unlinkauth"_n, &native::unlinkauth>;
         using canceldelay_action = alaio::action_wrapper<"canceldelay"_n, &native::canceldelay>;
         using setcode_action = alaio::action_wrapper<"setcode"_n, &native::setcode>;
         using setabi_action = alaio::action_wrapper<"setabi"_n, &native::setabi>;
   };
}
