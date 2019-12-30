# Build

## Prerequisites
* eos v1.7.x
* cdt v1.5.x
* cmake 3.5+

## Build script
```
mkdir build && cd build
cmake ..
make -j`nproc`
```

## Artifacts
*.abi & *.wasm files are located at build/contracts/


# Setup

## Setup local testnet
!!! Check paths to EOS binaries; !!!

You can skip this step if you have already setup blockchain network.

Current guide presume that you are executing commands from `eos/build` directory and eos binaries are located at `bin`.

* create testnet directory 
  ```
  rm -r ./testnet &&  mkdir -p ./testnet/wallet ./testnet/node_bios
  ```
* start a wallet service 
  ```
  ./bin/kalad --wallet-dir `pwd`/testnet/wallet --http-server-address 127.0.0.1:8899 --unlock-timeout=86400 &
  ```
* create a new wallet called "default" 
  ```
  ./bin/cleos wallet create --to-console
  ```
* open and unlock it with the password generated in previous step 
  ```
  ./bin/cleos wallet open
  ./bin/cleos wallet unlock
  ```
* import developer private key 
  ```
  ./bin/cleos wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
  ```
* start a bios (genesis) node
```
  ./bin/nodeos  --contracts-console \
              --blocks-dir ./testnet/node_bios/blocks \
              --config-dir ./testnet/node_bios \
              --data-dir ./testnet/node_bios \
              --http-server-address 127.0.0.1:8888 \
              --p2p-listen-endpoint 127.0.0.1:9000 \
              --enable-stale-production \
              --producer-name eosio \
              --signature-provider = EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV=KEY:5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3 \
              --access-control-allow-origin='*' \
              --plugin eosio::http_plugin \
              --plugin eosio::chain_api_plugin \
              --plugin eosio::producer_api_plugin \
              --plugin eosio::producer_plugin \
              --plugin eosio::history_plugin \
              --plugin eosio::history_api_plugin >> ./testnet/node_bios/nodeos.log 2>&1 &
```
* check genesis node state with 
  ```
  curl http://127.0.0.1:8888/v1/chain/get_info | json_pp
  ```
* create EOS essential accounts
```
./bin/cleos create account eosio eosio.ram    EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/cleos create account eosio eosio.ramfee EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/cleos create account eosio eosio.saving EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/cleos create account eosio eosio.token  EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/cleos create account eosio eosio.vpay   EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/cleos create account eosio eosio.bpay   EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/cleos create account eosio eosio.dpay   EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/cleos create account eosio eosio.wpay   EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/cleos create account eosio eosio.spay   EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/cleos create account eosio eosio.names  EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/cleos create account eosio eosio.stake  EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/cleos create account eosio eosio.rex    EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
```
* Deploy essential contracts
```
./bin/cleos set contract eosio /devel/eosio-tokenomics/build/contracts/eosio.system --abi eosio.system.abi
./bin/cleos set contract eosio.token /devel/eosio-tokenomics/build/contracts/eosio.token --abi eosio.token.abi
```

## Issue initial supply
```
./bin/cleos push action eosio.token create '["eosio", "800000000.0000 EOS"]'         -p eosio.token
./bin/cleos push action eosio.token issue  '["eosio", "400000000.0000 EOS", "memo"]' -p eosio
./bin/cleos push action eosio init         '["0", "4,EOS"]'                          -p eosio@active
```

### Check reward buckets
If you have set up network correctly and activate 15% of the stake (e.g. staked it) you will see that rewards buckets are filling with rewards;
```
./bin/cleos get table eosio eosio global
```
At this step testnet is ready for deploying Fork-specific contracts

# Dapp Registry
## Deploy DApp Registry

Generate new key for Dapp Registry Account
```
./bin/cleos wallet create_key
```

Create owner account for Dapp registry
```
./bin/cleos system newaccount eosio dappregistry EOS61ZLadyKs3HUM8x2kXr3tbMfmxrhUBPLhiQ2Zjwdaew36cjGM6 --transfer --stake-net "5000.0000 EOS" --stake-cpu "5000.0000 EOS" --buy-ram "5000.0000 EOS"
```

Deploy Dapp Registry code to Dapp Registry Account
```
./bin/cleos set contract dappregistry /devel/eosio-tokenomics/build/contracts/dapp_registry --abi dapp_registry.abi
```

### Create Dummy App Account
Generate new key for Dummy App Account
```
./bin/cleos wallet create_key
```

Create owner account for Dummy App
```
./bin/cleos system newaccount eosio dappowner1 EOS8B4pgLkXZvwP8CdEkyMDgoTKfeksdZNCYUhGxXFoFAw92TeioR --transfer --stake-net "5000.0000 EOS" --stake-cpu "5000.0000 EOS" --buy-ram "5000.0000 EOS"
```

### Set permissions for Dummy App Account
Generate new key for custom permissions and set permissions
```
./bin/cleos wallet create_key

./bin/cleos set account permission dappowner1 custom '{"threshold": 1,"keys": [{"key": "EOS62tvhS9KFMTuh9RBZfUn9NN4k5jzw8m4bNgi7js1PDYdrm1LbZ","weight": 1}], "accounts": [{"permission":{"actor":"dappregistry","permission":"eosio.code"},"weight":1}]}' -p dappowner1  
./bin/cleos set action permission dappowner1 dappregistry ontransfer custom -p dappowner1  
./bin/cleos set account permission dappregistry active --add-code -p dappregistry
```

