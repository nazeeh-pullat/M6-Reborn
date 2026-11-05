[CmdletBinding()]
param([string]$ProjectRoot=(Split-Path $PSScriptRoot -Parent))
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Path (Join-Path $ProjectRoot 'logs') -Force | Out-Null
$toolRoot=Join-Path $ProjectRoot 'tools'
$downloadRoot=Join-Path $toolRoot 'downloads'
New-Item -ItemType Directory -Path $downloadRoot -Force | Out-Null
$packages=@(
  @{Name='cmake'; File='cmake-3.31.8-windows-x86_64.zip'; Url='https://github.com/Kitware/CMake/releases/download/v3.31.8/cmake-3.31.8-windows-x86_64.zip'},
  @{Name='ninja'; File='ninja-win.zip'; Url='https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip'},
  @{Name='arm'; File='arm-gnu-toolchain-14.2.rel1-mingw-w64-x86_64-arm-none-eabi.zip'; Url='https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-mingw-w64-x86_64-arm-none-eabi.zip'}
)
$results=@()
foreach($package in $packages) {
  $archive=Join-Path $downloadRoot $package.File
  $destination=Join-Path $toolRoot $package.Name
  if(-not(Test-Path -LiteralPath $archive)) { Invoke-WebRequest -Uri $package.Url -OutFile $archive }
  if(-not(Test-Path -LiteralPath $destination)) { Expand-Archive -LiteralPath $archive -DestinationPath $destination }
  $results += [ordered]@{name=$package.Name;url=$package.Url;archive=$archive;sha256=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash;extracted=$destination}
}
$results | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $ProjectRoot 'logs\build-tool-downloads.json')
# Portable tools only. No installer, registry, system PATH, driver or VS Code extension changes.
