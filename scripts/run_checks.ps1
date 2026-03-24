Param()

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $projectRoot '..\build' | Resolve-Path -Relative
$sourceDirs = @('Source', 'Source\Dsp')

Write-Host 'Running full code quality checks for ResonanceEQ...'

# clang-format
if (-not (Get-Command clang-format -ErrorAction SilentlyContinue)) {
    Write-Error 'clang-format not found in PATH.'
    exit 1
}

Write-Host '1/3: Running clang-format dry-run'
$formatFail = $false
foreach ($dir in $sourceDirs) {
    Get-ChildItem -Path $dir -Recurse -Include *.cpp,*.h | ForEach-Object {
        & clang-format --dry-run --Werror --style=file $_.FullName
        if ($LASTEXITCODE -ne 0) {
            $formatFail = $true
            Write-Host "clang-format issue: $($_.FullName)"
        }
    }
}
if ($formatFail) {
    Write-Error 'clang-format check failed.'
    exit 1
}

Write-Host '2/3: Running clang-tidy'
if (-not (Get-Command clang-tidy -ErrorAction SilentlyContinue)) {
    Write-Warning 'clang-tidy not found, skipping clang-tidy.'
} else {
    if (-not (Test-Path "$buildDir\compile_commands.json")) {
        Write-Host 'compile_commands.json not found. Generating with CMake...'
        New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
        Push-Location $buildDir
        cmake .. -G 'Visual Studio 17 2022'
        Pop-Location
    }
    if (-not (Test-Path "$buildDir\compile_commands.json")) {
        Write-Error 'compile_commands.json still missing after CMake configuration.'
        exit 1
    }

    $tidyFail = $false
    Get-ChildItem -Path Source -Recurse -Include *.cpp | ForEach-Object {
        & clang-tidy $_.FullName -p $buildDir --quiet
        if ($LASTEXITCODE -ne 0) {
            $tidyFail = $true
            Write-Host "clang-tidy issue: $($_.FullName)"
        }
    }
    if ($tidyFail) {
        Write-Error 'clang-tidy check failed.'
        exit 1
    }
}

Write-Host '3/3: Running cppcheck'
if (-not (Get-Command cppcheck -ErrorAction SilentlyContinue)) {
    Write-Warning 'cppcheck not found, skipping cppcheck.'
} else {
    & cppcheck --enable=warning,performance,portability --std=c++17 --project="$buildDir\compile_commands.json" --template=gcc Source 2> "$buildDir\cppcheck.log"
    if ($LASTEXITCODE -ne 0) {
        Write-Error "cppcheck found issues. See $buildDir\cppcheck.log"
        exit 1
    }
}

Write-Host 'Code quality checks passed.'
exit 0
