#include <eosio.system/eosio.system.hpp>
#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

   const int64_t  min_activated_stake   = 120'000'000'0000;
   const double   continuous_rate       = 0.04879;          // 5% annual rate
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

      _gstate2.last_block_num = timestamp;

      if( _gstate.total_activated_stake < min_activated_stake )
         return;

      if( _gstate.last_pervote_bucket_fill == time_point() )  /// start the presses
         _gstate.last_pervote_bucket_fill = current_time_point();

      auto prod = _producers.find( producer.value );
      if ( prod != _producers.end() ) {
         _gstate.total_unpaid_blocks++;
         _producers.modify( prod, same_payer, [&](auto& p ) {
               p.unpaid_blocks++;
         });
      }

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
         auto to_network = new_tokens / 2;         // storing the half new generated tokens into the to_network
         auto to_producers = to_network * 0.6;     // Giving the block producers 60%
         auto to_per_block_pay = to_producers;     // Giving the Block reward to the Block Producers
         auto to_witnesses = to_network * 0.15;    // Giving the witnesses 15%
         auto to_per_witness_pay = to_witnesses;   // Giving the Vote reward to the Block Producers and witnesses
         
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

         _gstate.perwitness_bucket       += to_per_witness_pay;
         _gstate.perblock_bucket         += to_per_block_pay;
         _gstate.last_pervote_bucket_fill = ct;
      }

      auto prod2 = _producers2.find( owner.value );

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

      int64_t producer_per_block_pay = 0;
      if( _gstate.total_unpaid_blocks > 0 ) {
         producer_per_block_pay = (_gstate.perblock_bucket * prod.unpaid_blocks) / _gstate.total_unpaid_blocks;
      }

      double new_votepay_share = update_producer_votepay_share( prod2,
                                    ct,
                                    updated_after_threshold ? 0.0 : prod.total_votes,
                                    true // reset votepay_share to zero after updating
                                 );

      int64_t witness_per_vote_pay = 0;
      if( witness_per_vote_pay == 0 ){
         witness_per_vote_pay = int64_t(_gstate.perwitness_bucket / counter);
      }

      _gstate.perwitness_bucket   -= witness_per_vote_pay;
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
   }

} //namespace eosiosystem
