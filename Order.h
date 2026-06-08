#pragma once
#include "Constants.h"
#include "OrderType.h"
#include "Side.h"
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

class Order{
public:
    Order(OrderType orderType, OrderId orderId, Side side,Price price,Quantity quantity);
    OrderId GetOrderId()const;
    Side GetSide()const ;
    Price GetPrice()const;
    Quantity GetRemainingQuantity() const ;
    Quantity GetInitialQuantity() const ;
    Quantity GetFilledQuantity() const;
    OrderType GetOrderType() const;
    bool IsFilled() const ;
    void Fill(Quantity quantity);
private:
    OrderType orderType_;
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
};