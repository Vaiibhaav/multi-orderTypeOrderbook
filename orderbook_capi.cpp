#include "orderbook_capi.h"
#include "Orderbook.h"
#include "FIXMessage.h"
#include "FIXCodec.h"
#include "FIXTags.h"
#include <string>
#include <exception>
#include <iostream>

struct EngineHandle {
    Orderbook* orderbook;
};

// Thread-local return buffer to make string returns thread-safe and simple
static thread_local std::string return_buffer;

extern "C" {
    OB_EXPORT void* ob_create() {
        return new EngineHandle{ new Orderbook() };
    }

    OB_EXPORT void ob_destroy(void* handle) {
        if (!handle) return;
        auto* eh = static_cast<EngineHandle*>(handle);
        delete eh->orderbook;
        delete eh;
    }

    OB_EXPORT void ob_reset(void* handle) {
        if (!handle) return;
        auto* eh = static_cast<EngineHandle*>(handle);
        delete eh->orderbook;
        eh->orderbook = new Orderbook();
    }

    OB_EXPORT const char* ob_submit_fix(void* handle, const char* fix_message) {
        if (!handle || !fix_message) return "";
        auto* eh = static_cast<EngineHandle*>(handle);
        
        try {
            FIXMessage msg = FIXMessage::parse(fix_message);
            std::string msgType = msg.getMsgType();
            
            if (msgType == FIX::MsgType::NewOrderSingle) {
                AddAction add = FIXCodec::decodeNewOrderSingle(msg);
                
                // Construct OrderPointer
                auto order = std::make_shared<Order>(add.orderType, add.orderId, add.side, add.price, add.quantity);
                Trades trades = eh->orderbook->AddOrder(order);
                
                if (trades.empty()) {
                    if (order->IsFilled()) {
                        return_buffer = FIXCodec::encodeNewOrderAck(add.orderId, 0);
                    } else if (add.orderType == OrderType::FillAndKill && !eh->orderbook->Size()) {
                        // Entirely cancelled FAK
                        return_buffer = FIXCodec::encodeReject(add.orderId, "FAK order could not match and was cancelled");
                    } else {
                        return_buffer = FIXCodec::encodeNewOrderAck(add.orderId, order->GetRemainingQuantity());
                    }
                } else {
                    return_buffer = FIXCodec::encodeNewOrderAck(add.orderId, order->GetRemainingQuantity());
                    return_buffer += "\n" + FIXCodec::encodeTrades(trades);
                }
            } 
            else if (msgType == FIX::MsgType::OrderCancelRequest) {
                CancelAction cancel = FIXCodec::decodeOrderCancelRequest(msg);
                eh->orderbook->CancelOrder(cancel.orderId);
                return_buffer = FIXCodec::encodeCancelAck(cancel.orderId);
            } 
            else if (msgType == FIX::MsgType::OrderCancelReplaceRequest) {
                ModifyAction modify = FIXCodec::decodeOrderCancelReplace(msg);
                OrderModify orderMod(modify.orderId, modify.side, modify.price, modify.quantity);
                Trades trades = eh->orderbook->ModifyOrder(orderMod);
                
                if (trades.empty()) {
                    return_buffer = FIXCodec::encodeNewOrderAck(modify.orderId, modify.quantity);
                } else {
                    return_buffer = FIXCodec::encodeNewOrderAck(modify.orderId, modify.quantity);
                    return_buffer += "\n" + FIXCodec::encodeTrades(trades);
                }
            } 
            else {
                return_buffer = FIXCodec::encodeReject(0, "Unknown FIX MsgType");
            }
        } 
        catch (const std::exception& e) {
            return_buffer = FIXCodec::encodeReject(0, std::string("Error: ") + e.what());
        }
        
        return return_buffer.c_str();
    }

    OB_EXPORT const char* ob_get_snapshot(void* handle, int depth) {
        if (!handle) return "{}";
        auto* eh = static_cast<EngineHandle*>(handle);
        OrderbookLevelInfo info = eh->orderbook->GetOrderInfo();
        return_buffer = FIXCodec::encodeSnapshot(info, depth);
        return return_buffer.c_str();
    }

    OB_EXPORT const char* ob_get_trade_history(void* handle, int count) {
        if (!handle) return "{}";
        auto* eh = static_cast<EngineHandle*>(handle);
        auto history = eh->orderbook->GetTradeHistory(count);
        return_buffer = FIXCodec::encodeTradeHistory(history, count);
        return return_buffer.c_str();
    }

    OB_EXPORT const char* ob_analyze_spread(void* handle) {
        if (!handle) return "{}";
        auto* eh = static_cast<EngineHandle*>(handle);
        OrderbookLevelInfo info = eh->orderbook->GetOrderInfo();
        return_buffer = FIXCodec::encodeSpreadAnalysis(info);
        return return_buffer.c_str();
    }

    OB_EXPORT int ob_size(void* handle) {
        if (!handle) return 0;
        auto* eh = static_cast<EngineHandle*>(handle);
        return static_cast<int>(eh->orderbook->Size());
    }
}
