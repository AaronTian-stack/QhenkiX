$clangFormatPath = Get-Command clang-format -ErrorAction SilentlyContinue
if (-not $clangFormatPath) {
    Write-Host "Error: clang-format not found in PATH" -ForegroundColor Red
    exit 1
}

$files = @(
    @(Get-ChildItem -Path "QhenkiX" -Include "*.cpp", "*.h" -Recurse | Where-Object { 
        $_.FullName -notlike "*\external\*" -and $_.FullName -notlike "*\build\*" 
    }),
    @(Get-ChildItem -Path "Examples" -Include "*.cpp", "*.h" -Recurse | Where-Object { 
        $_.FullName -notlike "*\include\*" -and $_.FullName -notlike "*\build\*" 
    })
) | Where-Object { $_ -ne $null }

foreach ($file in $files) {
    & clang-format -i $file.FullName
}

