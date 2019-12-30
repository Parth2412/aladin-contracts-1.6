#include <dapp_registry/dapp_registry.hpp>


namespace alaio {

   dapp_registry::dapp_registry( name s, name code, datastream<const char*> ds )
   : alaio::contract(s, code, ds),
     _config_singleton(get_self(), get_self().value)
   {
      _config = _config_singleton.get_or_create(_self, configuration{
         .reward_rate  = 1,
         .claim_period = alaio::days( 1 )
      });
   }

   dapp_registry::~dapp_registry()
   {
      _config_singleton.set( _config, get_self() );
   }


   /*
    * Creates DApp record with provided dapp_name, owner and preference (0 - token circulation, 1 - usage)
    */
   void dapp_registry::add( const name& owner, const name& dapp_name, int16_t preference )
   {
      require_auth( get_self() );

      check_preference( preference );

      dapp_info_table dapps( get_self(), get_self().value );
      check( dapps.find( dapp_name.value ) == dapps.end(), "DApp with this name already exists" );

      dapps.emplace( get_self(), [&]( auto& d ) {
         d.dapp_name       = dapp_name;
         d.owner           = owner;
         d.preference      = preference;
         d.last_claim_time = time_point( microseconds{ static_cast<int64_t>( current_time() ) } );
      });

      linkacc(dapp_name, owner);
   }

   /*
    * Removes DApp if there are no accounts linked to this DApp
    */
   void dapp_registry::remove( const name& dapp_name )
   {
      require_auth( get_self() );

      dapp_info_table dapps( get_self(), get_self().value );
      const auto& dapp = dapps.get( dapp_name.value, "DApp with this name doesn`t exist" );

      // Check that all accounts were unlinked before deletion
      dapp_accounts_info_table dapp_accounts( get_self(), get_self().value );
      const auto dapp_accounts_idx = dapp_accounts.get_index<"dapp"_n>();
      const auto it = dapp_accounts_idx.find( dapp_name.value );
      check( it == dapp_accounts_idx.end(), "DApp has linked accounts; you should unlink them first" );

      dapps.erase( dapp );
   }

   /*
    * Link account to DApp
    * Notifications received from linked accounts will update transfer/usage counts for respective DApp
    */
   void dapp_registry::linkacc( const name& dapp_name, const name& account )
   {
      require_auth( get_self() );

      dapp_info_table dapps( get_self(), get_self().value );
      check( dapps.find( dapp_name.value ) != dapps.end(), "DApp with this name doesn`t exist" );

      dapp_accounts_info_table dapp_accounts( get_self(), get_self().value );
      check( dapp_accounts.find( account.value ) == dapp_accounts.end(), "account belongs to different DApp" );
      dapp_accounts.emplace( get_self(), [&]( auto& a ) {
         a.account   = account;
         a.dapp_name = dapp_name;
      });
   }
   void dapp_registry::unlinkacc( const name& account )
   {
      require_auth( get_self() );

      dapp_accounts_info_table dapp_accounts( get_self(), get_self().value );
      const auto& acc = dapp_accounts.get( account.value, "account isn`t linked to any DApp" );
      dapp_accounts.erase( acc );
   }

   void dapp_registry::transfer( const name& from, const name& to, const alaio::asset& amount, const std::string& memo )
   {
      // Skip transfers to and from this account
      // because we are tracking only transfers to DApps
      // Skip transfer from rewards account
      if (from == get_self() || to == get_self() || from == alaiosystem::system_contract::dpay_account) {
         return;
      }

      dapp_accounts_info_table dapp_accounts( get_self(), get_self().value );
      // Try to find account which is receiving transfer in DApp accounts registry ...
      const auto acc_it = dapp_accounts.find( to.value );

      check( acc_it != dapp_accounts.end(), "Owner has not registred dapps" );

      // ... if found then find the DApp to which it is linked ...
      dapp_info_table dapps( get_self(), get_self().value );
      const auto& dapp = dapps.get( acc_it->dapp_name.value, "DApp with this name doesn`t exist" );

      // ... and execute action that will update information about transactions and users
      ontransfer_action ontransfer_act( get_self(), { { dapp.owner, dapp_owner_permission }, { get_self(), active_permission } } );
      ontransfer_act.send(acc_it->dapp_name, from, amount);
   }

