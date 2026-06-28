@echo off
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
