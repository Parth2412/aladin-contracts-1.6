# Contract for paying tokens to DApp owners

## Permission setup

In order for this contract to work permission setup must be executed:

1. Every DApp owner must create permission "custom" and add <contract_account>@alaio.code to authorities of that permission
2. Created permission "custom" must be linked to <contract_account>::ontransfer action
3. <contract_account> must add <contract_account>@alaio.code to authorities of <contract_account>@active permission

#### Example permission setup

```
$ alacli set account permission <dapp_owner_account> custom '{"threshold": 1,"keys": [{"key": "<key for new permission>","weight": 1}], "accounts": [{"permission":{"actor":"<dapp_registry_account>","permission":"alaio.code"},"weight":1}]}' -p <dapp_owner_account>  
$ alacli set action permission <dapp_owner_account> <dapp_registry_account> ontransfer custom -p <dapp_owner_account>  
$ alacli set account permission <dapp_registry_account> active --add-code -p <dapp_registry_account>  
```