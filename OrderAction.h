// OrderAction.h
#pragma once
#include "Constants.h"
#include "Side.h"
#include "OrderType.h"
#include <variant>

struct AddAction {
    OrderType orderType;
    OrderId orderId;
    Side side;
    Price price;
    Quantity quantity;
};

struct CancelAction {
    OrderId orderId;
};

struct ModifyAction {
    OrderId orderId;
    Side side;
    Price price;
    Quantity quantity;
};

using OrderAction = std::variant<AddAction, CancelAction, ModifyAction>;
