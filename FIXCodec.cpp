#include "FIXCodec.h"
#include "FIXTags.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

AddAction FIXCodec::decodeNewOrderSingle(const FIXMessage& msg) {
    OrderId orderId = msg.getIntField(FIX::Tag::ClOrdID);
    
    std::string sideStr = msg.getField(FIX::Tag::Side);
    Side side = (sideStr == FIX::SideVal::Buy) ? Side::Buy : Side::Sell;
    
    Price price = msg.getIntField(FIX::Tag::Price);
    Quantity quantity = msg.getIntField(FIX::Tag::OrderQty);
    
    OrderType orderType = OrderType::GoodTillCancel;
    if (msg.hasField(FIX::Tag::TimeInForce)) {
        std::string tif = msg.getField(FIX::Tag::TimeInForce);
        if (tif == FIX::TIF::IOC) {
            orderType = OrderType::ImmediateOrCancel;
        } else if (tif == FIX::TIF::FAK) {
            orderType = OrderType::FillAndKill;
        }
    }
    
    return AddAction{orderType, orderId, side, price, quantity};
}

CancelAction FIXCodec::decodeOrderCancelRequest(const FIXMessage& msg) {
    OrderId orderId = msg.getIntField(FIX::Tag::ClOrdID);
    return CancelAction{orderId};
}

ModifyAction FIXCodec::decodeOrderCancelReplace(const FIXMessage& msg) {
    OrderId orderId = msg.getIntField(FIX::Tag::ClOrdID);
    
    std::string sideStr = msg.getField(FIX::Tag::Side);
    Side side = (sideStr == FIX::SideVal::Buy) ? Side::Buy : Side::Sell;
    
    Price price = msg.getIntField(FIX::Tag::Price);
    Quantity quantity = msg.getIntField(FIX::Tag::OrderQty);
    
    return ModifyAction{orderId, side, price, quantity};
}

std::string FIXCodec::encodeNewOrderAck(OrderId orderId, Quantity leavesQty) {
    FIXMessage msg;
    msg.setField(FIX::Tag::BeginString, "FIX.4.2");
    msg.setField(FIX::Tag::MsgType, FIX::MsgType::ExecutionReport);
    msg.setField(FIX::Tag::ClOrdID, (int)orderId);
    msg.setField(FIX::Tag::OrdStatus, FIX::OrdStatus::New);
    msg.setField(FIX::Tag::ExecType, FIX::ExecType::New);
    msg.setField(FIX::Tag::LeavesQty, (int)leavesQty);
    msg.setField(FIX::Tag::CumQty, 0);
    return msg.serialize();
}

std::string FIXCodec::encodeCancelAck(OrderId orderId) {
    FIXMessage msg;
    msg.setField(FIX::Tag::BeginString, "FIX.4.2");
    msg.setField(FIX::Tag::MsgType, FIX::MsgType::ExecutionReport);
    msg.setField(FIX::Tag::ClOrdID, (int)orderId);
    msg.setField(FIX::Tag::OrdStatus, FIX::OrdStatus::Cancelled);
    msg.setField(FIX::Tag::ExecType, FIX::ExecType::Cancel);
    msg.setField(FIX::Tag::LeavesQty, 0);
    return msg.serialize();
}

std::string FIXCodec::encodeReject(OrderId orderId, const std::string& reason) {
    FIXMessage msg;
    msg.setField(FIX::Tag::BeginString, "FIX.4.2");
    msg.setField(FIX::Tag::MsgType, FIX::MsgType::Reject);
    msg.setField(FIX::Tag::ClOrdID, (int)orderId);
    msg.setField(FIX::Tag::OrdStatus, FIX::OrdStatus::Rejected);
    msg.setField(FIX::Tag::Text, reason);
    return msg.serialize();
}

