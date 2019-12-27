#include <eosio.system/eosio.system.hpp>

#include <eosio.token/eosio.token.hpp>
#include <eosio.token/eosio.token.hpp>

#include <dapp_registry/dapp_registry.hpp>

namespace eosiosystem {

   // const int64_t  min_pervote_daily_pay         = 100'0000;
   const int64_t  min_pervote_daily_pay         = 1'0000;
   // const int64_t  min_activated_stake           = 120'000'000'0000;
   const int64_t  min_activated_stake           = 10'000'000'0000;
   const double   continuous_rate               = 0.04879;          // 5% annual rate
   // TODO move those constants into _gstate to make them configurable
   // Network rewards
   const double   network_rewards_rate          = 0.5;              // 50% of new tokens
   const double   dapps_rewards_rate            = 0.2;              // 20% of network rewards
   const double   bp_rewards_rate               = 0.6;              // 60% of network rewards
   const double   witnesses_rewards_rate        = 0.15;             // 15% of network rewards
   const double   vote_rewards_rate             = 0.05;             // 05% of network rewards 

   // Infrastructure rewards
   const double   infrastructure_rewards_rate   = 0.5;              // 50% of new tokens
   const double   community_rewards_rate        = 0.65;             // 65% of infrastructure rewards
   const double   marketing_rewards_rate        = 0.1;              // 10% of infrastructure rewards
   const double   founding_rewards_rate         = 0.25;             // 25% of infrastrcuture rewards

   // Oracle rewards
   const double   oracle_rewards_rate           = 0.02;             // 2% of Community rewards
   constexpr int64_t oracle_punishment_rate     = 1'0000;                // number of tokens per failed request

   const uint32_t blocks_per_year               = 52*7*24*2*3600;   // half seconds per year
   const uint32_t seconds_per_year              = 52*7*24*3600;
   const uint32_t blocks_per_day                = 2 * 24 * 3600;
   const uint32_t blocks_per_hour               = 2 * 3600;
   const int64_t  useconds_per_day              = 24 * 3600 * int64_t(1000000);
   const int64_t  useconds_per_year             = seconds_per_year*1000000ll;


   void system_contract::onblock( ignore<block_header> ) {
      using namespace eosio;

      require_auth(_self);

      block_timestamp timestamp;
      name producer;
      _ds >> timestamp >> producer;

      // _gstate2.last_block_num is not used anywhere in the system contract code anymore.
      // Although this field is deprecated, we will continue updating it for now until the last_block_num field
      // is eventually completely removed, at which point this line can be removed.
      _gstate2.last_block_num = timestamp;

      /** until activated stake crosses this threshold no new rewards are paid */
      if( _gstate.total_activated_stake < min_activated_stake )
         return;

      if( _gstate.last_pervote_bucket_fill == time_point() )  /// start the presses
         _gstate.last_pervote_bucket_fill = current_time_point();

      /**
       * At startup the initial producer may not be one that is registered / elected
       * and therefore there may be no producer object for them.
       */
      auto prod = _producers.find( producer.value );
      if ( prod != _producers.end() ) {
         _gstate.total_unpaid_blocks++;
         _producers.modify( prod, same_payer, [&](auto& p ) {
               p.unpaid_blocks++;
         });
      }

      /// only update block producers once every minute, block_timestamp is in half seconds
      if( timestamp.slot - _gstate.last_producer_schedule_update.slot > 120 ) {
         update_elected_producers( timestamp );

         if( (timestamp.slot - _gstate.last_name_close.slot) > blocks_per_day ) {
            name_bid_table bids(_self, _self.value);
            auto idx = bids.get_index<"highbid"_n>();
            auto highest = idx.lower_bound( std::numeric_limits<uint64_t>::max()/2 );
            if( highest != idx.end() &&
                highest->high_bid > 0 &&
                (current_time_point() - highest->last_bid_time) > microseconds(useconds_per_day) &&
                _gstate.thresh_activated_stake_time > time_point() &&
                (current_time_point() - _gstate.thresh_activated_stake_time) > microseconds(14 * useconds_per_day)
            ) {
               _gstate.last_name_close = timestamp;
               channel_namebid_to_rex( highest->high_bid );
               idx.modify( highest, same_payer, [&]( auto& b ){
                  b.high_bid = -b.high_bid;
               });
            }
         }
      }
   }

