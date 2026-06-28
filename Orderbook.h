#pragma once
#include <map>
#include <unordered_map>
#include <vector>
#include <list>
#include <memory>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <numeric>
#include <string>
#include <iostream>
#include <deque>
#include <chrono>
#include "Constants.h"
#include "OrderType.h"
#include "Side.h"
#include "Order.h"
#include "Trade.h"
#include "OrderModify.h"
#include "OrderbookLevelInfo.h"

struct TradeRecord {
    OrderId  bidOrderId;
    OrderId  askOrderId;
    Price    price;
    Quantity quantity;
    uint64_t timestampMs;
};

class Orderbook{
private:
    struct OrderEntry{
        OrderPointer order_ { nullptr};
        OrderPointers::iterator location_;
    };
    std::map<Price,OrderPointers,std::greater<Price>> bids_;
    std::map<Price,OrderPointers,std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;
    
    static constexpr std::size_t MAX_TRADE_HISTORY = 1000;
    std::deque<TradeRecord> tradeHistory_;

    bool CanMatch(Side side, Price price) const;
    Trades MatchOrders();
public:
    Trades AddOrder(OrderPointer order);
    void CancelOrder(OrderId orderId);
    Trades ModifyOrder(OrderModify order);
    std::size_t Size() const;
    OrderbookLevelInfo GetOrderInfo() const;
    std::vector<TradeRecord> GetTradeHistory(std::size_t count) const;
};