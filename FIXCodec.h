#pragma once
#include <string>
#include <vector>
#include "FIXMessage.h"
#include "OrderAction.h"
#include "Trade.h"
#include "OrderbookLevelInfo.h"
#include "Orderbook.h"

class FIXCodec {
public:
    static AddAction decodeNewOrderSingle(const FIXMessage& msg);
    static CancelAction decodeOrderCancelRequest(const FIXMessage& msg);
    static ModifyAction decodeOrderCancelReplace(const FIXMessage& msg);

    static std::string encodeNewOrderAck(OrderId orderId, Quantity leavesQty);
    static std::string encodeCancelAck(OrderId orderId);
    static std::string encodeReject(OrderId orderId, const std::string& reason);
    static std::string encodeTrades(const Trades& trades);

    static std::string encodeSnapshot(const OrderbookLevelInfo& info, int depth);
    static std::string encodeTradeHistory(const std::vector<TradeRecord>& records, int count);
    static std::string encodeSpreadAnalysis(const OrderbookLevelInfo& info);
};