   using namespace eosio;
   void system_contract::claimrewards( const name owner ) {
      require_auth( owner );
      name producer;
      check( _gstate.total_activated_stake >= min_activated_stake, "cannot claim rewards until the chain is activated (at least 15% of all tokens participate in voting)" );
      
      // share inflation between buckets
      share_inflation();

      // pay rewards for producing blocks
      const auto& prod = _producers.get( owner.value );
      check( prod.active(), "producer does not have an active key" );

      const auto ct = current_time_point();
      // check( ct - prod.last_claim_time > microseconds(useconds_per_day), "already claimed rewards within past day" );

      auto prod2 = _producers2.find( owner.value );

      /// New metric to be used in pervote pay calculation. Instead of vote weight ratio, we combine vote weight and
      /// time duration the vote weight has been held into one metric.
      const auto last_claim_plus_3days = prod.last_claim_time + microseconds(3 * useconds_per_day);

      bool crossed_threshold       = (last_claim_plus_3days <= ct);
      bool updated_after_threshold = true;
      if ( prod2 != _producers2.end() ) {
         updated_after_threshold = (last_claim_plus_3days <= prod2->last_votepay_share_update);
      } else {
         prod2 = _producers2.emplace( owner, [&]( producer_info2& info  ) {
            info.owner                     = owner;
            info.last_votepay_share_update = ct;
         });
      }

      // Note: updated_after_threshold implies cross_threshold (except if claiming rewards when the producers2 table row did not exist).
      // The exception leads to updated_after_threshold to be treated as true regardless of whether the threshold was crossed.
      // This is okay because in this case the producer will not get paid anything either way.
      // In fact it is desired behavior because the producers votes need to be counted in the global total_producer_votepay_share for the first time.

      // Block Reward
      int64_t producer_per_block_pay = 0;
      if( _gstate.total_unpaid_blocks > 0 ) {
         producer_per_block_pay = (_gstate.perblock_bucket * prod.unpaid_blocks) / _gstate.total_unpaid_blocks;
      }

      double new_votepay_share = update_producer_votepay_share( prod2, ct, updated_after_threshold ? 0.0 : prod.total_votes, true); // reset votepay_share to zero after updating

      // Witness Reward
      int64_t producer_per_witness_pay = 0;
      auto idx = _producers.get_index<"prototalvote"_n>();
      const auto prod_it_by_votes = idx.find( owner.value );
      const auto position = std::distance( idx.begin(), prod_it_by_votes );
      bool is_witness = position < 50 && (prod_it_by_votes->total_votes / _gstate.total_producer_vote_weight) > 0.005;
      const auto prod3_it = _producers3.find( owner.value );
      check( prod3_it != _producers3.end(),"producer not found");
      double new_witness_share = update_producer_witnesspay_share( prod3_it, ct, (is_witness ? 1.0 : 0.0), true);
      print("==== new_witness_share ====",new_witness_share);
      double total_witness_share = update_total_witnesspay_share( ct );
      print("==== total_witness_share ====",total_witness_share);
      if( total_witness_share > 0 ) {
         producer_per_witness_pay = int64_t((new_witness_share * _gstate.perwitness_bucket) / total_witness_share);
         if( producer_per_witness_pay > _gstate.perwitness_bucket )
         producer_per_witness_pay = _gstate.perwitness_bucket;
         print("==== _gstate.perwitness_bucket ====",_gstate.perwitness_bucket);
      }
      update_total_witnesspay_share( ct, -new_witness_share, (is_witness ? 1.0 : 0.0) );


      // Oracle Reward
      int64_t producer_oracle_pay = 0;
      const auto oracle_it = _oracles.find( owner.value );
      if (oracle_it != _oracles.end()) {
         if( _oracle_state.total_successful_requests > 0 ) {
            producer_oracle_pay = int64_t((_gstate.oracle_bucket * oracle_it->successful_requests ) / _oracle_state.total_successful_requests);
         }

         int64_t punishment = oracle_it->pending_punishment + (oracle_it->failed_requests * oracle_punishment_rate);
         const auto oracle_pay_punishment = std::min(producer_oracle_pay, punishment);
         producer_oracle_pay -= oracle_pay_punishment;
         punishment -= oracle_pay_punishment;
         if (punishment > 0) {
            const auto per_block_pay_punishment = std::min(producer_per_block_pay, punishment);
            producer_per_block_pay -= per_block_pay_punishment;
            punishment -= per_block_pay_punishment;
         }

         _oracle_state.total_successful_requests -= oracle_it->successful_requests;
         _oracles.modify(oracle_it, same_payer, [&](auto& o) {
            o.successful_requests = 0;
            o.failed_requests     = 0;
            o.pending_punishment += punishment;
         });
      }

      _gstate.perwitness_bucket           -= producer_per_witness_pay;
      _gstate.perblock_bucket             -= producer_per_block_pay;
      _gstate.total_unpaid_blocks         -= prod.unpaid_blocks;
      _gstate.oracle_bucket               -= producer_oracle_pay;

      update_total_votepay_share( ct, -new_votepay_share, (updated_after_threshold ? prod.total_votes : 0.0) );
      _producers.modify( prod, same_payer, [&](auto& p) {
         p.last_claim_time       = ct;
         p.unpaid_blocks         = 0;
         // p.unpaid_witness_reward = 0;
      });

      // Sharing bpay reward
      if( producer_per_block_pay > 0 ) {
         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {bpay_account, active_permission}, {owner, active_permission} },
            { bpay_account, owner, asset(producer_per_block_pay, core_symbol()), std::string("producer block pay") }
         );
      }
      // Sharing wpay reward
      if( producer_per_witness_pay > 0 ) {
         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {wpay_account, active_permission}, {owner, active_permission} },
            { wpay_account, owner, asset(producer_per_witness_pay, core_symbol()), std::string("producer witness pay") }
         );
      }
      // Sharing oracle reward
      if( producer_oracle_pay > 0 ) {
         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {oracle_account, active_permission}, {owner, active_permission} },
            { oracle_account, owner, asset(producer_oracle_pay, core_symbol()), std::string("producer oracle pay") }
         );
      }


   }

   void system_contract::claimdapprwd( const name dapp_registry, const name owner, const name dapp ) {
      require_auth( owner );

      check( _gstate.total_activated_stake >= min_activated_stake, "cannot claim rewards until the chain is activated (at least 15% of all tokens participate in voting)" );

      // share inflation between buckets
      share_inflation();

      // ensure that we have corresponding record owner-app
      const auto dapp_info =  eosio::dapp_registry::get_dapp_info( dapp_registry, owner, dapp );
      
      // check if we are an app owner
      require_auth(dapp_info.owner);

      // check if reward could only be paid partially
      int64_t reward = 0;
      switch ( static_cast< eosio::dapp_registry::preference_type >( dapp_info.preference ) )
      {
         case eosio::dapp_registry::preference_type::TokenCirculation:
            reward = eosio::dapp_registry::get_dapp_rewards( dapp_registry, owner, dapp ) * _gstate.dapps_per_transfer_rewards_bucket;
            _gstate.dapps_per_transfer_rewards_bucket -= reward;
         break;

         case eosio::dapp_registry::preference_type::UniqueUsers:
            reward = eosio::dapp_registry::get_dapp_rewards( dapp_registry, owner, dapp ) * _gstate.dapps_per_user_rewards_bucket;
            _gstate.dapps_per_user_rewards_bucket -= reward;
         break;
      
         default:
            check( false, "Unknown DApp reward type" );
         break;
      }

      // delegate claim functionality to dapp_registry::claim
      INLINE_ACTION_SENDER(eosio::dapp_registry, claim)( dapp_registry, { {owner, active_permission} },{ owner, dapp, reward });

      // make transfer to dapp owner
      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {eosiosystem::system_contract::dpay_account, active_permission} },
         { eosiosystem::system_contract::dpay_account, dapp_info.owner, asset(reward, eosiosystem::system_contract::get_core_symbol()), std::string{"DApp reward transfer"} }
      );
   }

   void system_contract::claimvoterwd( const name owner ) {
      require_auth( owner );
      // share inflation between buckets
      share_inflation();

      const auto& voter = _voters.get( owner.value, "user must stake before they can vote" );
      const auto ct = current_time_point();
      check( ct - voter.last_claim_time > _gstate.vote_claim_period, "already claimed rewards within past month" );
      const int64_t reward = voter.unpaid_votes * _gstate.pervote_bucket / _gstate.total_unpaid_votes;
   
      check( reward > 0, "no rewards yet");

      // make transfer to voter
      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {eosiosystem::system_contract::vpay_account, active_permission} },
         { eosiosystem::system_contract::vpay_account, owner, asset(reward, eosiosystem::system_contract::get_core_symbol()), std::string{"Voter reward transfer"} }
      );

      // deduce paid rewards from voter, total_unpaid rewards and pervote_bucket
      _voters.modify( voter, same_payer, [&]( auto& av ) {
         _gstate.pervote_bucket     -= reward;
         _gstate.total_unpaid_votes -= av.unpaid_votes;
         av.unpaid_votes             = 0;
         av.last_claim_time          = ct;
      });
   }

   void system_contract::setvclaimprd( uint32_t period_in_days )
   {
      require_auth( get_self() );
      check( period_in_days >= 0, "claim period must be positive" );
      _gstate.vote_claim_period = eosio::days( period_in_days );
   }

   void system_contract::share_inflation() {
      const auto ct = current_time_point();
      const auto usecs_since_last_fill = (ct - _gstate.last_pervote_bucket_fill).count();

      // check if we filled bucked in last second
      if( usecs_since_last_fill <= 0 || _gstate.last_pervote_bucket_fill <= time_point() ) {
         return;
      }

      const asset token_supply   = eosio::token::get_supply(token_account, core_symbol().code() );
      const auto new_tokens = static_cast<int64_t>( (continuous_rate * double(token_supply.amount) * double(usecs_since_last_fill)) / double(useconds_per_year) );

      const auto network_rewards          = network_rewards_rate * new_tokens;
      const auto infrastructure_rewards   = infrastructure_rewards_rate * new_tokens;

      const auto to_producers             = bp_rewards_rate * network_rewards;

      const auto to_dapps                 = dapps_rewards_rate * network_rewards;
      const auto to_witnesses             = witnesses_rewards_rate * network_rewards;
      const auto to_voters                = vote_rewards_rate * network_rewards;
      
      const auto to_community             = community_rewards_rate * infrastructure_rewards;
      const auto to_marketing             = marketing_rewards_rate * infrastructure_rewards;
      const auto to_founding              = founding_rewards_rate * infrastructure_rewards;

      const auto to_oracle                = oracle_rewards_rate * to_community;

      INLINE_ACTION_SENDER(eosio::token, issue)(
         token_account, { {_self, active_permission} },
         { _self, asset(new_tokens, core_symbol()), std::string("issue tokens for producer pay and savings") }
      );

      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {_self, active_permission} },
         { _self, network_account, asset(network_rewards, core_symbol()), "For Network" }
      );

      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {_self, active_permission} },
         { _self, infrastructure_account, asset(infrastructure_rewards, core_symbol()), "For Infrastructure" }
      );

      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {infrastructure_account, active_permission} },
         { infrastructure_account, community_account, asset(to_community, core_symbol()), "For community fund" }
      );

      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {infrastructure_account, active_permission} },
         { infrastructure_account, marketing_account, asset(to_marketing, core_symbol()), "For marketing fund" }
      );

      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {infrastructure_account, active_permission} },
         { infrastructure_account, founding_account, asset(to_founding, core_symbol()), "For founding fund" }
      );

      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {community_account, active_permission} },
         { community_account, oracle_account, asset(to_oracle, core_symbol()), "For oracle fund" }
      );

      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {network_account, active_permission} },
         { network_account, bpay_account, asset(to_producers, core_symbol()), "For Block Producers fund" }
      );

      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {network_account, active_permission} },
         { network_account, dpay_account, asset(to_dapps, core_symbol()), "For DApp-Developers fund" }
      );

      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {network_account, active_permission} },
         { network_account, wpay_account, asset(to_witnesses, core_symbol()), "For Witnesses fund" }
      );

      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {network_account, active_permission} },
         { network_account, vpay_account, asset(to_voters, core_symbol()), "For Voters fund" }
      );

      _gstate.perblock_bucket                       += to_producers;
      _gstate.dapps_per_transfer_rewards_bucket     += 0.5 * to_dapps;
      _gstate.dapps_per_user_rewards_bucket         += 0.5 * to_dapps;
      _gstate.perwitness_bucket                     += to_witnesses;
      _gstate.pervote_bucket                        += to_voters;
      _gstate.oracle_bucket                         += to_oracle;
      _gstate.last_pervote_bucket_fill               = ct;
   }
} //namespace eosiosystem