### Deploy Dummy App code to Dummy App Account
```
./bin/cleos set contract dappowner1 /devel/eosio-tokenomics/build/contracts/dapp_registry dummy_app.wasm --abi dummy_app.abi
```

### Set dappregistry
```
./bin/cleos push action dappowner1 setdappsacc '["dappregistry"]' -p dappowner1@active
```

check that registry is set correctly
```
./bin/cleos get table dappowner1 dappowner1 config
```


### Register Dapp in Dapp Registry
Only DApp Registry owner can add apps to registry
```
./bin/cleos push action dappregistry add '["dappowner1", "dummydap", 0]' -p dappregistry@active
```

you can check that app is added to registry
```
./bin/cleos get table dappregistry dappregistry dapps
```

you can check that dummy app owner account is linked to dummy app with the following command
```
./bin/cleos get table dappregistry dappregistry dappaccounts
```

# Create Dummy users
## Generate new key & Create accounts
```
./bin/cleos wallet create_key

./bin/cleos system newaccount eosio dappuser1 EOS5dXm8gVXp8H5NHoHaTQnUPTbAEcEThWzvj3B63YQUW7iJWEjJF --transfer --stake-net "5000.0000 EOS" --stake-cpu "5000.0000 EOS" --buy-ram "5000.0000 EOS" 
./bin/cleos system newaccount eosio dappuser2 EOS5dXm8gVXp8H5NHoHaTQnUPTbAEcEThWzvj3B63YQUW7iJWEjJF --transfer --stake-net "5000.0000 EOS" --stake-cpu "5000.0000 EOS" --buy-ram "5000.0000 EOS" 
./bin/cleos system newaccount eosio dappuser3 EOS5dXm8gVXp8H5NHoHaTQnUPTbAEcEThWzvj3B63YQUW7iJWEjJF --transfer --stake-net "5000.0000 EOS" --stake-cpu "5000.0000 EOS" --buy-ram "5000.0000 EOS" 
```

Users need a liquid tokens in order to do transfers to dummy dapp
```
./bin/cleos push action eosio.token transfer '["eosio", "dappuser1", "1000.0000 EOS", "tokens for testing dapp"]' -p eosio
```

# Check Dapp
Dummy App contains action `greetings` which accepts user name and greets him. You can check that app is working correctly if the following command will provide greetings to `dappuser1`
```
./bin/cleos push action dappowner1 greetings '["dappuser1"]' -p dappuser1@active
```

To check that rewards are calculated correctly you may do a transfer via `eosio.token::transfer` action. Following command should produce 10'0000 incoming transfer for dummy app
```
./bin/cleos push action eosio.token transfer '["dappuser1", "dappowner1", "10.0000 EOS", "dappuser1"]' -p dappuser1
```

You can check that rewards were correctly calculated with the following commands:

* this command will provide user with information about total incoming transfers\unique users of each dapp
  ```
  ./bin/cleos get table dappregistry dappregistry dapps
  ```

* this command will give an information about total number of incoming transfer\unique users
  ```
  ./bin/cleos get table dappregistry dappregistry config
  ```

# Claim Dapp rewards
Check dapps buckets
```
./bin/cleos get table eosio eosio global
```

```
./bin/cleos push action eosio claimdapprwd    '["dappregistry", "dappowner1", "dummydap"]' -p dappowner1@active
```

check that rewards were paid 
```
./bin/cleos get account dappowner1
```

## Claim Period
By default dapp claim period is 1 Month. You can modify it via `dappregistry::setclaimprd` action
```
./bin/cleos push action dappregistry setclaimprd '[1]' -p dappregistry@active
```
`./bin/cleos get account dappowner1`


# Voters & Producers
## Create new key & Create accounts
```
./bin/cleos wallet create_key

./bin/cleos system newaccount eosio producer111 EOS6ncxAFz9giRLm4NDPYHaCBVmfNVQ1UjxSXi2PtwnyuKLDWowJa --transfer --stake-net "5000.0000 EOS" --stake-cpu "5000.0000 EOS" --buy-ram "5000.0000 EOS" 
./bin/cleos system newaccount eosio producer222 EOS6ncxAFz9giRLm4NDPYHaCBVmfNVQ1UjxSXi2PtwnyuKLDWowJa --transfer --stake-net "5000.0000 EOS" --stake-cpu "5000.0000 EOS" --buy-ram "5000.0000 EOS" 
./bin/cleos system newaccount eosio producer333 EOS6ncxAFz9giRLm4NDPYHaCBVmfNVQ1UjxSXi2PtwnyuKLDWowJa --transfer --stake-net "5000.0000 EOS" --stake-cpu "5000.0000 EOS" --buy-ram "5000.0000 EOS" 
```

## Register producers
````
./bin/cleos system regproducer producer111 EOS6ncxAFz9giRLm4NDPYHaCBVmfNVQ1UjxSXi2PtwnyuKLDWowJa "https://blaize.tech"
./bin/cleos system regproducer producer222 EOS6ncxAFz9giRLm4NDPYHaCBVmfNVQ1UjxSXi2PtwnyuKLDWowJa "https://blaize.tech"
./bin/cleos system regproducer producer333 EOS6ncxAFz9giRLm4NDPYHaCBVmfNVQ1UjxSXi2PtwnyuKLDWowJa "https://blaize.tech"
```

```
./bin/cleos system listproducers
```

./bin/cleos system voteproducer approve dappuser1 producer111
./bin/cleos system voteproducer approve dappuser2 producer222
./bin/cleos system voteproducer approve dappuser3 producer333