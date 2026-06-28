import asyncio
import time
from base_agent import BaseAgent

class SpooferDetectorAgent(BaseAgent):
    def __init__(self, name, order_id_start, large_order_threshold=1000, time_window_seconds=1.0):
        # Detector doesn't place orders, but still inherits from BaseAgent for bridge and logs
        super().__init__(name, order_id_start)
        self.large_order_threshold = large_order_threshold
        self.time_window_seconds = time_window_seconds
        
        self.snapshot_history = []  # List of (timestamp, snapshot)
        self.alerts = []

    def _find_level_qty(self, levels, price):
        for lvl in levels:
            if lvl["price"] == price:
                return lvl["qty"]
        return 0

    async def run(self, duration_seconds):
        self.log("Starting Spoofer Detector Agent (Surveillance Mode)")
        elapsed = 0.0
        interval = 0.15  # Check every 150ms
        
        while elapsed < duration_seconds:
            try:
                # OBSERVE
                curr_time = time.time()
                snapshot = self.bridge.get_snapshot(depth=10)
                spread_info = self.bridge.analyze_spread()
                mid_price = spread_info.get("mid", 1000.0)
                
                self.update_pnl(mid_price)
                
                self.snapshot_history.append((curr_time, snapshot))
                if len(self.snapshot_history) > 50:
                    self.snapshot_history.pop(0)
                    
                # Look back in history to find a snapshot from time_window_seconds ago
                target_time = curr_time - self.time_window_seconds
                historical_snap = None
                
                for t, snap in reversed(self.snapshot_history):
                    if t <= target_time:
                        historical_snap = snap
                        break
                        
                if historical_snap:
                    # Fetch recent trades to see if cancellations are actually matches
                    trade_hist = self.bridge.get_trade_history(count=10)
                    recent_trades = trade_hist.get("trades", [])
                    
                    # Helper to get trade volume at a price
                    def get_trade_vol_at_price(price):
                        vol = 0
                        for t in recent_trades:
                            if t["price"] == price:
                                vol += t["qty"]
                        return vol

                    # THINK
                    # Check Bid side changes
                    for old_bid in historical_snap.get("bids", []):
                        old_price = old_bid["price"]
                        old_qty = old_bid["qty"]
                        
                        if old_qty >= self.large_order_threshold:
                            # What is the qty at this price now?
                            curr_qty = self._find_level_qty(snapshot.get("bids", []), old_price)
                            drop = old_qty - curr_qty
                            
                            if drop >= self.large_order_threshold:
                                # Did trades happen at this price to explain the drop?
                                trade_vol = get_trade_vol_at_price(old_price)
                                cancel_qty = drop - trade_vol
                                
                                if cancel_qty >= self.large_order_threshold:
                                    alert_msg = f"SPOOF ALERT on Bids: Price {old_price} had {old_qty} qty, dropped to {curr_qty}. Only {trade_vol} traded. ~{cancel_qty} cancelled!"
                                    self.log(alert_msg)
                                    self.alerts.append({
                                        "time": curr_time,
                                        "side": "Buy",
                                        "price": old_price,
                                        "cancelled_qty": cancel_qty,
                                        "msg": alert_msg
                                    })
                                    
                    # Check Ask side changes
                    for old_ask in historical_snap.get("asks", []):
                        old_price = old_ask["price"]
                        old_qty = old_ask["qty"]
                        
                        if old_qty >= self.large_order_threshold:
                            curr_qty = self._find_level_qty(snapshot.get("asks", []), old_price)
                            drop = old_qty - curr_qty
                            
                            if drop >= self.large_order_threshold:
                                trade_vol = get_trade_vol_at_price(old_price)
                                cancel_qty = drop - trade_vol
                                
                                if cancel_qty >= self.large_order_threshold:
                                    alert_msg = f"SPOOF ALERT on Asks: Price {old_price} had {old_qty} qty, dropped to {curr_qty}. Only {trade_vol} traded. ~{cancel_qty} cancelled!"
                                    self.log(alert_msg)
                                    self.alerts.append({
                                        "time": curr_time,
                                        "side": "Sell",
                                        "price": old_price,
                                        "cancelled_qty": cancel_qty,
                                        "msg": alert_msg
                                    })
                                    
            except Exception as e:
                self.log(f"Error in Detector loop: {e}")
                
            await asyncio.sleep(interval)
            elapsed += interval
            
        self.log(f"Stopping Detector. Total alerts flagged: {len(self.alerts)}")
