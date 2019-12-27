#include <eosio.system/eosio.system.hpp>

#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/transaction.hpp>
#include <eosio.token/eosio.token.hpp>

#include <algorithm>
#include <cmath>

namespace eosiosystem {
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::singleton;
   using eosio::transaction;

   /**
    *  This method will create a producer_config and producer_info object for 'producer'
    *
    *  @pre producer is not already registered
    *  @pre producer to register is an account
    *  @pre authority of producer to register
    *
    */
   void system_contract::regproducer( const name producer, const eosio::public_key& producer_key, const std::string& url, uint16_t location ) {
      check( url.size() < 512, "url too long" );
      check( producer_key != eosio::public_key(), "public key should not be the default value" );
      require_auth( producer );
      auto producer_name = _voters.find(producer.value);
      check(producer_name -> staked > 1000000000, "You have 100'000 EOS tokens to become a Block Producer");
      auto prod = _producers.find( producer.value );
      const auto ct = current_time_point();

      if ( prod != _producers.end() ) {
         _producers.modify( prod, producer, [&]( producer_info& info ){
            info.producer_key = producer_key;
            info.is_active    = true;
            info.url          = url;
            info.location     = location;
            if ( info.last_claim_time == time_point() )
               info.last_claim_time = ct;
         });

         auto prod2 = _producers2.find( producer.value );
         if ( prod2 == _producers2.end() ) {
            _producers2.emplace( producer, [&]( producer_info2& info ){
               info.owner                     = producer;
               info.last_votepay_share_update = ct;
            });
            update_total_votepay_share( ct, 0.0, prod->total_votes );
            // When introducing the producer2 table row for the first time, the producer's votes must also be accounted for in the global total_producer_votepay_share at the same time.
         }
      } else {
         _producers.emplace( producer, [&]( producer_info& info ){
            info.owner           = producer;
            info.total_votes     = 0;
            info.producer_key    = producer_key;
            info.is_active       = true;
            info.url             = url;
            info.location        = location;
            info.last_claim_time = ct;
         });
         _producers2.emplace( producer, [&]( producer_info2& info ){
            info.owner                     = producer;
            info.last_votepay_share_update = ct;
         });
         _producers3.emplace( producer, [&]( producer_info3& info ){
            info.owner                     = producer;
            info.last_witnesspay_share_update = ct;
         });
      }
   }

   void system_contract::unregprod( const name producer ) {
      require_auth( producer );

      const auto& prod = _producers.get( producer.value, "producer not found" );
      _producers.modify( prod, same_payer, [&]( producer_info& info ){
         info.deactivate();
      });
   }

   void system_contract::update_elected_producers( block_timestamp block_time ) {
      _gstate.last_producer_schedule_update = block_time;

      auto idx = _producers.get_index<"prototalvote"_n>();

      std::vector< std::pair<eosio::producer_key,uint16_t> > top_producers;
      top_producers.reserve(21);

      for ( auto it = idx.cbegin(); it != idx.cend() && top_producers.size() < 21 && 0 < it->total_votes && it->active(); ++it ) {
         top_producers.emplace_back( std::pair<eosio::producer_key,uint16_t>({{it->owner, it->producer_key}, it->location}) );
      }

      if ( top_producers.size() < _gstate.last_producer_schedule_size ) {
         return;
      }

      /// sort by producer name
      std::sort( top_producers.begin(), top_producers.end() );

      std::vector<eosio::producer_key> producers;

      producers.reserve(top_producers.size());
      for( const auto& item : top_producers )
         producers.push_back(item.first);

      auto packed_schedule = pack(producers);

      if( set_proposed_producers( packed_schedule.data(),  packed_schedule.size() ) >= 0 ) {
         _gstate.last_producer_schedule_size = static_cast<decltype(_gstate.last_producer_schedule_size)>( top_producers.size() );
      }
   }

   double stake2vote( int64_t staked ) {
      /// TODO subtract 2080 brings the large numbers closer to this decade
      double weight = int64_t( (now() - (block_timestamp::block_timestamp_epoch / 1000)) / (seconds_per_day * 7) )  / double( 52 );
      return double(staked); //* std::pow( 2, weight );
   }

