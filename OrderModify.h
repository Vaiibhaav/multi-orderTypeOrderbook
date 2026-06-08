#include "Constants.h"
#include "Side.h"
#include "OrderType.h"
#include <memory>


// not only we have an order
// we need to create a abstracted representaion of order that need to be modified 
// for cancel we would need order id
// for add we would need and order 
// for replace we need a represntaiton of order that is lightweight can be converted  to new order, we need to cancel(needs order id) and replace(price, quantity,//we can also add side too)
class Order;
class OrderModify{
public:
    OrderModify(OrderId orderId,Side side,Price price,Quantity quantity);
    OrderId GetOrderId()const;
    Side GetSide()const ;
    Price GetPrice()const ;
    Quantity GetQuantity()const;
    OrderPointer ToOrderPointer(OrderType type)const;

private:
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity quantity_; 
};