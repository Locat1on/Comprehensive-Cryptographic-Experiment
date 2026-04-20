# 加解密综合服务平台 - PowerShell 构建脚本
# 用法: .\build.ps1

$ErrorActionPreference = "Stop"

# 检测编译器路径（优先使用环境变量，否则使用默认路径）
$GXX = if ($env:GXX) { $env:GXX } elseif (Get-Command g++ -ErrorAction SilentlyContinue) { "g++" }
$AR = if ($env:AR) { $env:AR } elseif (Get-Command ar -ErrorAction SilentlyContinue) { "ar" }

# 检测 MSYS2 路径
$MSYS2_ROOT = if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT }

$INC = @("-Icrypto\include", "-Ilib")
if ($MSYS2_ROOT) { $INC += "-I$MSYS2_ROOT\include" }

$FLAGS = @("-std=c++17", "-O2", "-DASIO_STANDALONE")

# 验证编译器是否存在
if (-not (Test-Path $GXX -ErrorAction SilentlyContinue) -and -not (Get-Command $GXX -ErrorAction SilentlyContinue)) {
    Write-Host "错误: 找不到 g++ 编译器" -ForegroundColor Red
    Write-Host "请设置环境变量 GXX 指向 g++.exe 的路径，或将 g++ 添加到 PATH" -ForegroundColor Yellow
    Write-Host "例如: `$env:GXX = 'C:\msys64\mingw64\bin\g++.exe'" -ForegroundColor Yellow
    exit 1
}

Write-Host "[1/3] 编译算法库..." -ForegroundColor Cyan

$sourceFiles = @(
    @{src="crypto\src\AffineCipher.cpp"; obj="crypto\AffineCipher.o"},
    @{src="crypto\src\BigInt128.cpp"; obj="crypto\BigInt128.o"},
    @{src="crypto\src\StreamCipher.cpp"; obj="crypto\StreamCipher.o"},
    @{src="crypto\src\DES.cpp"; obj="crypto\DES.o"},
    @{src="crypto\src\RSA.cpp"; obj="crypto\RSA.o"},
    @{src="crypto\src\DHProtocol.cpp"; obj="crypto\DHProtocol.o"},
    @{src="crypto\src\Hash.cpp"; obj="crypto\Hash.o"},
    @{src="crypto\src\DigitalSignature.cpp"; obj="crypto\DigitalSignature.o"},
    @{src="crypto\src\SecureFileTransfer.cpp"; obj="crypto\SecureFileTransfer.o"}
)

foreach ($file in $sourceFiles) {
    Write-Host "  编译 $($file.src)..." -ForegroundColor Gray
    & $GXX $FLAGS $INC -c $file.src -o $file.obj
    if ($LASTEXITCODE -ne 0) {
        Write-Host "编译失败: $($file.src)" -ForegroundColor Red
        exit 1
    }
}

Write-Host "[2/3] 打包静态库 libcrypto.a..." -ForegroundColor Cyan
$objFiles = $sourceFiles | ForEach-Object { $_.obj }
& $AR rcs lib\libcrypto.a $objFiles
if ($LASTEXITCODE -ne 0) {
    Write-Host "打包静态库失败" -ForegroundColor Red
    exit 1
}

Write-Host "[3/3] 编译服务器并链接..." -ForegroundColor Cyan
& $GXX $FLAGS $INC src\main.cpp -Llib -lcrypto -o server.exe -lws2_32 -lmswsock
if ($LASTEXITCODE -ne 0) {
    Write-Host "编译服务器失败" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "构建成功！运行: .\server.exe" -ForegroundColor Green
