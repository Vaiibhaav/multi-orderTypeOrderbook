@echo offecho Checking GCC target architecture...
set "GCC_TARGET="
for /f "delims=" %%t in ('g++ -dumpmachine 2^>nul') do set "GCC_TARGET=%%t"
echo GCC target: %GCC_TARGET%
if /I "%GCC_TARGET%"=="i686-w64-mingw32" (
    echo WARNING: Detected 32-bit GCC toolchain. This will build a 32-bit DLL.
    echo If you need a 64-bit DLL for 64-bit Python, run build_dll_msvc.bat instead.
)
echo Building Orderbook FIX Engine DLL...
g++ -std=c++17 -shared -o orderbook_engine.dll ^
    orderbook_capi.cpp FIXCodec.cpp Orderbook.cpp Order.cpp ^
    OrderbookLevelInfo.cpp Trade.cpp OrderModify.cpp ^
    -I. -O2 -Wall -Wl,--kill-at
if %errorlevel% neq 0 (
    echo BUILD FAILED!
    exit /b 1
)
echo BUILD SUCCEEDED: orderbook_engine.dll