std::string FIXCodec::encodeTrades(const Trades& trades) {
    std::ostringstream oss;
    for (const auto& trade : trades) {
        // Encode Bid execution report
        FIXMessage bidMsg;
        bidMsg.setField(FIX::Tag::BeginString, "FIX.4.2");
        bidMsg.setField(FIX::Tag::MsgType, FIX::MsgType::ExecutionReport);
        bidMsg.setField(FIX::Tag::ClOrdID, (int)trade.GetBidTrade().orderId_);
        bidMsg.setField(FIX::Tag::OrdStatus, FIX::OrdStatus::Filled); // Assume filled (or partially filled, but simplistically Filled/F)
        bidMsg.setField(FIX::Tag::ExecType, FIX::ExecType::Fill);
        bidMsg.setField(FIX::Tag::LastPx, trade.GetBidTrade().price_);
        bidMsg.setField(FIX::Tag::LastQty, (int)trade.GetBidTrade().quantity_);
        bidMsg.setField(FIX::Tag::LeavesQty, 0); // simplification
        oss << bidMsg.serialize() << "\n";

        // Encode Ask execution report
        FIXMessage askMsg;
        askMsg.setField(FIX::Tag::BeginString, "FIX.4.2");
        askMsg.setField(FIX::Tag::MsgType, FIX::MsgType::ExecutionReport);
        askMsg.setField(FIX::Tag::ClOrdID, (int)trade.GetAskTrade().orderId_);
        askMsg.setField(FIX::Tag::OrdStatus, FIX::OrdStatus::Filled);
        askMsg.setField(FIX::Tag::ExecType, FIX::ExecType::Fill);
        askMsg.setField(FIX::Tag::LastPx, trade.GetAskTrade().price_);
        askMsg.setField(FIX::Tag::LastQty, (int)trade.GetAskTrade().quantity_);
        askMsg.setField(FIX::Tag::LeavesQty, 0);
        oss << askMsg.serialize() << "\n";
    }
    return oss.str();
}

std::string FIXCodec::encodeSnapshot(const OrderbookLevelInfo& info, int depth) {
    std::ostringstream oss;
    oss << "{\"bids\":[";
    const auto& bids = info.getBids();
    int count = 0;
    for (auto it = bids.begin(); it != bids.end() && count < depth; ++it, ++count) {
        if (count > 0) oss << ",";
        oss << "{\"price\":" << it->price_ << ",\"qty\":" << it->quantity_ << "}";
    }
    oss << "],\"asks\":[";
    const auto& asks = info.getAsks();
    count = 0;
    for (auto it = asks.begin(); it != asks.end() && count < depth; ++it, ++count) {
        if (count > 0) oss << ",";
        oss << "{\"price\":" << it->price_ << ",\"qty\":" << it->quantity_ << "}";
    }
    oss << "]}";
    return oss.str();
}

std::string FIXCodec::encodeTradeHistory(const std::vector<TradeRecord>& records, int count) {
    std::ostringstream oss;
    oss << "{\"trades\":[";
    int added = 0;
    for (const auto& r : records) {
        if (added >= count) break;
        if (added > 0) oss << ",";
        oss << "{\"bid_id\":" << r.bidOrderId
            << ",\"ask_id\":" << r.askOrderId
            << ",\"price\":" << r.price
            << ",\"qty\":" << r.quantity
            << ",\"ts\":" << r.timestampMs << "}";
        added++;
    }
    oss << "]}";
    return oss.str();
}

std::string FIXCodec::encodeSpreadAnalysis(const OrderbookLevelInfo& info) {
    std::ostringstream oss;
    const auto& bids = info.getBids();
    const auto& asks = info.getAsks();
    
    bool has_market = !bids.empty() && !asks.empty();
    Price best_bid = bids.empty() ? 0 : bids.front().price_;
    Price best_ask = asks.empty() ? 0 : asks.front().price_;
    Price spread = has_market ? (best_ask - best_bid) : 0;
    double mid = has_market ? (best_bid + best_ask) / 2.0 : 0.0;
    
    Quantity total_bid_qty = 0;
    for (const auto& b : bids) total_bid_qty += b.quantity_;
    
    Quantity total_ask_qty = 0;
    for (const auto& a : asks) total_ask_qty += a.quantity_;
    
    double imbalance = 0.0;
    if (total_bid_qty + total_ask_qty > 0) {
        imbalance = (double)(total_bid_qty - total_ask_qty) / (total_bid_qty + total_ask_qty);
    }
    
    oss << "{\"has_market\":" << (has_market ? "true" : "false")
        << ",\"best_bid\":" << best_bid
        << ",\"best_ask\":" << best_ask
        << ",\"spread\":" << spread
        << ",\"mid\":" << std::fixed << std::setprecision(4) << mid
        << ",\"total_bid_qty\":" << total_bid_qty
        << ",\"total_ask_qty\":" << total_ask_qty
        << ",\"imbalance\":" << imbalance << "}";
        
    return oss.str();
}
