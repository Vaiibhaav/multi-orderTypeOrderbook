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
// it is a single variable(a fixed - size box) that can hold exactly one value at any given moment, but that value can be chosen from any of the specified types(AddAction, CancelAction, or ModifyAction).
// std::variant<AddAction, CancelAction, ModifyAction>: A single slot. It holds only one item, but that single item can dynamically be an AddAction, a CancelAction, or a ModifyAction depending on what happened last.
// Memory footprint: It only takes up enough memory to store one action (plus a tiny hidden integer tag to remember which type it currently holds).

// Exclusivity: If the variant currently holds an AddAction, it is not holding a CancelAction or ModifyAction. If you assign a CancelAction to it later, the previous AddAction data is replaced or overwritten.

// Type Safety: You cannot accidentally read it as the wrong type. If you try to pull out a ModifyAction when it actually holds a CancelAction, C++ will throw an exception (via std::get) or safely return a null pointer (via std::get_if).
