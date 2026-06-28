import asyncio
import os
import sys

# Ensure correct path to import bridge
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'mcp_server'))
from orderbook_bridge import OrderbookBridge

from market_maker_agent import MarketMakerAgent
from momentum_agent import MomentumAgent
from spoofer_agent import SpooferAgent
from spoofer_detector_agent import SpooferDetectorAgent

async def main():
    print("=" * 60)
    print("        CONCURRENT ORDERBOOK AGENTIC SIMULATION RUNNER")
    print("=" * 60)
    
    # 1. Initialize and reset bridge
    bridge = OrderbookBridge()
    print("Resetting Orderbook Engine...")
    bridge.reset()
    
    # 2. Instantiate agents
    # MM Order IDs: 1000+, Momentum: 2000+, Spoofer: 3000+, Detector: 4000+
    mm = MarketMakerAgent("MarketMaker", 1000, spread_ticks=4, order_size=200)
    momentum = MomentumAgent("MomentumTrader", 2000, imbalance_threshold=0.3, trade_size=100, cooldown=0.8)
    spoofer = SpooferAgent("Spoofer", 3000, spoof_size=2000, trade_size=150, interval=2.5)
    detector = SpooferDetectorAgent("SurveillanceDetector", 4000, large_order_threshold=1500, time_window_seconds=0.8)
    
    # 3. Start simulation concurrent tasks
    duration = 15.0  # Run simulation for 15 seconds
    print(f"Starting simulation for {duration} seconds...")
    print("-" * 60)
    
    tasks = [
        asyncio.create_task(mm.run(duration)),
        asyncio.create_task(momentum.run(duration)),
        asyncio.create_task(spoofer.run(duration)),
        asyncio.create_task(detector.run(duration))
    ]
    
    # Run all agents concurrently
    await asyncio.gather(*tasks)
    
    print("-" * 60)
    print("SIMULATION COMPLETED. ANALYZING RESULTS...")
    print("-" * 60)
    
    # 4. Gather final agent stats
    spread_info = bridge.analyze_spread()
    mid_price = spread_info.get("mid", 1000.0)
    
    agents = [mm, momentum, spoofer]
    
    print("\n--- AGENT PERFORMANCE SUMMARY ---")
    print(f"{'Agent Name':<20} | {'PnL (Ticks)':<12} | {'Position':<10} | {'Cash (Ticks)':<12} | {'Fills':<6}")
    print("-" * 70)
    for agent in agents:
        agent.update_pnl(mid_price)
        print(f"{agent.name:<20} | {agent.pnl:<12.2f} | {agent.position:<10} | {agent.cash:<12.2f} | {len(agent.fills):<6}")
        
    print("\n--- SURVEILLANCE REPORT ---")
    print(f"Total Spoofing Alerts Flagged: {len(detector.alerts)}")
    for idx, alert in enumerate(detector.alerts):
        print(f"  {idx + 1}. [t={alert['time']:.2f}] {alert['msg']}")
        
    print("\n--- FINAL ORDERBOOK SNAPSHOT ---")
    snap = bridge.get_snapshot(depth=5)
    print("BIDS:")
    for b in snap.get("bids", []):
        print(f"  Price: {b['price']:<6} | Qty: {b['qty']}")
    if not snap.get("bids"):
        print("  [Empty]")
        
    print("ASKS:")
    for a in snap.get("asks", []):
        print(f"  Price: {a['price']:<6} | Qty: {a['qty']}")
    if not snap.get("asks"):
        print("  [Empty]")
        
    print("=" * 60)

if __name__ == "__main__":
    asyncio.run(main())
