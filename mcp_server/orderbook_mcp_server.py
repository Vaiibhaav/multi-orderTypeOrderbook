from mcp.server.fastmcp import FastMCP
from orderbook_bridge import OrderbookBridge
import json
import threading

# Initialize FastMCP Server
mcp = FastMCP("C++ Orderbook Engine")

# Thread-safe order ID counter for MCP client orders
_order_id_counter = 10000
_counter_lock = threading.Lock()

def get_next_mcp_order_id():
    global _order_id_counter
    with _counter_lock:
        _order_id_counter += 1
        return _order_id_counter

@mcp.tool()
def submit_order(side: str, price: int, quantity: int, order_type: str = "GTC", agent_id: str = "LLM_USER") -> str:
    """
    Submit a new limit order to the matching engine.
    - side: 'Buy' or 'Sell'
    - price: Integer price (tick size = 1)
    - quantity: Integer quantity to trade
    - order_type: 'GTC' (Good Till Cancel), 'IOC' (Immediate Or Cancel), or 'FAK' (Fill And Kill)
    - agent_id: Identifier of the agent placing the order
    """
    bridge = OrderbookBridge()
    order_id = get_next_mcp_order_id()
    try:
        # Validate side
        if side.lower() not in ["buy", "sell"]:
            return json.dumps({"error": f"Invalid side: {side}. Must be 'Buy' or 'Sell'."})
        # Validate order type
        if order_type.upper() not in ["GTC", "IOC", "FAK"]:
            return json.dumps({"error": f"Invalid order_type: {order_type}. Must be 'GTC', 'IOC', or 'FAK'."})
        
        res = bridge.submit_order(agent_id, order_id, side, price, quantity, order_type)
        return json.dumps(res)
    except Exception as e:
        return json.dumps({"error": str(e)})

@mcp.tool()
def cancel_order(order_id: int) -> str:
    """
    Cancel an existing active order in the orderbook using its order_id.
    """
    bridge = OrderbookBridge()
    try:
        res = bridge.cancel_order(order_id)
        return json.dumps(res)
    except Exception as e:
        return json.dumps({"error": str(e)})

@mcp.tool()
def modify_order(order_id: int, side: str, price: int, quantity: int) -> str:
    """
    Modify an existing active order. This cancels the original order and replaces it with new price/quantity.
    - side: 'Buy' or 'Sell'
    - price: New price
    - quantity: New quantity
    """
    bridge = OrderbookBridge()
    try:
        if side.lower() not in ["buy", "sell"]:
            return json.dumps({"error": f"Invalid side: {side}. Must be 'Buy' or 'Sell'."})
        res = bridge.modify_order(order_id, side, price, quantity)
        return json.dumps(res)
    except Exception as e:
        return json.dumps({"error": str(e)})

@mcp.tool()
def get_snapshot(depth: int = 10) -> str:
    """
    Get the current market depth (bid and ask ladders).
    - depth: Max number of levels to return for bids and asks
    """
    bridge = OrderbookBridge()
    try:
        snapshot = bridge.get_snapshot(depth)
        return json.dumps(snapshot)
    except Exception as e:
        return json.dumps({"error": str(e)})

@mcp.tool()
def get_trade_history(count: int = 20) -> str:
    """
    Retrieve the historical trades matched by the engine.
    - count: Max number of recent trades to return
    """
    bridge = OrderbookBridge()
    try:
        history = bridge.get_trade_history(count)
        return json.dumps(history)
    except Exception as e:
        return json.dumps({"error": str(e)})

@mcp.tool()
def analyze_spread() -> str:
    """
    Get key statistics including best bid, best ask, mid-price, spread, total depth, and orderbook imbalance.
    """
    bridge = OrderbookBridge()
    try:
        analysis = bridge.analyze_spread()
        return json.dumps(analysis)
    except Exception as e:
        return json.dumps({"error": str(e)})

@mcp.tool()
def reset_book() -> str:
    """
    Clear all orders from the orderbook and reset the state.
    """
    bridge = OrderbookBridge()
    try:
        bridge.reset()
        return json.dumps({"status": "success", "message": "Orderbook reset successfully."})
    except Exception as e:
        return json.dumps({"error": str(e)})

if __name__ == "__main__":
    mcp.run("stdio")
