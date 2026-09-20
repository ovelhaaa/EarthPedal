# Builds and runs the Stage F timebase harness (JUCE-free).
#
#   pwsh Apollo/tools/dsp_parity/build_timebase.ps1
#
# Outputs CSVs + WAVs to Apollo/docs/dsp_parity/stage_f_timebase/renders,
# plots to .../plots.

$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$apollo = Join-Path $root "Apollo"
$inc = @(
    (Join-Path $apollo "Source\DSP"),
    (Join-Path $apollo "Source\DSP\Dattorro"),
    (Join-Path $apollo "Source\DSP\Util")
)
$src = @(
    (Join-Path $apollo "tools\dsp_parity\timebase_harness.cpp"),
    (Join-Path $apollo "Source\DSP\Dattorro\Dattorro.cpp"),
    (Join-Path $apollo "Source\DSP\Dattorro\dsp\delays\InterpDelay.cpp"),
    (Join-Path $apollo "Source\DSP\Dattorro\dsp\filters\OnePoleFilters.cpp")
)
$outDir = Join-Path $apollo "docs\dsp_parity\stage_f_timebase\renders"
$plotDir = Join-Path $apollo "docs\dsp_parity\stage_f_timebase\plots"
$exe = Join-Path $apollo "tools\dsp_parity\timebase_harness.exe"

$args = @("-std=c++20", "-O2") + ($inc | ForEach-Object { "-I$_" }) + $src + @("-o", $exe)
& g++ @args

New-Item -ItemType Directory -Force -Path $outDir, $plotDir | Out-Null
& $exe $outDir

python (Join-Path $apollo "tools\dsp_parity\stage_f_analyze.py") $outDir $plotDir
