#include <alaio.system/alaio.system.hpp>


namespace alaiosystem {
   static constexpr uint16_t max_api_count  = 10;
   static const microseconds request_period = alaio::minutes( 5 );


   void system_contract::addrequest( uint64_t request_id, const alaio::name& caller, const std::vector<api>& apis, uint16_t response_type, uint16_t aggregation_type, uint16_t prefered_api, std::string string_to_count )
   {
      require_auth( caller );

      check( apis.size() <= max_api_count, "number of endpoints is too large" );
      for (const auto& api: apis) {
         check( api.endpoint.substr(0, 5) == std::string("https"), "invalid endpoint" );
         check( !api.json_field.empty(), "empty json field" );
         //TODO: add aggregation type check
      }
      check_response_type( response_type );

      request_info_table requests( get_self(), caller.value );
      check( requests.find( request_id ) == requests.end(), "request with this id already exists" );

      const auto [assigned_oracle, standby_oracle] = get_current_oracle();
      auto idx = _oracles.get_index<"oracleacc"_n>();
      const auto it = idx.find(assigned_oracle.value);
      idx.modify(it, same_payer, [](auto& o) {
         o.pending_requests++;
      });

      requests.emplace( caller, [&, assigned=assigned_oracle, standby=standby_oracle]( auto& r ) {
         r.id               = request_id;
         r.time             = current_time_point();
         r.assigned_oracle  = assigned;
         r.standby_oracle   = standby;
         r.apis             = apis;
         r.response_type    = response_type;
         r.aggregation_type = aggregation_type;
         r.prefered_api     = prefered_api;
         r.string_to_count  = string_to_count;
      });
   }

   void system_contract::reply( const alaio::name& caller, uint64_t request_id, const std::vector<char>& response )
   {
      const auto ct = current_time_point();
      request_info_table requests( get_self(), caller.value );
      const auto& request = requests.get(request_id, "request is missing");
      bool first_timeframe = false;
      bool second_timeframe = false;
      bool timeout = false;
      if (ct - request.time < request_period) {
         require_auth(request.assigned_oracle); // if reply received within the first period then it must be signed by assigned_oracle
         first_timeframe = true;
      }
      else if (ct - request.time < (request_period + request_period)) {
         require_auth(request.standby_oracle); // if reply is received within the second period then it must be signed by standby_oracle
         second_timeframe = true;
      }
      else { // if request is timed out, anyone can call reply but with empty response
         check( response.empty(), "only empty response is allowed if request is timed out" );
         timeout = true;
      }

      name succeeded; // account that successfully executed request (it is either assigned oracle or standby)
      auto idx = _oracles.get_index<"oracleacc"_n>();
      auto it = idx.find(request.assigned_oracle.value);
      if (it != idx.end()) {
         if (first_timeframe) {
            succeeded = request.assigned_oracle;
         }
         idx.modify(it, same_payer, [&](auto& o) {
            o.pending_requests--;
            if (!first_timeframe) {
               o.failed_requests++;
            }
         });
      }

      if (second_timeframe) {
         succeeded = request.standby_oracle;
      }
      else if (timeout && request.standby_oracle) {
         auto it = idx.find(request.standby_oracle.value);
         if (it != idx.end()) {
            idx.modify(it, same_payer, [&](auto &o) {
               o.failed_requests++;
            });
         }
      }

      if (succeeded) {
         // increment successful request counter for oracle that have successfully executed request
         auto it = idx.find(succeeded.value);
         if (it != idx.end()) {
            idx.modify(it, same_payer, [&](auto &o) {
               o.successful_requests++;
            });
            check( _oracle_state.total_successful_requests != std::numeric_limits<decltype(_oracle_state.total_successful_requests)>::max(),
               "total_successful_requests overflow" );
            _oracle_state.total_successful_requests++;
         }
      }

      require_recipient(caller);

      requests.erase(request);
   }

   void system_contract::setoracle( const alaio::name& producer, const alaio::name& oracle )
   {
      require_auth(producer);

      check( _producers.find(producer.value) != _producers.end(), "only block producer is allowed to set it`s oracle" );

      const auto ct = current_time_point();
      auto it = _oracles.find(producer.value);
      if (it == _oracles.end()) {
         _oracles.emplace(producer, [&](auto& o) {
            o.producer                 = producer;
            o.oracle_account           = oracle;
            o.pending_requests         = 0;
            o.successful_requests      = 0;
            o.failed_requests          = 0;
            o.pending_punishment       = 0;
         });
      }
      else {
         _oracles.modify(it, same_payer, [&](auto& o) {
            o.oracle_account = oracle;
         });
      }
   }


   void system_contract::check_response_type(uint16_t t) const
   {
      check( t >= 0 && t < static_cast<uint16_t>( response_type::MaxVal ), "response type is out of range" );
   }

   std::pair<name, name> system_contract::get_current_oracle() const
   {
      auto idx = _producers.get_index<"prototalvote"_n>();
      std::vector<std::pair<name, uint32_t>> active_oracles; // oracle name and it`s pending requests counter
      for ( auto it = idx.cbegin(); it != idx.cend() && std::distance(idx.cbegin(), it) < 21 && 0 < it->total_votes && it->active(); ++it ) {
         auto oracle_it = _oracles.find(it->owner.value);
         if (oracle_it != _oracles.end() && oracle_it->is_active()) {
            active_oracles.emplace_back(oracle_it->oracle_account, oracle_it->pending_requests);
         }
      }
      check( !active_oracles.empty(), "noone from top21 has active oracle" );
      //sort oracles by pending requests in descending order
      std::sort( active_oracles.begin(), active_oracles.end(),
                 [](const std::pair<name, uint32_t>& l, const std::pair<name, uint32_t>& r) { return l.second > r.second; } );

      //Calculate oracle based on current block time and number of pending requests
      uint32_t value = 10; //the higher value the higher probability for oracle
      uint32_t total = value;
      std::vector<uint32_t> v{ total }; // vector of increasing values (probability ranges).
      // v { 10, 20, 30 } means that first oracle has probability range [0, 10), second - [10, 20), third - [20, 30)
      // so if seed value x is in range [0, 10) than the first oracle choosen, [10, 20) - second, [20, 30) - third
      for (size_t i = 1; i < active_oracles.size(); i++) {
         if (active_oracles[i - 1].second > active_oracles[i].second) {
            value++; //increase probability if oracle has less pending requests than previous one
         }
         total += value;
         v.push_back(total);
      }

      std::pair<name, name> ret;
      auto x = current_block_time().slot % total;
      auto it = std::find_if(std::begin(v), std::end(v), [x](const auto& val) { return x < val; });
      check( it != std::end(v), "shouldn`t happen" );
      auto index = std::distance(std::begin(v), it);
      ret.first = active_oracles[index].first;

      // delete choosen oracle because standby oracle must be a different one
      if (index == 0) {
         total -= 10;
      }
      else {
         total -= (v[index] - v[index - 1]);
      }
      v.erase(it);
      active_oracles.erase(active_oracles.begin() + index);
      if (!active_oracles.empty()) {
         x = current_block_time().slot % total;
         it = std::find_if(std::begin(v), std::end(v), [x](const auto& val) { return x < val; });
         check( it != std::end(v), "shouldn`t happen" );
         index = std::distance(std::begin(v), it);
         ret.second = active_oracles[index].first;
      }
      return ret;
   }
} //namespace alaiosystem