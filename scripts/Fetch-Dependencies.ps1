[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$packages=@(
 @{Name='pico-sdk';Tag='2.1.1';Url='https://github.com/raspberrypi/pico-sdk.git';Commit='bddd20f928ce76142793bef434d4f75f4af6e433'},
 @{Name='FreeRTOS-Kernel';Tag='V11.2.0';Url='https://github.com/FreeRTOS/FreeRTOS-Kernel.git';Commit='0adc196d4bd52a2d91102b525b0aafc1e14a2386'},
 @{Name='lvgl';Tag='v9.2.2';Url='https://github.com/lvgl/lvgl.git';Commit='7f07a129e8d77f4984fff8e623fd5be18ff42e74'}
)
foreach($package in $packages) {
 $destination=Join-Path $root ('deps\'+$package.Name)
 if(-not(Test-Path -LiteralPath $destination)) {
   & git -c http.sslBackend=openssl clone --depth 1 --branch $package.Tag $package.Url $destination
   if($LASTEXITCODE -ne 0) { throw 'Source download failed' }
 }
 $actual=& git -C $destination rev-parse HEAD
 if($LASTEXITCODE -ne 0 -or $actual.Trim() -ne $package.Commit) { throw ('Unexpected dependency revision: '+$package.Name) }
}
& git -c http.sslBackend=openssl -C (Join-Path $root 'deps\pico-sdk') submodule update --init --depth 1 lib/tinyusb
if($LASTEXITCODE -ne 0) { throw 'TinyUSB source download failed' }
