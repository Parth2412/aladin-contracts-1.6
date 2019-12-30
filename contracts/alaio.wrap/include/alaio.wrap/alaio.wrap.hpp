#pragma once

#include <alaiolib/alaio.hpp>
#include <alaiolib/ignore.hpp>
#include <alaiolib/transaction.hpp>

namespace alaio {

   class [[alaio::contract("alaio.wrap")]] wrap : public contract {
      public:
         using contract::contract;

         [[alaio::action]]
         void exec( ignore<name> executer, ignore<transaction> trx );

         using exec_action = alaio::action_wrapper<"exec"_n, &wrap::exec>;
   };

} /// namespace alaio
