import os
import sys
import asyncio
import abc

# Add mcp_server folder to path to import OrderbookBridge
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'mcp_server'))
from orderbook_bridge import OrderbookBridge

class BaseAgent(abc.ABC):
    def __init__(self, name, order_id_start):
        self.name = name
        self.bridge = OrderbookBridge()
        self._order_id_counter = order_id_start
        
        self.position = 0
        self.cash = 0.0
        self.pnl = 0.0
        self.active_orders = {}  # order_id -> dict of order details
        self.fills = []
        
    def next_order_id(self):
        self._order_id_counter += 1
        return self._order_id_counter

    def log(self, message):
        print(f"[{self.name}] {message}")

    def update_pnl(self, mid_price):
        # Mark to market valuation: cash + position * mid_price
        self.pnl = self.cash + (self.position * mid_price)
        return self.pnl

    def _process_fills(self, order_id, side, fills, leaves_qty):
        for fill in fills:
            price = fill["price"]
            qty = fill["qty"]
            
            self.fills.append(fill)
            
            # Position and cash updates
            if side.lower() == "buy":
                self.position += qty
                self.cash -= price * qty
                self.log(f"FILL: Bought {qty} @ {price}. Position: {self.position}")
            else:
                self.position -= qty
                self.cash += price * qty
                self.log(f"FILL: Sold {qty} @ {price}. Position: {self.position}")
                
        # If order is fully filled, remove from active
        if leaves_qty == 0 and order_id in self.active_orders:
            self.active_orders.pop(order_id, None)

    def submit_order(self, side, price, quantity, order_type="GTC"):
        order_id = self.next_order_id()
        self.log(f"SUBMIT: {side} {quantity} @ {price} ({order_type}) [ID: {order_id}]")
        
        # Save to active orders first
        self.active_orders[order_id] = {
            "side": side,
            "price": price,
            "quantity": quantity,
            "leaves_qty": quantity,
            "order_type": order_type
        }
        
        res = self.bridge.submit_order(self.name, order_id, side, price, quantity, order_type)
        
        if res.get("status") == "rejected":
            self.log(f"REJECTED: {res.get('text', 'No details')}")
            self.active_orders.pop(order_id, None)
            return order_id, res
            
        leaves_qty = res.get("leaves_qty", 0)
        self.active_orders[order_id]["leaves_qty"] = leaves_qty
        
        fills = res.get("fills", [])
        self._process_fills(order_id, side, fills, leaves_qty)
        
        return order_id, res

    def cancel_order(self, order_id):
        if order_id not in self.active_orders:
            return None
        self.log(f"CANCEL: Order ID {order_id}")
        res = self.bridge.cancel_order(order_id)
        if res.get("status") == "cancelled":
            self.active_orders.pop(order_id, None)
        return res

    @abc.abstractmethod
    async def run(self, duration_seconds):
        pass
