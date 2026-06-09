#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include "ThreadSafeQueue.h"
#include "OrderAction.h"
#include <mutex>

std::mutex printMutex;

int main(){
    ThreadSafeQueue<OrderAction> queue(32);
    std::atomic<int> consumed{0};

    // ─── Consumer thread ───
    // Runs in background, keeps popping until Shutdown() is called
    std::thread consumer([&queue, &consumed](){
        OrderAction action;
        while(queue.Pop(action)){
            // Pop returned true → we got an item
            consumed++;

            // Let's print what we got (just to see it working)
            if(std::holds_alternative<AddAction>(action)){
                const auto& add = std::get<AddAction>(action);
                std::lock_guard<std::mutex> printLock(printMutex);
                std::cout << "[Consumer] Got AddAction: id=" << add.orderId
                          << " side=" << (add.side == Side::Buy ? "Buy" : "Sell")
                          << " price=" << add.price
                          << " qty=" << add.quantity << "\n";
            }
            else if(std::holds_alternative<CancelAction>(action)){
                const auto& cancel = std::get<CancelAction>(action);
                std::lock_guard<std::mutex> printLock(printMutex);
                std::cout << "[Consumer] Got CancelAction: id=" << cancel.orderId << "\n";
            }
        }
        // Pop returned false → shutdown was called and queue is empty
        {
            std::lock_guard<std::mutex> printLock(printMutex);
            std::cout << "[Consumer] Shutting down.\n";
        }
    });

    // ─── Producer threads ───
    // 4 threads, each pushing 2 orders
    const int NUM_PRODUCERS = 4;
    const int ORDERS_PER_PRODUCER = 2;

    std::vector<std::thread> producers;
    for(int t = 0; t < NUM_PRODUCERS; t++){
        producers.emplace_back([&queue, t](){
            for(int i = 0; i < ORDERS_PER_PRODUCER; i++){
                // Each producer uses a unique ID range: t*10000 + i
                OrderId id = t * 4 + i;
                Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;
                Price price = 100 + (i % 10) - 5;  // prices 95-104

                queue.Push(AddAction{
                    OrderType::GoodTillCancel, id, side, price, 10
                });
            }
            std::lock_guard<std::mutex> printLock(printMutex);
            std::cout << "[Producer " << t << "] Done. Pushed " 
                      << ORDERS_PER_PRODUCER << " orders.\n";
        });
    }

    // ─── Shutdown sequence ───
    // Step 1: Wait for ALL producers to finish
    for(auto& p : producers){
        p.join();
    }
    std::cout << "\nAll producers done.\n";

    // Step 2: Signal shutdown (consumer will drain remaining items, then stop)
    queue.Shutdown();

    // Step 3: Wait for consumer to finish
    consumer.join();

    // ─── Verify ───
    int expected = NUM_PRODUCERS * ORDERS_PER_PRODUCER;
    std::cout << "\n=== RESULTS ===\n";
    std::cout << "Expected: " << expected << "\n";
    std::cout << "Consumed: " << consumed.load() << "\n";
    if(consumed.load() == expected){
        std::cout << " PASS  No data lost!\n";
    } else {
        std::cout << " FAIL  Data was lost!\n";
    }

    return 0;
}