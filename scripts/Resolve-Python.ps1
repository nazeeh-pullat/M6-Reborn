function Resolve-ProjectPython([string]$ProjectRoot) {
    $localPython=Join-Path $ProjectRoot '.venv\Scripts\python.exe'
    if(Test-Path -LiteralPath $localPython) { return $localPython }
    if($env:PYTHON_EXECUTABLE -and (Test-Path -LiteralPath $env:PYTHON_EXECUTABLE)) { return $env:PYTHON_EXECUTABLE }
    $command=Get-Command python.exe -ErrorAction SilentlyContinue
    if($command) { return $command.Source }
    throw 'Python not found. Create .venv or set PYTHON_EXECUTABLE to your Python executable.'
}
