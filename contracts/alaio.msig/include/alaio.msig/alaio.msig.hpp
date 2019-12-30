#pragma once
#include <alaiolib/alaio.hpp>
#include <alaiolib/ignore.hpp>
#include <alaiolib/transaction.hpp>

namespace alaio {

   class [[alaio::contract("alaio.msig")]] multisig : public contract {
      public:
         using contract::contract;

         [[alaio::action]]
         void propose(ignore<name> proposer, ignore<name> proposal_name,
               ignore<std::vector<permission_level>> requested, ignore<transaction> trx);
         [[alaio::action]]
         void approve( name proposer, name proposal_name, permission_level level,
                       const alaio::binary_extension<alaio::checksum256>& proposal_hash );
         [[alaio::action]]
         void unapprove( name proposer, name proposal_name, permission_level level );
         [[alaio::action]]
         void cancel( name proposer, name proposal_name, name canceler );
         [[alaio::action]]
         void exec( name proposer, name proposal_name, name executer );
         [[alaio::action]]
         void invalidate( name account );

         using propose_action = alaio::action_wrapper<"propose"_n, &multisig::propose>;
         using approve_action = alaio::action_wrapper<"approve"_n, &multisig::approve>;
         using unapprove_action = alaio::action_wrapper<"unapprove"_n, &multisig::unapprove>;
         using cancel_action = alaio::action_wrapper<"cancel"_n, &multisig::cancel>;
         using exec_action = alaio::action_wrapper<"exec"_n, &multisig::exec>;
         using invalidate_action = alaio::action_wrapper<"invalidate"_n, &multisig::invalidate>;
      private:
         struct [[alaio::table]] proposal {
            name                            proposal_name;
            std::vector<char>               packed_transaction;

            uint64_t primary_key()const { return proposal_name.value; }
         };

         typedef alaio::multi_index< "proposal"_n, proposal > proposals;

         struct [[alaio::table]] old_approvals_info {
            name                            proposal_name;
            std::vector<permission_level>   requested_approvals;
            std::vector<permission_level>   provided_approvals;

            uint64_t primary_key()const { return proposal_name.value; }
         };
         typedef alaio::multi_index< "approvals"_n, old_approvals_info > old_approvals;

         struct approval {
            permission_level level;
            time_point       time;
         };

         struct [[alaio::table]] approvals_info {
            uint8_t                 version = 1;
            name                    proposal_name;
            //requested approval doesn't need to cointain time, but we want requested approval
            //to be of exact the same size ad provided approval, in this case approve/unapprove
            //doesn't change serialized data size. So, we use the same type.
            std::vector<approval>   requested_approvals;
            std::vector<approval>   provided_approvals;

            uint64_t primary_key()const { return proposal_name.value; }
         };
         typedef alaio::multi_index< "approvals2"_n, approvals_info > approvals;

         struct [[alaio::table]] invalidation {
            name         account;
            time_point   last_invalidation_time;

            uint64_t primary_key() const { return account.value; }
         };

         typedef alaio::multi_index< "invals"_n, invalidation > invalidations;
   };

} /// namespace alaio