   double system_contract::update_total_votepay_share( time_point ct, double additional_shares_delta, double shares_rate_delta ){
      double delta_total_votepay_share = 0.0;
      if( ct > _gstate3.last_vpay_state_update ) {
         delta_total_votepay_share = _gstate3.total_vpay_share_change_rate
                                       * double( (ct - _gstate3.last_vpay_state_update).count() / 1E6 );
      }

      delta_total_votepay_share += additional_shares_delta;
      if( delta_total_votepay_share < 0 && _gstate2.total_producer_votepay_share < -delta_total_votepay_share ) {
         _gstate2.total_producer_votepay_share = 0.0;
      } else {
         _gstate2.total_producer_votepay_share += delta_total_votepay_share;
      }

      if( shares_rate_delta < 0 && _gstate3.total_vpay_share_change_rate < -shares_rate_delta ) {
         _gstate3.total_vpay_share_change_rate = 0.0;
      } else {
         _gstate3.total_vpay_share_change_rate += shares_rate_delta;
      }

      _gstate3.last_vpay_state_update = ct;

      return _gstate2.total_producer_votepay_share;
   }

   double system_contract::update_producer_votepay_share( const producers_table2::const_iterator& prod_itr, time_point ct, double shares_rate, bool reset_to_zero ){
      double delta_votepay_share = 0.0;
      if( shares_rate > 0.0 && ct > prod_itr->last_votepay_share_update ) {
         delta_votepay_share = shares_rate * double( (ct - prod_itr->last_votepay_share_update).count() / 1E6 ); // cannot be negative
      }

      double new_votepay_share = prod_itr->votepay_share + delta_votepay_share;
      _producers2.modify( prod_itr, same_payer, [&](auto& p) {
         if( reset_to_zero )
            p.votepay_share = 0.0;
         else
            p.votepay_share = new_votepay_share;

         p.last_votepay_share_update = ct;
      } );

      return new_votepay_share;
   }

   double system_contract::update_total_witnesspay_share( time_point ct, double additional_shares_delta, double shares_rate_delta ){
      double delta_total_witnesspay_share = 0.0;
      if( ct > _gstate4.last_wpay_state_update ) {
         delta_total_witnesspay_share = _gstate4.total_wpay_share_change_rate * double( (ct - _gstate4.last_wpay_state_update).count() / 1E6 );
      }

      delta_total_witnesspay_share += additional_shares_delta;
      if( delta_total_witnesspay_share < 0 && _gstate4.total_producer_witnesspay_share < -delta_total_witnesspay_share ) {
         _gstate4.total_producer_witnesspay_share = 0.0;
      } else {
         _gstate4.total_producer_witnesspay_share += delta_total_witnesspay_share;
      }

      if( shares_rate_delta < 0 && _gstate4.total_wpay_share_change_rate < -shares_rate_delta ) {
         _gstate4.total_wpay_share_change_rate = 0.0;
      } else {
         _gstate4.total_wpay_share_change_rate += shares_rate_delta;
      }

      _gstate4.last_wpay_state_update = ct;

      return _gstate4.total_producer_witnesspay_share;
   }

   double system_contract::update_producer_witnesspay_share( const producers_table3::const_iterator& prod3_itr, time_point ct, double shares_rate, bool reset_to_zero ){
      double delta_witnesspay_share = 0.0;
      if( shares_rate > 0.0 && ct > prod3_itr->last_witnesspay_share_update ) {
         delta_witnesspay_share = shares_rate * double( (ct - prod3_itr->last_witnesspay_share_update).count() / 1E6 ); // cannot be negative
      }

      double new_votepay_share = prod3_itr->witnesspay_share + delta_witnesspay_share;
      _producers3.modify( prod3_itr, same_payer, [&](auto& p) {
         if( reset_to_zero )
            p.witnesspay_share = 0.0;
         else
            p.witnesspay_share = new_votepay_share;

         p.last_witnesspay_share_update = ct;
      } );

      return new_votepay_share;
   }

