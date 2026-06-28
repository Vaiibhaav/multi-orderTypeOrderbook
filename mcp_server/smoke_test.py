from orderbook_bridge import OrderbookBridge
import json

def run_test():
    print("Initializing Orderbook Bridge...")
    b = OrderbookBridge()
    b.reset()
    print("Initial size:", b.size())
    
    print("\nSubmitting Buy order...")
    res1 = b.submit_order("TEST_AGENT", 100, "Buy", 999, 100, "GTC")
    print("Submit Buy result:", json.dumps(res1, indent=2))
    print("Current size:", b.size())
    
    print("\nRetrieving Snapshot:")
    snap = b.get_snapshot(depth=5)
    print(json.dumps(snap, indent=2))
    
    print("\nSubmitting Sell order to cross the spread...")
    res2 = b.submit_order("TEST_AGENT", 101, "Sell", 998, 50, "FAK")
    print("Submit Sell result:", json.dumps(res2, indent=2))
    print("Current size after fill:", b.size())
    
    print("\nRetrieving Snapshot:")
    snap2 = b.get_snapshot(depth=5)
    print(json.dumps(snap2, indent=2))

    print("\nRetrieving Trade History:")
    hist = b.get_trade_history(count=5)
    print(json.dumps(hist, indent=2))

    print("\nSpread Analysis:")
    analysis = b.analyze_spread()
    print(json.dumps(analysis, indent=2))
    
    print("\nResetting Orderbook...")
    b.reset()
    print("Size after reset:", b.size())

if __name__ == "__main__":
    run_test()
