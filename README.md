# Build

## Prerequisites
* ala v1.7.x
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
!!! Check paths to ALA binaries; !!!

You can skip this step if you have already setup blockchain network.

Current guide presume that you are executing commands from `ala/build` directory and ala binaries are located at `bin`.

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
  ./bin/alacli wallet create --to-console
  ```
* open and unlock it with the password generated in previous step 
  ```
  ./bin/alacli wallet open
  ./bin/alacli wallet unlock
  ```
* import developer private key 
  ```
  ./bin/alacli wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
  ```
* start a bios (genesis) node
```
  ./bin/alanode  --contracts-console \
              --blocks-dir ./testnet/node_bios/blocks \
              --config-dir ./testnet/node_bios \
              --data-dir ./testnet/node_bios \
              --http-server-address 127.0.0.1:8888 \
              --p2p-listen-endpoint 127.0.0.1:9000 \
              --enable-stale-production \
              --producer-name alaio \
              --signature-provider = ALA6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV=KEY:5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3 \
              --access-control-allow-origin='*' \
              --plugin alaio::http_plugin \
              --plugin alaio::chain_api_plugin \
              --plugin alaio::producer_api_plugin \
              --plugin alaio::producer_plugin \
              --plugin alaio::history_plugin \
              --plugin alaio::history_api_plugin >> ./testnet/node_bios/alanode.log 2>&1 &
```
* check genesis node state with 
  ```
  curl http://127.0.0.1:8888/v1/chain/get_info | json_pp
  ```
* create ALA essential accounts
```
./bin/alacli create account alaio alaio.ram    ALA6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/alacli create account alaio alaio.ramfee ALA6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/alacli create account alaio alaio.saving ALA6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/alacli create account alaio alaio.token  ALA6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/alacli create account alaio alaio.vpay   ALA6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/alacli create account alaio alaio.bpay   ALA6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/alacli create account alaio alaio.dpay   ALA6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/alacli create account alaio alaio.wpay   ALA6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/alacli create account alaio alaio.spay   ALA6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/alacli create account alaio alaio.names  ALA6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/alacli create account alaio alaio.stake  ALA6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./bin/alacli create account alaio alaio.rex    ALA6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
```
* Deploy essential contracts
```
./bin/alacli set contract alaio /devel/alaio-tokenomics/build/contracts/alaio.system --abi alaio.system.abi
./bin/alacli set contract alaio.token /devel/alaio-tokenomics/build/contracts/alaio.token --abi alaio.token.abi
```

## Issue initial supply
```
./bin/alacli push action alaio.token create '["alaio", "800000000.0000 ALA"]'         -p alaio.token
./bin/alacli push action alaio.token issue  '["alaio", "400000000.0000 ALA", "memo"]' -p alaio
./bin/alacli push action alaio init         '["0", "4,ALA"]'                          -p alaio@active
```

### Check reward buckets
If you have set up network correctly and activate 15% of the stake (e.g. staked it) you will see that rewards buckets are filling with rewards;
```
./bin/alacli get table alaio alaio global
```
At this step testnet is ready for deploying Fork-specific contracts

# Dapp Registry
## Deploy DApp Registry

Generate new key for Dapp Registry Account
```
./bin/alacli wallet create_key
```

Create owner account for Dapp registry
```
./bin/alacli system newaccount alaio dappregistry ALA61ZLadyKs3HUM8x2kXr3tbMfmxrhUBPLhiQ2Zjwdaew36cjGM6 --transfer --stake-net "5000.0000 ALA" --stake-cpu "5000.0000 ALA" --buy-ram "5000.0000 ALA"
```

Deploy Dapp Registry code to Dapp Registry Account
```
./bin/alacli set contract dappregistry /devel/alaio-tokenomics/build/contracts/dapp_registry --abi dapp_registry.abi
```

### Create Dummy App Account
Generate new key for Dummy App Account
```
./bin/alacli wallet create_key
```

Create owner account for Dummy App
```
./bin/alacli system newaccount alaio dappowner1 ALA8B4pgLkXZvwP8CdEkyMDgoTKfeksdZNCYUhGxXFoFAw92TeioR --transfer --stake-net "5000.0000 ALA" --stake-cpu "5000.0000 ALA" --buy-ram "5000.0000 ALA"
```

### Set permissions for Dummy App Account
Generate new key for custom permissions and set permissions
```
./bin/alacli wallet create_key

./bin/alacli set account permission dappowner1 custom '{"threshold": 1,"keys": [{"key": "ALA62tvhS9KFMTuh9RBZfUn9NN4k5jzw8m4bNgi7js1PDYdrm1LbZ","weight": 1}], "accounts": [{"permission":{"actor":"dappregistry","permission":"alaio.code"},"weight":1}]}' -p dappowner1  
./bin/alacli set action permission dappowner1 dappregistry ontransfer custom -p dappowner1  
./bin/alacli set account permission dappregistry active --add-code -p dappregistry
```

### Deploy Dummy App code to Dummy App Account
```
./bin/alacli set contract dappowner1 /devel/alaio-tokenomics/build/contracts/dapp_registry dummy_app.wasm --abi dummy_app.abi
```

### Set dappregistry
```
./bin/alacli push action dappowner1 setdappsacc '["dappregistry"]' -p dappowner1@active
```

check that registry is set correctly
```
./bin/alacli get table dappowner1 dappowner1 config
```


### Register Dapp in Dapp Registry
Only DApp Registry owner can add apps to registry
```
./bin/alacli push action dappregistry add '["dappowner1", "dummydap", 0]' -p dappregistry@active
```

you can check that app is added to registry
```
./bin/alacli get table dappregistry dappregistry dapps
```

you can check that dummy app owner account is linked to dummy app with the following command
```
./bin/alacli get table dappregistry dappregistry dappaccounts
```

# Create Dummy users
## Generate new key & Create accounts
```
./bin/alacli wallet create_key