   void dapp_registry::ontransfer( const name& dapp_name, const name& user, const alaio::asset& amount )
   {
      dapp_info_table dapps( get_self(), get_self().value );
      const auto& dapp = dapps.get( dapp_name.value, "DApp with this name doesn`t exist" );

      // This action must only be executed from contract code
      // inline actions are executed with alaio.code permission
      require_auth(dapp.owner); // we need dapp owner authority in order to charge him for RAM update (tracking of unique users)
      require_auth(get_self()); // we also need this contract`s authority to be sure that this action is called by this contract

      dapps.modify(dapp, dapp.owner, [&](auto& d) {
         if (dapp.preference == static_cast<int16_t>( preference_type::TokenCirculation )) {
            d.incoming_transfers_volume += amount.amount;
            _config.total_unpaid_transactions += amount.amount;
         } else if (dapp.preference == static_cast<int16_t>( preference_type::UniqueUsers )) {
            const auto new_user = d.users.insert(user);
            _config.total_unpaid_users += new_user.second;
         }
      });
   }

   void dapp_registry::claim( const name& owner, const name& dapp_name, int64_t paid_rewards )
   {
      require_auth(owner);
      check( paid_rewards > 0, "no rewards yet" );

      dapp_info_table dapps( get_self(), get_self().value );
      const auto& dapp = dapps.get( dapp_name.value, "DApp with this name doesn`t exist" );
      check( dapp.owner == owner, "cannot claim DApp rewards of another owner" );
      check( time_point( microseconds{ static_cast<int64_t>( current_time() ) } ) - dapp.last_claim_time > _config.claim_period, "already claimed dapp rewards within past month" );

      // reset transfer and user tracking
      dapps.modify(dapp, dapp.owner, [&](auto& d) {
         d.total_earned    += paid_rewards;
         d.last_claim_time  = time_point( microseconds{ static_cast<int64_t>( current_time() ) } );
         
         // reduce unpaid transactions only if rewards are based on transaction volume
         // else if rewards are based on unique users reset list of unique users
         if (dapp.preference == static_cast<int16_t>( preference_type::TokenCirculation )) {
            _config.total_unpaid_transactions -= d.incoming_transfers_volume;
            d.incoming_transfers_volume        = 0;
         } else if (dapp.preference == static_cast<int16_t>( preference_type::UniqueUsers )) {
            _config.total_unpaid_users -= d.users.size();
            d.users.clear();
         }
      });
   }

   void dapp_registry::setrewrate( uint32_t rate )
   {
      require_auth( get_self() );

      check( rate > 0, "reward rate must be positive" );
      _config.reward_rate = rate / 1'0000.0;
   }

   void dapp_registry::setclaimprd( uint32_t period_in_days )
   {
      require_auth( get_self() );

      check( period_in_days >= 0, "claim period must be positive" );
      _config.claim_period = alaio::days( period_in_days );
   }

   void dapp_registry::check_preference(int16_t p) const
   {
      check( p >= 0 && p < static_cast<int16_t>( preference_type::MaxVal ), "preference type is out of range" );
   }


   extern "C" {
   void apply(uint64_t receiver, uint64_t code, uint64_t action) {
      if (code == receiver)
      {
         switch (action)
         {
            ALAIO_DISPATCH_HELPER( dapp_registry, (add)(remove)(linkacc)(unlinkacc)(ontransfer)(claim)(setrewrate)(setclaimprd) )
         }
      }
      else if (code == token_account.value && action == "transfer"_n.value) {
         execute_action( name(receiver), name(code), &dapp_registry::transfer );
      }
   }
   }
} /// namespace alaio
