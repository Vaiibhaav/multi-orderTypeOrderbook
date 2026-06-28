@echo off
echo Setting up MSVC x64 build environment...
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

echo Compiling 64-bit Orderbook FIX Engine DLL with MSVC...
cl /EHsc /O2 /LD /std:c++17 /Fe:orderbook_engine.dll ^
    orderbook_capi.cpp FIXCodec.cpp Orderbook.cpp Order.cpp ^
    OrderbookLevelInfo.cpp Trade.cpp OrderModify.cpp

if %errorlevel% neq 0 (
    echo MSVC BUILD FAILED!
    exit /b 1
)
echo MSVC BUILD SUCCEEDED: orderbook_engine.dll
