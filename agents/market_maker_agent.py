import asyncio
from base_agent import BaseAgent

class MarketMakerAgent(BaseAgent):
    def __init__(self, name, order_id_start, spread_ticks=4, order_size=200):
        super().__init__(name, order_id_start)
        self.spread_ticks = spread_ticks
        self.order_size = order_size
        self.bid_order_id = None
        self.ask_order_id = None

    async def run(self, duration_seconds):
        self.log("Starting Market Maker Agent")
        elapsed = 0.0
        interval = 0.5
        
        while elapsed < duration_seconds:
            try:
                # OBSERVE
                spread_info = self.bridge.analyze_spread()
                has_market = spread_info.get("has_market", False)
                mid_price = spread_info.get("mid", 1000.0)
                
                # Update agent PnL using the mid price
                self.update_pnl(mid_price)
                
                # THINK
                if not has_market:
                    # Initial pricing when no other orders exist
                    mid_price = 1000.0
                    target_bid = int(mid_price) - (self.spread_ticks // 2)
                    target_ask = int(mid_price) + (self.spread_ticks // 2)
                else:
                    target_bid = int(mid_price) - (self.spread_ticks // 2)
                    target_ask = int(mid_price) + (self.spread_ticks // 2)
                    
                    # Prevent crossing our own spread
                    if target_bid >= target_ask:
                        target_ask = target_bid + 1
                
                # Check active orders
                active_bid_price = None
                active_ask_price = None
                
                if self.bid_order_id and self.bid_order_id in self.active_orders:
                    active_bid_price = self.active_orders[self.bid_order_id]["price"]
                if self.ask_order_id and self.ask_order_id in self.active_orders:
                    active_ask_price = self.active_orders[self.ask_order_id]["price"]
                
                # ACT
                # If target prices differ from active prices, cancel and replace
                if target_bid != active_bid_price:
                    if self.bid_order_id:
                        self.cancel_order(self.bid_order_id)
                        self.bid_order_id = None
                    self.bid_order_id, _ = self.submit_order("Buy", target_bid, self.order_size)

                if target_ask != active_ask_price:
                    if self.ask_order_id:
                        self.cancel_order(self.ask_order_id)
                        self.ask_order_id = None
                    self.ask_order_id, _ = self.submit_order("Sell", target_ask, self.order_size)
                    
            except Exception as e:
                self.log(f"Error in MM loop: {e}")
                
            await asyncio.sleep(interval)
            elapsed += interval

        # Cleanup on exit - cancel all remaining active orders
        self.log("Stopping Market Maker Agent - Cleaning up active orders")
        if self.bid_order_id:
            self.cancel_order(self.bid_order_id)
        if self.ask_order_id:
            self.cancel_order(self.ask_order_id)
