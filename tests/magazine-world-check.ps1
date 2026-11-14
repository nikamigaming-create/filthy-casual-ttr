& "$PSScriptRoot\magazine-world-pouch.ps1" -Weapon 0x8F21E -Rounds 13
. "$PSScriptRoot\runtime-helpers.ps1"
foreach($key in @('_FC_PhysicalReload_v1','_FC_PhysicalReloadWeapon','_FC_PhysicalReloadAmmo')){
 Assert-Equal (Get-Native ('RefMapSize "'+$key+'"')) 0 "Consumed parts leave no metadata in $key"
}
