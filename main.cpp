#include <iostream>
#include "Orderbook.h"

int main(){
    Orderbook orderbook;
    const OrderId order1 = 1;
    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel,order1,Side::Buy,100,10));
    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel,order1+1,Side::Sell,120,10));
    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel,order1+2,Side::Buy,100,10));
    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel,order1+4,Side::Buy,90,10));
    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel,order1+3,Side::Sell,120,10));
    OrderbookLevelInfo obli = orderbook.GetOrderInfo();
    std::cout<<"Buy Order:\n";
    std::cout<<"Price"<< "\t" <<"Quantity"<<std::endl; 
    for (const auto& [price, quantity]: obli.getBids()){
        std::cout<<price<<"\t"<<quantity<<std::endl;
    }
    std::cout<<std::endl;
    std::cout<<"Sell Order:\n";
    std::cout<<"Price"<< "\t" <<"Quantity"<<std::endl; 
    for (const auto& [price, quantity]: obli.getAsks()){
        std::cout<<price<<"\t"<<quantity<<std::endl;
    }
    return 0;
}