./bin/alacli system newaccount alaio dappuser1 ALA5dXm8gVXp8H5NHoHaTQnUPTbAEcEThWzvj3B63YQUW7iJWEjJF --transfer --stake-net "5000.0000 ALA" --stake-cpu "5000.0000 ALA" --buy-ram "5000.0000 ALA" 
./bin/alacli system newaccount alaio dappuser2 ALA5dXm8gVXp8H5NHoHaTQnUPTbAEcEThWzvj3B63YQUW7iJWEjJF --transfer --stake-net "5000.0000 ALA" --stake-cpu "5000.0000 ALA" --buy-ram "5000.0000 ALA" 
./bin/alacli system newaccount alaio dappuser3 ALA5dXm8gVXp8H5NHoHaTQnUPTbAEcEThWzvj3B63YQUW7iJWEjJF --transfer --stake-net "5000.0000 ALA" --stake-cpu "5000.0000 ALA" --buy-ram "5000.0000 ALA" 
```

Users need a liquid tokens in order to do transfers to dummy dapp
```
./bin/alacli push action alaio.token transfer '["alaio", "dappuser1", "1000.0000 ALA", "tokens for testing dapp"]' -p alaio
```

# Check Dapp
Dummy App contains action `greetings` which accepts user name and greets him. You can check that app is working correctly if the following command will provide greetings to `dappuser1`
```
./bin/alacli push action dappowner1 greetings '["dappuser1"]' -p dappuser1@active
```

To check that rewards are calculated correctly you may do a transfer via `alaio.token::transfer` action. Following command should produce 10'0000 incoming transfer for dummy app
```
./bin/alacli push action alaio.token transfer '["dappuser1", "dappowner1", "10.0000 ALA", "dappuser1"]' -p dappuser1
```

You can check that rewards were correctly calculated with the following commands:

* this command will provide user with information about total incoming transfers\unique users of each dapp
  ```
  ./bin/alacli get table dappregistry dappregistry dapps
  ```

* this command will give an information about total number of incoming transfer\unique users
  ```
  ./bin/alacli get table dappregistry dappregistry config
  ```

# Claim Dapp rewards
Check dapps buckets
```
./bin/alacli get table alaio alaio global
```

```
./bin/alacli push action alaio claimdapprwd    '["dappregistry", "dappowner1", "dummydap"]' -p dappowner1@active
```

check that rewards were paid 
```
./bin/alacli get account dappowner1
```

## Claim Period
By default dapp claim period is 1 Month. You can modify it via `dappregistry::setclaimprd` action
```
./bin/alacli push action dappregistry setclaimprd '[1]' -p dappregistry@active
```
`./bin/alacli get account dappowner1`


# Voters & Producers
## Create new key & Create accounts
```
./bin/alacli wallet create_key

./bin/alacli system newaccount alaio producer111 ALA6ncxAFz9giRLm4NDPYHaCBVmfNVQ1UjxSXi2PtwnyuKLDWowJa --transfer --stake-net "5000.0000 ALA" --stake-cpu "5000.0000 ALA" --buy-ram "5000.0000 ALA" 
./bin/alacli system newaccount alaio producer222 ALA6ncxAFz9giRLm4NDPYHaCBVmfNVQ1UjxSXi2PtwnyuKLDWowJa --transfer --stake-net "5000.0000 ALA" --stake-cpu "5000.0000 ALA" --buy-ram "5000.0000 ALA" 
./bin/alacli system newaccount alaio producer333 ALA6ncxAFz9giRLm4NDPYHaCBVmfNVQ1UjxSXi2PtwnyuKLDWowJa --transfer --stake-net "5000.0000 ALA" --stake-cpu "5000.0000 ALA" --buy-ram "5000.0000 ALA" 
```

## Register producers
````
./bin/alacli system regproducer producer111 ALA6ncxAFz9giRLm4NDPYHaCBVmfNVQ1UjxSXi2PtwnyuKLDWowJa "https://blaize.tech"
./bin/alacli system regproducer producer222 ALA6ncxAFz9giRLm4NDPYHaCBVmfNVQ1UjxSXi2PtwnyuKLDWowJa "https://blaize.tech"
./bin/alacli system regproducer producer333 ALA6ncxAFz9giRLm4NDPYHaCBVmfNVQ1UjxSXi2PtwnyuKLDWowJa "https://blaize.tech"
```

```
./bin/alacli system listproducers
```

./bin/alacli system voteproducer approve dappuser1 producer111
./bin/alacli system voteproducer approve dappuser2 producer222
./bin/alacli system voteproducer approve dappuser3 producer333