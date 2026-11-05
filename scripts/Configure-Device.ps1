[CmdletBinding()]
param(
    [string]$AppSerial,[string]$BootSerial,
    [switch]$ListDevices,
    [switch]$SmokeRuntimeVerified,
    [switch]$DisplayWiringVerified,
    [switch]$BacklightDisconnected,
    [switch]$CurrentLimitedBacklightVerified,
    [switch]$DisplayVisibleVerified
)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
if($ListDevices) {
    Get-PnpDevice -PresentOnly | Where-Object InstanceId -like 'USB\VID_2E8A*' | Select-Object FriendlyName,InstanceId
    return
}
if($AppSerial -or $BootSerial) {
    if($AppSerial -notmatch '^[A-Za-z0-9]+$' -or $BootSerial -notmatch '^[A-Za-z0-9]+$') { throw 'Provide both application and BOOTSEL serial values from your own Pico.' }
    @{app_serial=$AppSerial.ToUpperInvariant();boot_serial=$BootSerial.ToUpperInvariant()} | ConvertTo-Json | Set-Content (Join-Path $projectRoot 'config/device.json')
}
if(-not(Test-Path (Join-Path $projectRoot 'config/device.json'))) { throw 'Configure both device serials first; -ListDevices lists connected Raspberry Pi USB devices.' }
$logRoot=Join-Path $projectRoot 'logs'
New-Item -ItemType Directory -Path $logRoot -Force | Out-Null
$utc=[DateTime]::UtcNow.ToString('o')
if($SmokeRuntimeVerified) {
    @{utc=$utc;runtime_verified=$true;evidence='Operator confirmed onboard LED and SDK USB smoke output'} | ConvertTo-Json | Set-Content (Join-Path $logRoot 'smoke-flash.json')
}
if($DisplayWiringVerified) {
    if(-not($BacklightDisconnected -or $CurrentLimitedBacklightVerified)) { throw 'Confirm backlight disconnected or verified current-limited circuit.' }
    @{utc=$utc;actual_connector_pinout_confirmed=$true;isolated_from_original_processor=$true;vcc_3v3_suitable=$true;backlight_disconnected=[bool]$BacklightDisconnected;current_limited_backlight_verified=[bool]$CurrentLimitedBacklightVerified;evidence='Operator explicitly confirmed actual connector pinout, electrical isolation and 3.3V suitability'} | ConvertTo-Json | Set-Content (Join-Path $logRoot 'display-hardware-user-confirmation.json')
}
if($DisplayVisibleVerified) {
    if(-not(Test-Path (Join-Path $logRoot 'display-hardware-user-confirmation.json'))) { throw 'Verify wiring before recording visible display output.' }
    @{utc=$utc;visible_output_verified=$true;evidence='Operator confirmed visible display-test output'} | ConvertTo-Json | Set-Content (Join-Path $logRoot 'display-visible-confirmed.json')
}
Write-Output 'Local device configuration updated. Only record checks actually performed on this hardware.'
