#include "MatchingEngine.h"
#include <iostream>

template <class... Ts>
struct overload : Ts... { using Ts::operator()...; };
template<class... Ts>
overload(Ts...) -> overload<Ts...>;

void MatchingEngine::ProcessAction(const OrderAction& action) {
    // We use std::visit to handle the different types inside the variant
    std::visit(overload{
        // 1. Handle AddAction
        [this](const AddAction& add) {
            auto order = std::make_shared<Order>(
                add.orderType, add.orderId, add.side, add.price, add.quantity
            );
            Trades trades = orderbook_.AddOrder(order);
            PrintTrades(trades);
        },
        
        // 2. Handle CancelAction
        [this](const CancelAction& cancel) {
            orderbook_.CancelOrder(cancel.orderId);
            // Cancellations don't generate trades, so nothing to print
        },
        
        // 3. Handle ModifyAction
        [this](const ModifyAction& modify) {
            OrderModify orderMod(modify.orderId, modify.side, modify.price, modify.quantity);
            Trades trades = orderbook_.ModifyOrder(orderMod);
            PrintTrades(trades);
        }
    }, action);
}
void MatchingEngine::ProcessBatch(const std::vector<OrderAction>& actions) {
    // Simply loop over the batch and process each action sequentially
    for (const auto& action : actions) {
        ProcessAction(action);
    }
}
void MatchingEngine::PrintTrades(const Trades& trades) const {
    for (const auto& trade : trades) {
        std::cout << "[TRADE] Matched! Bid OrderId: " << trade.GetBidTrade().orderId_ 
                  << " | Ask OrderId: " << trade.GetAskTrade().orderId_ 
                  << " | Price: " << trade.GetBidTrade().price_ 
                  << " | Quantity: " << trade.GetBidTrade().quantity_ << "\n";
    }
}
