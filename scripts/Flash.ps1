[CmdletBinding()]
param([Parameter(Mandatory)][ValidateSet('Smoke','DisplayTest','Headless','OS')][string]$Mode,
      [switch]$PicoOnlyConfirmed,[switch]$DisplayHardwareConfirmed)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
New-Item -ItemType Directory -Path (Join-Path $projectRoot 'logs') -Force | Out-Null
$device=Get-Content (Join-Path $projectRoot 'config/device.json') -Raw | ConvertFrom-Json
$name=switch($Mode) {'Smoke' {'picoos_smoke'} 'DisplayTest' {'picoos_display_test'} 'Headless' {'picoos_rtos_test'} 'OS' {'M6_Reborn'}}
if($Mode -in @('Smoke','Headless') -and -not $PicoOnlyConfirmed) { throw 'Confirm Pico-only power arrangement before independent test' }
if($Mode -in @('DisplayTest','OS')) {
    if(-not $DisplayHardwareConfirmed) { throw 'Display electrical checks must be explicitly confirmed' }
    $checks=Get-Content (Join-Path $projectRoot 'logs\display-hardware-user-confirmation.json') -Raw | ConvertFrom-Json
    $backlightSafe=$checks.current_limited_backlight_verified -or $checks.backlight_disconnected
    if(-not($checks.actual_connector_pinout_confirmed -and $checks.isolated_from_original_processor -and $checks.vcc_3v3_suitable -and $backlightSafe)) { throw 'Incomplete display electrical checks: backlight must remain disconnected or have a verified current-limited driver' }
    $smoke=Get-Content (Join-Path $projectRoot 'logs\smoke-flash.json') -Raw | ConvertFrom-Json
    if(-not $smoke.runtime_verified) { throw 'First verify LED/USB smoke firmware' }
}
if($Mode -eq 'OS' -and -not(Test-Path (Join-Path $projectRoot 'logs\display-visible-confirmed.json'))) { throw 'Obtain visible display-test confirmation before full display OS' }
$image=Join-Path $projectRoot ('dist\'+$name+'.uf2')
. (Join-Path $PSScriptRoot 'Resolve-Python.ps1')
$python=Resolve-ProjectPython $projectRoot
& $python (Join-Path $PSScriptRoot 'Verify-Images.py') $image
if($LASTEXITCODE -ne 0) { throw 'Firmware verification failed' }
$drives=@(Get-CimInstance Win32_LogicalDisk | Where-Object VolumeName -eq 'RPI-RP2')
if($drives.Count -ne 1) { throw 'Expected exactly one RPI-RP2 drive; use BOOTSEL on the intended Pico' }
$pico=Get-PnpDevice -PresentOnly | Where-Object InstanceId -eq ('USB\VID_2E8A&PID_0003\'+$device.boot_serial)
if(-not $pico) { throw 'Previously identified Pico BOOTSEL serial not found' }
$driveRoot=$drives[0].DeviceID+'\'
$info=Get-Content -LiteralPath (Join-Path $driveRoot 'INFO_UF2.TXT') -Raw
if($info -notmatch 'Board-ID: RPI-RP2') { throw 'Unexpected BOOTSEL board' }
$hash=(Get-FileHash -LiteralPath $image -Algorithm SHA256).Hash
Copy-Item -LiteralPath $image -Destination (Join-Path $driveRoot ($name+'.uf2'))
[ordered]@{utc=[DateTime]::UtcNow.ToString('o');mode=$Mode;image=$image;sha256=$hash;boot_serial=$device.boot_serial;copied=$true;runtime_verified=$false;visible_output_verified=$false} |
    ConvertTo-Json | Set-Content (Join-Path $projectRoot ('logs\'+$Mode.ToLowerInvariant()+'-flash.json'))
Write-Output ('UF2 copied: '+$image+'; runtime and visible output require separate verification.')
