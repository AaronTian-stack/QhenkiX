$clangFormatPath = Get-Command clang-format-21 -ErrorAction SilentlyContinue
if (-not $clangFormatPath) {
    $clangFormatPath = Get-Command clang-format -ErrorAction SilentlyContinue
}

if (-not $clangFormatPath) {
    Write-Host "Error: clang-format not found in PATH" -ForegroundColor Red
    exit 1
}

& $clangFormatPath --version

$externalPattern = '[\\/]external[\\/]'
$buildPattern = '[\\/]build[\\/]'
$includePattern = '[\\/]include[\\/]'
$libPattern = '[\\/]lib[\\/]'
$redistPattern = '[\\/]redist[\\/]'
$x64Pattern = '[\\/]x64[\\/]'

$files = @(
    @(Get-ChildItem -Path "QhenkiX" -Include "*.cpp", "*.h", "*.hlsl" -Recurse | Where-Object {
        $_.FullName -notmatch $externalPattern -and $_.FullName -notmatch $buildPattern
    }),
    @(Get-ChildItem -Path "Examples" -Include "*.cpp", "*.h", "*.hlsl" -Recurse | Where-Object {
        $_.FullName -notmatch $includePattern -and $_.FullName -notmatch $buildPattern -and $_.FullName -notmatch $externalPattern
    }),
    @(Get-ChildItem -Path "SXC" -Include "*.cpp", "*.h", "*.hlsl" -Recurse | Where-Object {
        $_.FullName -notmatch $includePattern -and $_.FullName -notmatch $buildPattern -and $_.FullName -notmatch $externalPattern -and $_.FullName -notmatch $libPattern -and $_.FullName -notmatch $redistPattern -and $_.FullName -notmatch $x64Pattern
    })
) | Where-Object { $_ -ne $null }

foreach ($file in $files) {
    & clang-format -i $file.FullName
}

