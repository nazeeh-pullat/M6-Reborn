[CmdletBinding()]
param([ValidateSet('All','Smoke','DisplayTest','OS','Headless')][string]$Target='All',
      [ValidateRange(0,2)][int]$Profile=0,[ValidateRange(0,3)][int]$Rotation=0)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$cmake=(Get-ChildItem (Join-Path $projectRoot 'tools\cmake') -Filter cmake.exe -Recurse | Select-Object -First 1).FullName
$gcc=(Get-ChildItem (Join-Path $projectRoot 'tools\arm') -Filter arm-none-eabi-gcc.exe -Recurse | Select-Object -First 1).FullName
$armBin=Split-Path $gcc -Parent
$toolchain=Split-Path $armBin -Parent
$ninja=Join-Path $projectRoot 'tools\ninja\ninja.exe'
. (Join-Path $PSScriptRoot 'Resolve-Python.ps1')
$python=Resolve-ProjectPython $projectRoot
New-Item -ItemType Directory -Path (Join-Path $projectRoot 'logs'),(Join-Path $projectRoot 'dist') -Force | Out-Null
foreach($path in @($cmake,$gcc,$ninja,$python)) { if(-not $path -or -not(Test-Path -LiteralPath $path)) { throw "Missing approved build tool: $path" } }
$originalPath=$env:PATH
$env:PATH=$armBin+';'+(Split-Path $cmake -Parent)+';'+(Split-Path $ninja -Parent)+';'+$originalPath
$build=Join-Path $projectRoot 'build'
$log=Join-Path $projectRoot 'logs\build.log'
$epoch=[DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
try {
    $ErrorActionPreference='Continue'
    & $cmake -S $projectRoot -B $build -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" "-DPICO_TOOLCHAIN_PATH=$toolchain" "-DPython3_EXECUTABLE=$python" '-DCMAKE_BUILD_TYPE=MinSizeRel' "-DPICOOS_PROFILE=$Profile" "-DPICOOS_ROTATION=$Rotation" "-DPICOOS_BUILD_EPOCH=$epoch" 2>&1 | Tee-Object -FilePath $log
    $ErrorActionPreference='Stop'
    if($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
    $targets=switch($Target) { 'Smoke' {@('picoos_smoke')} 'DisplayTest' {@('picoos_display_test')} 'OS' {@('M6_Reborn')} 'Headless' {@('picoos_rtos_test')} default {@('picoos_smoke','picoos_display_test','M6_Reborn','picoos_rtos_test')} }
    $ErrorActionPreference='Continue'
    & $cmake --build $build --parallel 8 --target $targets 2>&1 | Tee-Object -FilePath $log -Append
    $ErrorActionPreference='Stop'
    if($LASTEXITCODE -ne 0) { throw 'Compilation failed' }
    foreach($name in $targets) {
        foreach($ext in @('uf2','elf','bin','manifest.json')) {
            $file=Join-Path $build ($name+'.'+$ext)
            if(Test-Path -LiteralPath $file) { Copy-Item -LiteralPath $file -Destination (Join-Path $projectRoot 'dist') -Force }
        }
        & (Join-Path $armBin 'arm-none-eabi-size.exe') --format=berkeley (Join-Path $build ($name+'.elf')) | Tee-Object -FilePath (Join-Path $projectRoot ('logs\'+$name+'-memory.txt'))
        & (Join-Path $armBin 'arm-none-eabi-readelf.exe') -h -A (Join-Path $build ($name+'.elf')) | Set-Content (Join-Path $projectRoot ('logs\'+$name+'-target.txt'))
    }
    @(& $gcc --version; & $cmake --version; & $ninja --version; & $python --version) | Set-Content (Join-Path $projectRoot 'logs\tool-versions.txt')
} finally { $env:PATH=$originalPath }
