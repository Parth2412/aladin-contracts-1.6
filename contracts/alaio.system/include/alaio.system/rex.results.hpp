#pragma once

#include <alaiolib/alaio.hpp>
#include <alaiolib/name.hpp>
#include <alaiolib/asset.hpp>

using alaio::name;
using alaio::asset;
using alaio::action_wrapper;

class [[alaio::contract("rex.results")]] rex_results : alaio::contract {
   public:

      using alaio::contract::contract;

      [[alaio::action]]
      void buyresult( const asset& rex_received );

      [[alaio::action]]
      void sellresult( const asset& proceeds );

      [[alaio::action]]
      void orderresult( const name& owner, const asset& proceeds );

      [[alaio::action]]
      void rentresult( const asset& rented_tokens );

      using buyresult_action   = action_wrapper<"buyresult"_n,   &rex_results::buyresult>;
      using sellresult_action  = action_wrapper<"sellresult"_n,  &rex_results::sellresult>;
      using orderresult_action = action_wrapper<"orderresult"_n, &rex_results::orderresult>;
      using rentresult_action  = action_wrapper<"rentresult"_n,  &rex_results::rentresult>;
};