   /**
    *  @pre producers must be sorted from lowest to highest and must be registered and active
    *  @pre if proxy is set then no producers can be voted for
    *  @pre if proxy is set then proxy account must exist and be registered as a proxy
    *  @pre every listed producer or proxy must have been previously registered
    *  @pre voter must authorize this action
    *  @pre voter must have previously staked some amount of CORE_SYMBOL for voting
    *  @pre voter->staked must be up to date
    *
    *  @post every producer previously voted for will have vote reduced by previous vote weight
    *  @post every producer newly voted for will have vote increased by new vote amount
    *  @post prior proxy will proxied_vote_weight decremented by previous vote weight
    *  @post new proxy will proxied_vote_weight incremented by new vote weight
    *
    *  If voting for a proxy, the producer votes will not change until the proxy updates their own vote.
    */
   void system_contract::voteproducer( const name voter_name, const name proxy, const std::vector<name>& producers ) {
      require_auth( voter_name );

      check( _producers.find( voter_name.value ) == _producers.end(), "Producers are not allowed to vote" );

      vote_stake_updater( voter_name );
      update_votes( voter_name, proxy, producers, true );
      auto rex_itr = _rexbalance.find( voter_name.value );
      if( rex_itr != _rexbalance.end() && rex_itr->rex_balance.amount > 0 ) {
         check_voting_requirement( voter_name, "voter holding REX tokens must vote for at least 21 producers or for a proxy" );
      }

      const auto& voter = _voters.get( voter_name.value, "user must stake before they can vote" );
      _voters.modify( voter, same_payer, [&]( auto& av ) {
         // av.unpaid_votes            += 1;
         // _gstate.total_unpaid_votes += 1;
         const auto vote_power = std::min( av.staked, _gstate.max_vote_power );
         av.unpaid_votes            += vote_power;
         _gstate.total_unpaid_votes += vote_power;
      });
   }

