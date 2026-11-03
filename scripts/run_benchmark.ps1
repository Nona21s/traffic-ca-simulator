param(
    [string]$BuildDirectory = ".\build\Release",
    [string]$OutputPath = ".\results\benchmark.csv",
    [int]$Steps = 10000,
    [int]$Repetitions = 5
)

$ErrorActionPreference = "Stop"
$executable = Join-Path $BuildDirectory "traffic_benchmark.exe"
if (-not (Test-Path $executable)) {
    throw "Benchmark executable not found: $executable. Build the traffic_benchmark target first."
}

$outputDirectory = Split-Path -Parent $OutputPath
if ($outputDirectory -and -not (Test-Path $outputDirectory)) {
    New-Item -ItemType Directory -Path $outputDirectory | Out-Null
}

& $executable --steps $Steps --repetitions $Repetitions --output $OutputPath
if ($LASTEXITCODE -ne 0) {
    throw "Benchmark failed with exit code $LASTEXITCODE."
}
