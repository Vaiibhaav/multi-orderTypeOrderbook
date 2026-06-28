import asyncio
from base_agent import BaseAgent

class MomentumAgent(BaseAgent):
    def __init__(self, name, order_id_start, imbalance_threshold=0.5, trade_size=75, cooldown=1.0):
        super().__init__(name, order_id_start)
        self.imbalance_threshold = imbalance_threshold
        self.trade_size = trade_size
        self.cooldown = cooldown

    async def run(self, duration_seconds):
        self.log("Starting Momentum Agent")
        elapsed = 0.0
        
        while elapsed < duration_seconds:
            sleep_time = 0.1
            try:
                # OBSERVE
                spread_info = self.bridge.analyze_spread()
                has_market = spread_info.get("has_market", False)
                mid_price = spread_info.get("mid", 1000.0)
                imbalance = spread_info.get("imbalance", 0.0)
                best_bid = spread_info.get("best_bid", 0)
                best_ask = spread_info.get("best_ask", 0)
                
                # Update PnL
                self.update_pnl(mid_price)
                
                if has_market:
                    # THINK
                    if imbalance > self.imbalance_threshold:
                        # Bullish momentum -> Buy at best ask
                        self.log(f"Bullish signal (imbalance: {imbalance:.2f}) -> Buying {self.trade_size} @ {best_ask} FAK")
                        # ACT
                        self.submit_order("Buy", best_ask, self.trade_size, "FAK")
                        sleep_time = self.cooldown
                    elif imbalance < -self.imbalance_threshold:
                        # Bearish momentum -> Sell at best bid
                        self.log(f"Bearish signal (imbalance: {imbalance:.2f}) -> Selling {self.trade_size} @ {best_bid} FAK")
                        # ACT
                        self.submit_order("Sell", best_bid, self.trade_size, "FAK")
                        sleep_time = self.cooldown
                        
            except Exception as e:
                self.log(f"Error in Momentum loop: {e}")
                
            await asyncio.sleep(sleep_time)
            elapsed += sleep_time
            
        self.log("Stopping Momentum Agent")