   void system_contract::update_votes( const name voter_name, const name proxy, const std::vector<name>& producers, bool voting ) {
      //validate input
      if ( proxy ) {
         check( producers.size() == 0, "cannot vote for producers and proxy at same time" );
         check( voter_name != proxy, "cannot proxy to self" );
      } else {
         check( producers.size() <= 30, "attempt to vote for too many producers" );
         for( size_t i = 1; i < producers.size(); ++i ) {
            check( producers[i-1] < producers[i], "producer votes must be unique and sorted" );
         }
      }

      auto voter = _voters.find( voter_name.value );
      check( voter != _voters.end(), "user must stake before they can vote" ); /// staking creates voter object
      check( !proxy || !voter->is_proxy, "account registered as a proxy is not allowed to use a proxy" );

      // We need to get top 50 producers with vote > 0.5% before and after update of total_votes
      const uint32_t max_witness_count = 50;
      std::vector<name> witnesses_before;
      std::vector<name> witnesses_after;
      witnesses_before.reserve(max_witness_count);
      witnesses_after.reserve(max_witness_count);
      auto idx = _producers.get_index<"prototalvote"_n>();
      for ( auto it = idx.cbegin(); it != idx.cend() && witnesses_before.size() < max_witness_count && (it->total_votes / _gstate.total_producer_vote_weight) > 0.005 && it->active(); ++it ) {
         witnesses_before.emplace_back( it->owner );
         eosio::print("+++++++++++++++++++++++++++++",it->owner);
      }
      std::sort(std::begin(witnesses_before), std::end(witnesses_before));
         
      /**
       * The first time someone votes we calculate and set last_vote_weight, since they cannot unstake until
       * after total_activated_stake hits threshold, we can use last_vote_weight to determine that this is
       * their first vote and should consider their stake activated.
       */
      if( voter->last_vote_weight <= 0.0 ) {
         _gstate.total_activated_stake += voter->staked;
         if( _gstate.total_activated_stake >= min_activated_stake && _gstate.thresh_activated_stake_time == time_point() ) {
            _gstate.thresh_activated_stake_time = current_time_point();
         }
      }

      // vote power has upper limit of _gstate.max_vote_power
      // auto new_vote_weight = stake2vote( std::max( voter->staked, _gstate.max_vote_power ) );
      auto new_vote_weight = stake2vote( std::min( voter->staked, _gstate.max_vote_power ) );
      if( voter->is_proxy ) {
         new_vote_weight += voter->proxied_vote_weight;
      }

      boost::container::flat_map<name, pair<double, bool /*new*/> > producer_deltas;
      if ( voter->last_vote_weight > 0 ) {
         if( voter->proxy ) {
            auto old_proxy = _voters.find( voter->proxy.value );
            check( old_proxy != _voters.end(), "old proxy not found" ); //data corruption
            _voters.modify( old_proxy, same_payer, [&]( auto& vp ) {
                  vp.proxied_vote_weight -= voter->last_vote_weight;
               });
            propagate_weight_change( *old_proxy );
         } else {
            for( const auto& p : voter->producers ) {
               auto& d = producer_deltas[p];
               d.first -= voter->last_vote_weight;
               d.second = false;
            }
         }
      }

      if( proxy ) {
         auto new_proxy = _voters.find( proxy.value );
         check( new_proxy != _voters.end(), "invalid proxy specified" ); //if ( !voting ) { data corruption } else { wrong vote }
         check( !voting || new_proxy->is_proxy, "proxy not found" );
         if ( new_vote_weight >= 0 ) {
            _voters.modify( new_proxy, same_payer, [&]( auto& vp ) {
                  vp.proxied_vote_weight += new_vote_weight;
               });
            propagate_weight_change( *new_proxy );
         }
      } else {
         if( new_vote_weight >= 0 ) {
            for( const auto& p : producers ) {
               auto& d = producer_deltas[p];
               d.first += new_vote_weight;
               d.second = true;
            }
         }
      }

      const auto ct = current_time_point();
      double delta_change_rate         = 0.0;
      double total_inactive_vpay_share = 0.0;
      for( const auto& pd : producer_deltas ) {
         auto pitr = _producers.find( pd.first.value );
         if( pitr != _producers.end() ) {
            check( !voting || pitr->active() || !pd.second.second /* not from new set */, "producer is not currently registered" );
            double init_total_votes = pitr->total_votes;
            _producers.modify( pitr, same_payer, [&]( auto& p ) {
               p.total_votes += pd.second.first;
               if ( p.total_votes < 0 ) { // floating point arithmetics can give small negative numbers
                  p.total_votes = 0;
               }
               _gstate.total_producer_vote_weight += pd.second.first;
               //check( p.total_votes >= 0, "something bad happened" );
            });
            auto prod2 = _producers2.find( pd.first.value );
            if( prod2 != _producers2.end() ) {
               const auto last_claim_plus_3days = pitr->last_claim_time + microseconds(3 * useconds_per_day);
               bool crossed_threshold       = (last_claim_plus_3days <= ct);
               bool updated_after_threshold = (last_claim_plus_3days <= prod2->last_votepay_share_update);
               // Note: updated_after_threshold implies cross_threshold

               double new_votepay_share = update_producer_votepay_share( prod2,
                                             ct,
                                             updated_after_threshold ? 0.0 : init_total_votes,
                                             crossed_threshold && !updated_after_threshold // only reset votepay_share once after threshold
                                          );

               if( !crossed_threshold ) {
                  delta_change_rate += pd.second.first;
               } else if( !updated_after_threshold ) {
                  total_inactive_vpay_share += new_votepay_share;
                  delta_change_rate -= init_total_votes;
               }
            }
         } else {
            check( !pd.second.second /* not from new set */, "producer is not registered" ); //data corruption
         }
      }

      for ( auto it = idx.cbegin(); it != idx.cend() && witnesses_after.size() < max_witness_count && (it->total_votes / _gstate.total_producer_vote_weight) > 0.005 && it->active(); ++it ) {
         witnesses_after.emplace_back( it->owner );
         eosio::print("=====================================",it->owner);
      }
      std::sort(std::begin(witnesses_after), std::end(witnesses_after));

      // After getting witnesses before and after update of total_votes 
      // we get producers that fell out of witness list and producers that got in witness list
      std::vector<name> witnesses_out;
      std::vector<name> witnesses_in;
      // eosio::print("ppppppppppppppppppppp",witnesses_before);
      
      // for(auto& prod: witnesses_after){
      //    eosio::print("aaaaaaaaaaaaaaaaaaaaa",prod);
      // }
      // for(auto& prod2: witnesses_before){
      //    eosio::print("bbbbbbbbbbbbbbbbbbbbbb",prod2);
      // }
      // std::set_difference(std::begin(witnesses_before), std::end(witnesses_before),
      //                      std::begin(witnesses_after), std::end(witnesses_after),
      //                      std::back_inserter(witnesses_out));

      // std::set_difference(std::begin(witnesses_after), std::end(witnesses_after),
      //                      std::begin(witnesses_before), std::end(witnesses_before),
      //                      std::back_inserter(witnesses_in));
      double witness_delta_change_rate = 0.0;
      for (const auto& wb: witnesses_before) {
         if (std::find(std::begin(witnesses_after), std::end(witnesses_after), wb) == std::end(witnesses_after)) {
            witnesses_out.push_back(wb);
            print("===== witness before ===",wb);
            auto pitr = _producers3.find( wb.value );
            check( pitr != _producers3.end(), "producer not found" );
            update_producer_witnesspay_share( pitr, ct, 1.0, false);
            witness_delta_change_rate += 1.0;
         }
      }
      for (const auto& wa: witnesses_after) {
         if (std::find(std::begin(witnesses_before), std::end(witnesses_before), wa) == std::end(witnesses_before)) {
            witnesses_in.push_back(wa);
            print("===== witness after ===",wa);
            auto pitr = _producers3.find( wa.value );
            check( pitr != _producers3.end(), "producer not found" );
            update_producer_witnesspay_share( pitr, ct, 0.0, false);
            witness_delta_change_rate -= 1.0;
         }
      }

      // After that we need to update share calculation mechanism
    //   for (const auto& witness: witnesses_out) {
    //      eosio::print("###############################################", witness,"\n");
    //      auto pitr = _producers3.find( witness.value );
    //      check( pitr != _producers3.end(), "producer not found" );
    //      update_producer_witnesspay_share( pitr, ct, 1.0, false); // 1.0 means that producer have had a share until this moment
    //      witness_delta_change_rate += 1.0;
    //   }
    //   for (const auto& witness: witnesses_in) {
    //      eosio::print("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@", witness,"\n");
    //      auto pitr = _producers3.find( witness.value );
    //      check( pitr != _producers3.end(), "producer not found" );
    //      update_producer_witnesspay_share( pitr, ct, 0.0, false); // 0.0 means that producer haven`t had a share until this moment
    //      witness_delta_change_rate -= 1.0;
    //   }
      update_total_witnesspay_share( ct, 0.0, witness_delta_change_rate );

      update_total_votepay_share( ct, -total_inactive_vpay_share, delta_change_rate );

      _voters.modify( voter, same_payer, [&]( auto& av ) {
         av.last_vote_weight = new_vote_weight;
         av.producers = producers;
         av.proxy     = proxy;
      });
   }

