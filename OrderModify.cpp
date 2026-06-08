#include "OrderModify.h"
#include "Order.h"

OrderModify::OrderModify(OrderId orderId,Side side,Price price,Quantity quantity):
    orderId_{orderId},
    side_{side},
    price_{price},
    quantity_{quantity}
{}

OrderId OrderModify::GetOrderId()const {return orderId_;}
Side OrderModify::GetSide()const {return side_;}
Price OrderModify::GetPrice()const {return price_;}
Quantity OrderModify::GetQuantity()const {return quantity_;}
OrderPointer OrderModify::ToOrderPointer(OrderType type)const {
    return std::make_shared<Order>(type,GetOrderId(),GetSide(),GetPrice(),GetQuantity());
}