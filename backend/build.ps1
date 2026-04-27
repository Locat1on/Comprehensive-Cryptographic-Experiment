$ErrorActionPreference = "Stop"

function Resolve-Tool([string]$envName, [string]$defaultCommand) {
    $fromEnv = [Environment]::GetEnvironmentVariable($envName)
    if ($fromEnv) {
        return $fromEnv
    }

    $cmd = Get-Command $defaultCommand -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    # Fallback: 常见 MinGW 安装路径
    $candidates = @(
        "D:\CodeBlocks\MinGW\bin\$defaultCommand.exe",
        "C:\msys64\mingw64\bin\$defaultCommand.exe",
        "F:\msys2\mingw64\bin\$defaultCommand.exe",
        "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\$defaultCommand.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }

    return $null
}

function Resolve-IncludeRoot([string]$envName, [string[]]$candidates, [string]$header) {
    $roots = @()

    $fromEnv = [Environment]::GetEnvironmentVariable($envName)
    if ($fromEnv) {
        $roots += $fromEnv
    }

    $roots += $candidates

    foreach ($root in $roots) {
        if (-not $root) {
            continue
        }

        if (-not (Test-Path $root -ErrorAction SilentlyContinue)) {
            continue
        }

        $headerPath = Join-Path $root $header
        if (Test-Path $headerPath) {
            return $root
        }
    }

    return $null
}

function Run-Step([string]$tool, [string[]]$arguments, [string]$failureMessage) {
    & $tool @arguments
    if ($LASTEXITCODE -ne 0) {
        Write-Host $failureMessage -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

$gxx = Resolve-Tool "GXX" "g++"
$ar = Resolve-Tool "AR" "ar"
$asioInclude = Resolve-IncludeRoot "ASIO_ROOT" @(
    "$PSScriptRoot\lib\asio",
    "$PSScriptRoot\lib\asio\include",
    "$PSScriptRoot\lib",
    "C:\msys64\mingw64\include",
    "F:\msys2\mingw64\include",
    "D:\msys64\mingw64\include",
    "D:\msys2\mingw64\include"
) "asio.hpp"

if (-not $gxx) {
    Write-Host "Error: g++ was not found." -ForegroundColor Red
    Write-Host 'Set $env:GXX to g++.exe or add g++ to PATH.' -ForegroundColor Yellow
    exit 1
}

if (-not $ar) {
    Write-Host "Error: ar was not found." -ForegroundColor Red
    Write-Host 'Set $env:AR to ar.exe or add ar to PATH.' -ForegroundColor Yellow
    exit 1
}

$toolDirs = @(@(
    [System.IO.Path]::GetDirectoryName($gxx),
    [System.IO.Path]::GetDirectoryName($ar)
) | Where-Object { $_ } | Select-Object -Unique)

for ($i = $toolDirs.Count - 1; $i -ge 0; $i--) {
    $dir = $toolDirs[$i]
    $pathEntries = @($env:PATH -split ';') | Where-Object { $_ -and $_ -ne $dir }
    $env:PATH = ($dir + ';' + ($pathEntries -join ';')).TrimEnd(';')
}

$includeArgs = @(
    "-Icrypto\include",
    "-Ilib"
)

if ($asioInclude) {
    $includeArgs += "-I$asioInclude"
} else {
    Write-Host "Error: asio.hpp was not found." -ForegroundColor Red
    Write-Host 'Install standalone Asio, then set $env:ASIO_ROOT to its include directory if needed.' -ForegroundColor Yellow
    Write-Host 'Example: $env:ASIO_ROOT = "C:\msys64\mingw64\include"' -ForegroundColor Yellow
    exit 1
}

$compilerFlags = @(
    "-std=c++17",
    "-O2",
    "-DASIO_STANDALONE",
    "-DCROW_FILESYSTEM_IS_EXPERIMENTAL"
)

$sourceFiles = @(
    @{ src = "crypto\src\AffineCipher.cpp";         obj = "crypto\AffineCipher.o" },
    @{ src = "crypto\src\BigInt128.cpp";            obj = "crypto\BigInt128.o" },
    @{ src = "crypto\src\StreamCipher.cpp";         obj = "crypto\StreamCipher.o" },
    @{ src = "crypto\src\DES.cpp";                  obj = "crypto\DES.o" },
    @{ src = "crypto\src\RSA.cpp";                  obj = "crypto\RSA.o" },
    @{ src = "crypto\src\DHProtocol.cpp";           obj = "crypto\DHProtocol.o" },
    @{ src = "crypto\src\Hash.cpp";                 obj = "crypto\Hash.o" },
    @{ src = "crypto\src\DigitalSignature.cpp";     obj = "crypto\DigitalSignature.o" },
    @{ src = "crypto\src\SecureFileTransfer.cpp";   obj = "crypto\SecureFileTransfer.o" }
)

Write-Host "[1/3] Compiling crypto sources..." -ForegroundColor Cyan
foreach ($file in $sourceFiles) {
    Write-Host "  $($file.src)" -ForegroundColor DarkGray
    Run-Step $gxx ($compilerFlags + $includeArgs + @("-c", $file.src, "-o", $file.obj)) "Failed to compile $($file.src)."
}

Write-Host "[2/3] Creating static library..." -ForegroundColor Cyan
$objectFiles = $sourceFiles | ForEach-Object { $_.obj }
Run-Step $ar (@("rcs", "lib\libcrypto.a") + $objectFiles) "Failed to create lib\libcrypto.a."

Write-Host "[3/3] Linking server.exe..." -ForegroundColor Cyan
Run-Step $gxx ($compilerFlags + $includeArgs + @(
    "src\main.cpp",
    "-Llib",
    "-lcrypto",
    "-o",
    "server.exe",
    "-lws2_32",
    "-lmswsock"
)) "Failed to link server.exe."

Write-Host ""
Write-Host "Build succeeded. Run .\server.exe" -ForegroundColor Green
