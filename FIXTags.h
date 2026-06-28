#pragma once

namespace FIX {
    namespace Tag {
        constexpr int BeginString = 8;
        constexpr int MsgType = 35;
        constexpr int ClOrdID = 11;
        constexpr int Side = 54;
        constexpr int Price = 44;
        constexpr int OrderQty = 38;
        constexpr int TimeInForce = 59;
        constexpr int OrdType = 40;
        constexpr int OrdStatus = 39;
        constexpr int ExecType = 150;
        constexpr int LastPx = 31;
        constexpr int LastQty = 32;
        constexpr int LeavesQty = 151;
        constexpr int CumQty = 14;
        constexpr int Text = 58;
    }
    namespace MsgType {
        constexpr const char* NewOrderSingle            = "D";
        constexpr const char* OrderCancelRequest        = "F";
        constexpr const char* OrderCancelReplaceRequest = "G";
        constexpr const char* ExecutionReport           = "8";
        constexpr const char* Reject                    = "3";
    }
    namespace SideVal {
        constexpr const char* Buy = "1";
        constexpr const char* Sell = "2";
    }
    namespace TIF {
        constexpr const char* GTC = "1";
        constexpr const char* IOC = "3";
        constexpr const char* FAK = "4";
    }
    namespace OrdStatus {
        constexpr const char* New = "0";
        constexpr const char* Filled = "2";
        constexpr const char* Cancelled = "4";
        constexpr const char* Rejected = "8";
    }
    namespace ExecType {
        constexpr const char* New = "0";
        constexpr const char* Fill = "F";
        constexpr const char* Cancel = "4";
    }
}