   /**
    *  An account marked as a proxy can vote with the weight of other accounts which
    *  have selected it as a proxy. Other accounts must refresh their voteproducer to
    *  update the proxy's weight.
    *
    *  @param isproxy - true if proxy wishes to vote on behalf of others, false otherwise
    *  @pre proxy must have something staked (existing row in voters table)
    *  @pre new state must be different than current state
    */
   void system_contract::regproxy( const name proxy, bool isproxy ) {
      require_auth( proxy );

      auto pitr = _voters.find( proxy.value );
      if ( pitr != _voters.end() ) {
         check( isproxy != pitr->is_proxy, "action has no effect" );
         check( !isproxy || !pitr->proxy, "account that uses a proxy is not allowed to become a proxy" );
         _voters.modify( pitr, same_payer, [&]( auto& p ) {
               p.is_proxy = isproxy;
            });
         const auto ct = current_time_point();
         const uint32_t max_witness_count = 50;
         std::vector<name> witnesses_before;
         std::vector<name> witnesses_after;
         witnesses_before.reserve(max_witness_count);
         witnesses_after.reserve(max_witness_count);
         auto idx = _producers.get_index<"prototalvote"_n>();
         for ( auto it = idx.cbegin(); it != idx.cend() && witnesses_before.size() < max_witness_count && (it->total_votes / _gstate.total_producer_vote_weight) > 0.005 && it->active(); ++it ) {
            witnesses_before.emplace_back( it->owner );
         }
         std::sort(std::begin(witnesses_before), std::end(witnesses_before));

         propagate_weight_change( *pitr );

         for ( auto it = idx.cbegin(); it != idx.cend() && witnesses_after.size() < max_witness_count && (it->total_votes / _gstate.total_producer_vote_weight) > 0.005 && it->active(); ++it ) {
            witnesses_after.emplace_back( it->owner );
         }
         std::sort(std::begin(witnesses_after), std::end(witnesses_after));

         // std::vector<name> witnesses_out;
         // std::vector<name> witnesses_in;
         // std::set_difference(std::begin(witnesses_before), std::end(witnesses_before),
         //                     std::begin(witnesses_after), std::end(witnesses_after),
         //                     std::back_inserter(witnesses_out));
         // std::set_difference(std::begin(witnesses_after), std::end(witnesses_after),
         //                     std::begin(witnesses_before), std::end(witnesses_before),
         //                     std::back_inserter(witnesses_in));

         double witness_delta_change_rate = 0.0;
         for (const auto& wb: witnesses_before) {
            if (std::find(std::begin(witnesses_after), std::end(witnesses_after), wb) == std::end(witnesses_after)) {
               // witnesses_out.push_back(wb);
               print("===== witness before ===",wb);
               auto pitr = _producers3.find( wb.value );
               check( pitr != _producers3.end(), "producer not found" );
               update_producer_witnesspay_share( pitr, ct, 1.0, false);
               witness_delta_change_rate += 1.0;
            }
         }
         for (const auto& wa: witnesses_after) {
            if (std::find(std::begin(witnesses_before), std::end(witnesses_before), wa) == std::end(witnesses_before)) {
               // witnesses_in.push_back(wa);
               print("===== witness after ===",wa);
               auto pitr = _producers3.find( wa.value );
               check( pitr != _producers3.end(), "producer not found" );
               update_producer_witnesspay_share( pitr, ct, 0.0, false);
               witness_delta_change_rate -= 1.0;
            }
         }

        //  for (const auto& witness: witnesses_out) {
        //     auto pitr = _producers3.find( witness.value );
        //     check( pitr != _producers3.end(), "producer not found" );
        //     update_producer_witnesspay_share( pitr, ct, 1.0, false);
        //     witness_delta_change_rate += 1.0;
        //  }
        //  for (const auto& witness: witnesses_in) {
        //     auto pitr = _producers3.find( witness.value );
        //     check( pitr != _producers3.end(), "producer not found" );
        //     update_producer_witnesspay_share( pitr, ct, 0.0, false);
        //     witness_delta_change_rate -= 1.0;
        //  }

         update_total_witnesspay_share( ct, 0.0, witness_delta_change_rate );

      } else {
         _voters.emplace( proxy, [&]( auto& p ) {
               p.owner  = proxy;
               p.is_proxy = isproxy;
            });
      }
   }

