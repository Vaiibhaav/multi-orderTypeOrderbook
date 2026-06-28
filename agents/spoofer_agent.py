import asyncio
from base_agent import BaseAgent

class SpooferAgent(BaseAgent):
    def __init__(self, name, order_id_start, spoof_size=3000, trade_size=150, interval=2.0):
        super().__init__(name, order_id_start)
        self.spoof_size = spoof_size
        self.trade_size = trade_size
        self.interval = interval
        self.cycle_count = 0

    async def run(self, duration_seconds):
        self.log("Starting Spoofer Agent")
        elapsed = 0.0
        
        while elapsed < duration_seconds:
            try:
                # OBSERVE
                spread_info = self.bridge.analyze_spread()
                has_market = spread_info.get("has_market", False)
                mid_price = spread_info.get("mid", 1000.0)
                best_bid = spread_info.get("best_bid", 0)
                best_ask = spread_info.get("best_ask", 0)
                
                self.update_pnl(mid_price)
                
                if has_market:
                    # THINK
                    is_buy_spoof = (self.cycle_count % 2 == 0)
                    
                    if is_buy_spoof:
                        # Spoof on Buy side, execute Sell trade
                        spoof_price = best_bid
                        trade_price = best_bid  # Hit the bid
                        
                        # ACT Phase 1: Submit huge spoof bid
                        self.log(f"Cycle {self.cycle_count}: Posting SPOOF BUY order of {self.spoof_size} @ {spoof_price}")
                        spoof_id, _ = self.submit_order("Buy", spoof_price, self.spoof_size)
                        
                        # Phase 2: Wait briefly
                        await asyncio.sleep(0.3)
                        elapsed += 0.3
                        
                        # Phase 3: Submit profit sell order
                        self.log(f"Cycle {self.cycle_count}: Executing actual SELL order of {self.trade_size} @ {trade_price}")
                        self.submit_order("Sell", trade_price, self.trade_size, "FAK")
                        
                        # Phase 4: Cancel spoof bid
                        self.log(f"Cycle {self.cycle_count}: Cancelling SPOOF BUY order [ID: {spoof_id}]")
                        self.cancel_order(spoof_id)
                        
                    else:
                        # Spoof on Sell side, execute Buy trade
                        spoof_price = best_ask
                        trade_price = best_ask  # Lift the offer
                        
                        # ACT Phase 1: Submit huge spoof offer
                        self.log(f"Cycle {self.cycle_count}: Posting SPOOF SELL order of {self.spoof_size} @ {spoof_price}")
                        spoof_id, _ = self.submit_order("Sell", spoof_price, self.spoof_size)
                        
                        # Phase 2: Wait briefly
                        await asyncio.sleep(0.3)
                        elapsed += 0.3
                        
                        # Phase 3: Submit profit buy order
                        self.log(f"Cycle {self.cycle_count}: Executing actual BUY order of {self.trade_size} @ {trade_price}")
                        self.submit_order("Buy", trade_price, self.trade_size, "FAK")
                        
                        # Phase 4: Cancel spoof offer
                        self.log(f"Cycle {self.cycle_count}: Cancelling SPOOF SELL order [ID: {spoof_id}]")
                        self.cancel_order(spoof_id)

                    self.cycle_count += 1
                    
            except Exception as e:
                self.log(f"Error in Spoofer loop: {e}")
                
            await asyncio.sleep(self.interval)
            elapsed += self.interval
            
        self.log("Stopping Spoofer Agent")
