# Builds and runs the JUCE-free DSP parity harness.
# Requires MinGW g++ (or any C++20 compiler) on PATH. JUCE is not needed.
#
#   pwsh Apollo/tools/dsp_parity/build.ps1
#
# Renders land in Apollo/docs/dsp_parity/renders, plots in .../plots.

$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$apollo = Join-Path $root "Apollo"
$inc = @(
    (Join-Path $apollo "Source\DSP"),
    (Join-Path $apollo "Source\DSP\Dattorro"),
    (Join-Path $apollo "Source\DSP\Util")
)
$src = @(
    (Join-Path $apollo "tools\dsp_parity\parity_harness.cpp"),
    (Join-Path $apollo "Source\DSP\Dattorro\Dattorro.cpp"),
    (Join-Path $apollo "Source\DSP\Dattorro\dsp\delays\InterpDelay.cpp"),
    (Join-Path $apollo "Source\DSP\Dattorro\dsp\filters\OnePoleFilters.cpp")
)
$outDir = Join-Path $apollo "docs\dsp_parity\renders"
$plotDir = Join-Path $apollo "docs\dsp_parity\plots"
$exe = Join-Path $apollo "tools\dsp_parity\parity_harness.exe"

$args = @("-std=c++20", "-O2") + ($inc | ForEach-Object { "-I$_" }) + $src + @("-o", $exe)
& g++ @args

New-Item -ItemType Directory -Force -Path $outDir, $plotDir | Out-Null
& $exe $outDir

python (Join-Path $apollo "tools\dsp_parity\analyze.py") $outDir $plotDir
