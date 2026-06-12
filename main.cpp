#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include "ThreadSafeQueue.h"
#include "OrderAction.h"
#include <mutex>
#include "MatchingEngine.h"

std::mutex printMutex;

int main(){
    ThreadSafeQueue<OrderAction> queue(32);
    std::atomic<int> consumed{0};

    // ─── Consumer thread ───
    // Runs in background, keeps popping until Shutdown() is called
    std::thread consumer([&queue, &consumed](){
        MatchingEngine engine;
        bool useBatch = true; // Toggle this flag to switch between batch and one-by-one

        if (useBatch) {
            std::vector<OrderAction> batch;
            const std::size_t MAX_BATCH_SIZE = 10;
            
            while(true) {
                batch.clear(); // Clear the batch for the next iteration
                std::size_t popped = queue.PopBatch(batch, MAX_BATCH_SIZE);
                
                if (popped == 0) {
                    // PopBatch returns 0 only when shutdown AND empty
                    break;
                }
                
                consumed += popped;
                // Pass the entire batch to the matching engine
                engine.ProcessBatch(batch);
            }
        } else {
            OrderAction action;
            while(queue.Pop(action)){
                consumed++;
                // Pass the single action to the matching engine
                engine.ProcessAction(action);
            }
        }
        
        std::lock_guard<std::mutex> printLock(printMutex);
        std::cout << "[Consumer] Shutting down. Final Orderbook size: " 
                  << engine.GetOrderbook().Size() << "\n";
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
                Price price = 100;  // prices 95-104

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