#include <eosio.system/eosio.system.hpp>

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

   //const int64_t  min_pervote_daily_pay = 100'0000;
   const int64_t  min_activated_stake   = 120'000'000'0000;
   const double   continuous_rate       = 0.04879;          // 5% annual rate
   // const double   perblock_rate         = 0.0025;         // 0.25%
   // const double   perblock_rate         = 0.015; // 1.5 For BP
   // const double   to_producers_percent  = 0.3;  // For Block Producers 60%
   // const double   standby_rate          = 0.0075;           // 0.75%
   const uint32_t blocks_per_year       = 52*7*24*2*3600;   // half seconds per year
   const uint32_t seconds_per_year      = 52*7*24*3600;
   const uint32_t blocks_per_day        = 2 * 24 * 3600;
   const uint32_t blocks_per_hour       = 2 * 3600;
   const int64_t  useconds_per_day      = 24 * 3600 * int64_t(1000000);
   const int64_t  useconds_per_year     = seconds_per_year*1000000ll;

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

      // Checking the BPs
      // if BPs have at least 0.5% of voting in their account 
      // and they have in the top 50 position or not
      
      // auto vote_restriction = _gstate.total_producer_vote_weight / 200;
      auto vote_restriction = _gstate.total_producer_vote_weight / 500;
      auto counter =0;
      //_gstate.last_producer_schedule_update = block_time;
      // code to get the counter for number of the producer that have .5% votes of the total votes for all 50 producer
      auto idx = _producers.get_index<"prototalvote"_n>();

      std::vector< std::pair<eosio::producer_key,uint16_t> > top_fifty_witnesses_producers;
      top_fifty_witnesses_producers.reserve(50);

      for ( auto it = idx.cbegin(); it != idx.cend() && top_fifty_witnesses_producers.size() <= 50 && vote_restriction <= it->total_votes && it->active(); ++it){
            counter ++;
        }

         const auto& prod = _producers.get( owner.value );
         //auto vitr = _producers.find( owner.value );
         check( prod.active(), "producer does not have an active key" );
         print(".....................................................................");
         print("number of the producer that has passed the vote restriction", counter);

      check( _gstate.total_activated_stake >= min_activated_stake, "cannot claim rewards until the chain is activated (at least 15% of all tokens participate in voting)" );

      const auto ct = current_time_point();

      //check( ct - prod.last_claim_time > microseconds(useconds_per_day), "already claimed rewards within past day" );

      const asset token_supply   = eosio::token::get_supply(token_account, core_symbol().code() );
      print("token_supply: ",token_supply);
      const auto usecs_since_last_fill = (ct - _gstate.last_pervote_bucket_fill).count();

      // if( usecs_since_last_fill > 0 && _gstate.last_pervote_bucket_fill > time_point() ) {
      if( continuous_rate == 0.04879 ) {
         auto new_tokens = static_cast<int64_t>( (continuous_rate * double(token_supply.amount) * double(usecs_since_last_fill)) / double(useconds_per_year) );
         print("================================/n");
         print("New Tokens: ",new_tokens);
         print("================================/n");
         
         //Original Producer calculation
         // auto to_producers     = new_tokens / 5;
         // auto to_savings       = new_tokens - to_producers;
         // auto to_per_block_pay = to_producers / 4;
         // Removing the share of votes 
         //auto to_per_vote_pay  = to_producers - to_per_block_pay; 

         //Aladin calculation for the network side
         // auto to_network = new_tokens / 2;
         // auto to_producer = to_network * 0.6;
         // auto to_standby = to_network * 0.15;
         // auto to_dappDev = to_network * 0.2;
         // auto to_voters = to_network * 0.05;

         //Aladin calculation for the infrastructure side
         // auto to_infrastructue = new_tokens - to_network;
         // auto to_community_development_team = to_infrastructure * 0.65; 
         // auto to_marketing_team = to_infrastructure * 0.1;              
         // auto to_founding_team = to_infrastructure * 0.25;              

         // 5% inflated tokens will be divided into two parts
         // 1. Network
         // 2. Infrastructure

         // 1. Division of Network
         //    a. Block Producer (60% + 15%)
         //    b. Witnesses (15%)
         auto to_network = new_tokens / 2;      // storing the half new generated tokens into the to_network
         auto to_producers = to_network * 0.6;  // Giving the block producers 60%
         auto to_per_block_pay = to_producers;  // Giving the Block reward to the Block Producers
         auto to_witnesses = to_network * 0.15; // Giving the witnesses 15%
         auto to_per_witness_pay = to_witnesses;   // Giving the Vote reward to the Block Producers and witnesses
         
         // 2. Division of Infrastructure
         //    a. Community Development Team
         //    b. Founding Team
         //    c. Marketing Team
         auto to_infrastructure = new_tokens - to_network;              // Storing the half new generated tokens for the infrastructure developement
         auto to_community_development_team = to_infrastructure * 0.65; // Giving the 65% from the infrastructure share to community team
         auto to_marketing_team = to_infrastructure * 0.1;              // Giving the 10% from the infrastructure share to marketing team
         auto to_founding_team = to_infrastructure * 0.25;              // Giving the 25% from the infrastructe share to founding team

         //Printing the values
         print("===============================To Network: ",to_network);
         print("===============================To Producers: ",to_producers); 
         print("===============================To Infrastructure: ",to_infrastructure);
         print("===============================To Per Block Pay: ",to_per_block_pay);
         print("===============================To Community Development Team: ",to_community_development_team);
         print("===============================To Marketing Team: ",to_marketing_team);
         print("===============================To Founding Team: ",to_founding_team);

         INLINE_ACTION_SENDER(eosio::token, issue)(
            token_account, { {_self, active_permission} },
            { _self, asset(new_tokens, core_symbol()), std::string("issue tokens for Network and Infrastructure") }
         );

         // 50% to Network
         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {_self, active_permission} },
            { _self, network_account, asset(to_network, core_symbol()), "For Network" }
         );

         // 50% to Infrastructure
         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {_self, active_permission} },
            { _self, infrastructure_account, asset(to_infrastructure, core_symbol()), "For Infrastucture Developement" }
         );

         // 65% to Community Development team
         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {infrastructure_account, active_permission} },
            { infrastructure_account, community_account, asset(to_community_development_team, core_symbol()), "For Community Developement Team" }
         );

         // 10% to Marketing team
         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {infrastructure_account, active_permission} },
            { infrastructure_account, marketing_account, asset(to_marketing_team, core_symbol()), "For Marketing Team" }
         );

         // 25% to Founding team
         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {infrastructure_account, active_permission} },
            { infrastructure_account, founding_account, asset(to_founding_team, core_symbol()), "For Founding Team" }
         );

         // Per Block Pay
         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {network_account, active_permission} },
            { network_account, bpay_account, asset(to_per_block_pay, core_symbol()), "Fund per-block bucket" }
         );

         // Per Witness Pay
         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {network_account, active_permission} },
            { network_account, wpay_account, asset(to_per_witness_pay, core_symbol()), "Fund per-witness bucket" }
         );

         // Per Vote Pay
         // INLINE_ACTION_SENDER(eosio::token, transfer)(
         //    token_account, { {network_account, active_permission} },
         //    { network_account, vpay_account, asset(to_per_vote_pay, core_symbol()), "Fund per-vote bucket" }
         // );

         _gstate.perwitness_bucket       += to_per_witness_pay;
         // _gstate.pervote_bucket          += to_per_vote_pay;
         _gstate.perblock_bucket         += to_per_block_pay;
         _gstate.last_pervote_bucket_fill = ct;
      }

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

      int64_t producer_per_block_pay = 0;
      if( _gstate.total_unpaid_blocks > 0 ) {
         producer_per_block_pay = (_gstate.perblock_bucket * prod.unpaid_blocks) / _gstate.total_unpaid_blocks;
         print("===============================producer_per_block_pay: ",producer_per_block_pay);
         print("===============================perblock bucket: ",_gstate.perblock_bucket);
         print("===============================prod.unpaid_blocks: ",prod.unpaid_blocks);
         print("===============================_gstate.total_unpaid_blocks:",_gstate.total_unpaid_blocks);
      }

      double new_votepay_share = update_producer_votepay_share( prod2,
                                    ct,
                                    updated_after_threshold ? 0.0 : prod.total_votes,
                                    true // reset votepay_share to zero after updating
                                 );

      int64_t witness_per_vote_pay = 0;
      if( witness_per_vote_pay == 0 ){
         witness_per_vote_pay = int64_t(_gstate.perwitness_bucket / counter);
         print("============================witness_per_vote_pay: ",witness_per_vote_pay);
         print("============================_gstate.perwitness_bucket: ",_gstate.perwitness_bucket);
         print("============================counter: ",counter);
      }

      // Original Code
         // int64_t producer_per_vote_pay = 0;
         // if( _gstate2.revision > 0 ) {
         //    double total_votepay_share = update_total_votepay_share( ct );
         //    if( total_votepay_share > 0 && !crossed_threshold ) {
         //       producer_per_vote_pay = int64_t((new_votepay_share * _gstate.pervote_bucket) / total_votepay_share);
         //       if( producer_per_vote_pay > _gstate.pervote_bucket )
         //          producer_per_vote_pay = _gstate.pervote_bucket;
         //    }
         // } else {
         //    if( _gstate.total_producer_vote_weight > 0 ) {
         //       producer_per_vote_pay = int64_t((_gstate.pervote_bucket * prod.total_votes) / _gstate.total_producer_vote_weight);
         //       print("============================_gstate.pervote_bucket: ",_gstate.pervote_bucket);
         //       print("============================prod.total_votes: ",prod.total_votes);
         //       print("============================_gstate.total_producer_vote_weight: ",_gstate.total_producer_vote_weight);
         //    }
         // }
      // Original Code End

      // int64_t producer_per_vote_pay = 0;
      // if( _gstate2.revision > 0 ) {
      //    double total_votepay_share = update_total_votepay_share( ct );
      //    if( total_votepay_share > 0 && !crossed_threshold ) {
      //       producer_per_vote_pay = int64_t((new_votepay_share * _gstate.pervote_bucket) / total_votepay_share);
      //       if( producer_per_vote_pay > _gstate.pervote_bucket )
      //          producer_per_vote_pay = _gstate.pervote_bucket;
      //    }
      // } else {
      //       if( _gstate.total_producer_vote_weight > 0 && prod.total_votes >= vote_restriction ) {
      //          producer_per_vote_pay = int64_t(_gstate.pervote_bucket / counter);
      //          print("----------------------------*******-----------------");
      //          print("producer per vote pay = ",producer_per_vote_pay);
      //          print("----------------------****-------------------");
      //          print("per vote bucket" , _gstate.pervote_bucket);
      //          print("-------------------****-------------------");
      //          print("total producer vote weight" , _gstate.total_producer_vote_weight);
      //       }
      //    }


      // if( producer_per_vote_pay < min_pervote_daily_pay ) {
      //    producer_per_vote_pay = 0;
      // }

      _gstate.perwitness_bucket   -= witness_per_vote_pay;
      // _gstate.pervote_bucket      -= producer_per_vote_pay;
      _gstate.perblock_bucket     -= producer_per_block_pay;
      _gstate.total_unpaid_blocks -= prod.unpaid_blocks;

      update_total_votepay_share( ct, -new_votepay_share, (updated_after_threshold ? prod.total_votes : 0.0) );

      _producers.modify( prod, same_payer, [&](auto& p) {
         p.last_claim_time = ct;
         p.unpaid_blocks   = 0;
      });

      if( producer_per_block_pay > 0 ) {
         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {bpay_account, active_permission}, {owner, active_permission} },
            { bpay_account, owner, asset(producer_per_block_pay, core_symbol()), std::string("producer block pay") }
         );
      }
      if( witness_per_vote_pay > 0 ) {
         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {wpay_account, active_permission}, {owner, active_permission} },
            { wpay_account, owner, asset(witness_per_vote_pay, core_symbol()), std::string("witness vote pay") }
         );
      }
      // if( producer_per_vote_pay > 0 ) {
      //    INLINE_ACTION_SENDER(eosio::token, transfer)(
      //       token_account, { {vpay_account, active_permission}, {owner, active_permission} },
      //       { vpay_account, owner, asset(producer_per_vote_pay, core_symbol()), std::string("producer vote pay") }
      //    );
      // }
   }

} //namespace eosiosystem
