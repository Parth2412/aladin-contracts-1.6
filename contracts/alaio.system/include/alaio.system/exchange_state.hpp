#pragma once

#include <alaiolib/asset.hpp>

namespace alaiosystem {
   using alaio::asset;
   using alaio::symbol;

   typedef double real_type;

   /**
    *  Uses Bancor math to create a 50/50 relay between two asset types. The state of the
    *  bancor exchange is entirely contained within this struct. There are no external
    *  side effects associated with using this API.
    */
   struct [[alaio::table, alaio::contract("alaio.system")]] exchange_state {
      asset    supply;

      struct connector {
         asset balance;
         double weight = .5;

         ALALIB_SERIALIZE( connector, (balance)(weight) )
      };

      connector base;
      connector quote;

      uint64_t primary_key()const { return supply.symbol.raw(); }

      asset convert_to_exchange( connector& c, asset in );
      asset convert_from_exchange( connector& c, asset in );
      asset convert( asset from, const symbol& to );

      ALALIB_SERIALIZE( exchange_state, (supply)(base)(quote) )
   };

   typedef alaio::multi_index< "rammarket"_n, exchange_state > rammarket;

} /// namespace alaiosystem
