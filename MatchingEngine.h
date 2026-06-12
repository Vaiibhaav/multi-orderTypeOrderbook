#pragma once
#include "Orderbook.h"
#include "OrderAction.h"
#include <vector>

class MatchingEngine{
public:
    void ProcessAction(const OrderAction& action);
    void ProcessBatch(const std::vector<OrderAction>& actions);
    const Orderbook& GetOrderbook() const{return orderbook_;}
private:
    Orderbook orderbook_;
    void PrintTrades(const Trades& trades) const;
};