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
#include "Constants.h"
#include "OrderType.h"
#include "Side.h"
#include "Order.h"
#include "Trade.h"
#include "OrderModify.h"
#include "OrderbookLevelInfo.h"

class Orderbook{
private:
    struct OrderEntry{
        OrderPointer order_ { nullptr};
        OrderPointers::iterator location_;
    };
    std::map<Price,OrderPointers,std::greater<Price>> bids_;
    std::map<Price,OrderPointers,std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;
    bool CanMatch(Side side, Price price) const;
    Trades MatchOrders();
public:
    Trades AddOrder(OrderPointer order);
    void CancelOrder(OrderId orderId);
    Trades ModifyOrder(OrderModify order);
    std::size_t Size() const;
    OrderbookLevelInfo GetOrderInfo() const;
};