   void system_contract::propagate_weight_change( const voter_info& voter ) {
      check( !voter.proxy || !voter.is_proxy, "account registered as a proxy is not allowed to use a proxy" );
      
      // vote power has upper limit of _gstate.max_vote_power
      // auto new_weight = stake2vote( std::max( voter.staked, _gstate.max_vote_power ) );
      auto new_weight = stake2vote( std::min( voter.staked, _gstate.max_vote_power ) );
      if ( voter.is_proxy ) {
         new_weight += voter.proxied_vote_weight;
      }

      /// don't propagate small changes (1 ~= epsilon)
      if ( fabs( new_weight - voter.last_vote_weight ) > 1 )  {
         if ( voter.proxy ) {
            auto& proxy = _voters.get( voter.proxy.value, "proxy not found" ); //data corruption
            _voters.modify( proxy, same_payer, [&]( auto& p ) {
                  p.proxied_vote_weight += new_weight - voter.last_vote_weight;
               }
            );
            propagate_weight_change( proxy );
         } else {
            auto delta = new_weight - voter.last_vote_weight;
            const auto ct = current_time_point();
            double delta_change_rate         = 0;
            double total_inactive_vpay_share = 0;
            for ( auto acnt : voter.producers ) {
               auto& prod = _producers.get( acnt.value, "producer not found" ); //data corruption
               const double init_total_votes = prod.total_votes;
               _producers.modify( prod, same_payer, [&]( auto& p ) {
                  p.total_votes += delta;
                  _gstate.total_producer_vote_weight += delta;
               });
               auto prod2 = _producers2.find( acnt.value );
               if ( prod2 != _producers2.end() ) {
                  const auto last_claim_plus_3days = prod.last_claim_time + microseconds(3 * useconds_per_day);
                  bool crossed_threshold       = (last_claim_plus_3days <= ct);
                  bool updated_after_threshold = (last_claim_plus_3days <= prod2->last_votepay_share_update);
                  // Note: updated_after_threshold implies cross_threshold

                  double new_votepay_share = update_producer_votepay_share( prod2,
                                                ct,
                                                updated_after_threshold ? 0.0 : init_total_votes,
                                                crossed_threshold && !updated_after_threshold // only reset votepay_share once after threshold
                                             );

                  if( !crossed_threshold ) {
                     delta_change_rate += delta;
                  } else if( !updated_after_threshold ) {
                     total_inactive_vpay_share += new_votepay_share;
                     delta_change_rate -= init_total_votes;
                  }
               }
            }

            update_total_votepay_share( ct, -total_inactive_vpay_share, delta_change_rate );
         }
      }
      _voters.modify( voter, same_payer, [&]( auto& v ) {
            v.last_vote_weight = new_weight;
         }
      );
   }

} /// namespace eosiosystem