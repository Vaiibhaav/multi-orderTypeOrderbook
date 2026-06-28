import os
import ctypes
import json
import threading

class OrderbookBridge:
    _instance = None
    _lock = threading.Lock()

    def __new__(cls, *args, **kwargs):
        with cls._lock:
            if cls._instance is None:
                cls._instance = super(OrderbookBridge, cls).__new__(cls)
                cls._instance._init_bridge()
            return cls._instance

    def _init_bridge(self):
        # Find DLL path relative to this file
        dir_path = os.path.dirname(os.path.abspath(__file__))
        dll_path = os.path.join(dir_path, "..", "orderbook_engine.dll")
        if not os.path.exists(dll_path):
            dll_path = os.path.join(os.getcwd(), "orderbook_engine.dll")
            if not os.path.exists(dll_path):
                raise FileNotFoundError(f"orderbook_engine.dll not found at {dll_path}")

        # Load DLL
        self.dll = ctypes.CDLL(dll_path)

        # Set arg/res types
        self.dll.ob_create.argtypes = []
        self.dll.ob_create.restype = ctypes.c_void_p

        self.dll.ob_destroy.argtypes = [ctypes.c_void_p]
        self.dll.ob_destroy.restype = None

        self.dll.ob_reset.argtypes = [ctypes.c_void_p]
        self.dll.ob_reset.restype = None

        self.dll.ob_submit_fix.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        self.dll.ob_submit_fix.restype = ctypes.c_char_p

        self.dll.ob_get_snapshot.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.dll.ob_get_snapshot.restype = ctypes.c_char_p

        self.dll.ob_get_trade_history.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.dll.ob_get_trade_history.restype = ctypes.c_char_p

        self.dll.ob_analyze_spread.argtypes = [ctypes.c_void_p]
        self.dll.ob_analyze_spread.restype = ctypes.c_char_p

        self.dll.ob_size.argtypes = [ctypes.c_void_p]
        self.dll.ob_size.restype = ctypes.c_int

        # Create handle
        self.handle = self.dll.ob_create()
        self.lock = threading.Lock()

        # FIX tag mappings for response parsing
        self.TAG_MAP = {
            8: "begin_string",
            35: "msg_type",
            11: "order_id",
            54: "side",
            44: "price",
            38: "quantity",
            59: "time_in_force",
            39: "ord_status",
            150: "exec_type",
            31: "last_px",
            32: "last_qty",
            151: "leaves_qty",
            14: "cum_qty",
            58: "text"
        }

    def __del__(self):
        if hasattr(self, 'handle') and self.handle:
            self.dll.ob_destroy(self.handle)
            self.handle = None

    def reset(self):
        with self.lock:
            self.dll.ob_reset(self.handle)

    def size(self):
        with self.lock:
            return self.dll.ob_size(self.handle)

    def _parse_fix_message(self, fix_str):
        if not fix_str:
            return {}
        msg = {}
        parts = fix_str.split('|')
        for part in parts:
            if not part:
                continue
            subparts = part.split('=')
            if len(subparts) != 2:
                continue
            try:
                tag = int(subparts[0])
                val = subparts[1]
                
                # Format type conversion
                if tag in [11, 44, 38, 31, 32, 151, 14]:
                    val = int(val)
                elif tag == 58:
                    pass
                
                key = self.TAG_MAP.get(tag, f"tag_{tag}")
                msg[key] = val
            except ValueError:
                continue
        return msg

    def _parse_fix_response(self, response_bytes):
        if not response_bytes:
            return []
        response_str = response_bytes.decode('utf-8')
        lines = response_str.strip().split('\n')
        parsed_msgs = []
        for line in lines:
            if not line:
                continue
            parsed_msgs.append(self._parse_fix_message(line))
        return parsed_msgs

    def submit_order(self, agent_id, order_id, side, price, quantity, order_type="GTC"):
        # Map parameters to FIX tags
        # 35=D (NewOrderSingle)
        # Side: Buy=1, Sell=2
        # TimeInForce: GTC=1, IOC=3, FAK=4
        side_val = "1" if side.lower() == "buy" else "2"
        tif_val = "1"
        if order_type.upper() == "IOC":
            tif_val = "3"
        elif order_type.upper() == "FAK":
            tif_val = "4"

        fix_msg = f"8=FIX.4.2|35=D|11={order_id}|54={side_val}|44={price}|38={quantity}|59={tif_val}|40=2|"
        
        with self.lock:
            res_bytes = self.dll.ob_submit_fix(self.handle, fix_msg.encode('utf-8'))
            
        reports = self._parse_fix_response(res_bytes)
        raw_fix_str = res_bytes.decode('utf-8') if res_bytes else ""
        
        # Structure the result
        fills = []
        status = "new"
        leaves_qty = quantity
        
        for rep in reports:
            if rep.get("msg_type") == "3" and rep.get("order_id") == order_id:  # Reject
                return {"order_id": order_id, "status": "rejected", "text": rep.get("text", "Unknown rejection"), "fills": []}
            if rep.get("exec_type") == "F" and rep.get("order_id") == order_id:  # Fill
                fills.append({
                    "price": rep.get("last_px"),
                    "qty": rep.get("last_qty"),
                })
                leaves_qty = rep.get("leaves_qty", 0)
                if leaves_qty == 0:
                    status = "filled"
                else:
                    status = "partially_filled"
            elif rep.get("exec_type") == "0" and rep.get("order_id") == order_id:  # New Ack
                leaves_qty = rep.get("leaves_qty", quantity)
                status = "new"

        return {
            "order_id": order_id,
            "status": status,
            "leaves_qty": leaves_qty,
            "fills": fills,
            "raw_fix": raw_fix_str
        }

    def cancel_order(self, order_id):
        # 35=F (OrderCancelRequest)
        fix_msg = f"8=FIX.4.2|35=F|11={order_id}|"
        with self.lock:
            res_bytes = self.dll.ob_submit_fix(self.handle, fix_msg.encode('utf-8'))
            
        reports = self._parse_fix_response(res_bytes)
        for rep in reports:
            if rep.get("msg_type") == "3" and rep.get("order_id") == order_id:
                return {"order_id": order_id, "status": "failed", "text": rep.get("text")}
            if rep.get("exec_type") == "4" and rep.get("order_id") == order_id:  # Cancelled
                return {"order_id": order_id, "status": "cancelled"}
        return {"order_id": order_id, "status": "unknown"}

    def modify_order(self, order_id, side, price, quantity):
        # 35=G (OrderCancelReplaceRequest)
        side_val = "1" if side.lower() == "buy" else "2"
        fix_msg = f"8=FIX.4.2|35=G|11={order_id}|54={side_val}|44={price}|38={quantity}|"
        with self.lock:
            res_bytes = self.dll.ob_submit_fix(self.handle, fix_msg.encode('utf-8'))
            
        reports = self._parse_fix_response(res_bytes)
        
        fills = []
        status = "modified"
        
        for rep in reports:
            if rep.get("msg_type") == "3" and rep.get("order_id") == order_id:
                return {"order_id": order_id, "status": "rejected", "text": rep.get("text", "Unknown modification error"), "fills": []}
            if rep.get("exec_type") == "F" and rep.get("order_id") == order_id:  # Fill
                fills.append({
                    "price": rep.get("last_px"),
                    "qty": rep.get("last_qty"),
                })
                leaves_qty = rep.get("leaves_qty", 0)
                if leaves_qty == 0:
                    status = "filled"
                else:
                    status = "partially_filled"

        return {
            "order_id": order_id,
            "status": status,
            "fills": fills
        }

    def get_snapshot(self, depth=10):
        with self.lock:
            res_bytes = self.dll.ob_get_snapshot(self.handle, depth)
        return json.loads(res_bytes.decode('utf-8'))

    def get_trade_history(self, count=20):
        with self.lock:
            res_bytes = self.dll.ob_get_trade_history(self.handle, count)
        return json.loads(res_bytes.decode('utf-8'))

    def analyze_spread(self):
        with self.lock:
            res_bytes = self.dll.ob_analyze_spread(self.handle)
        return json.loads(res_bytes.decode('utf-8'))
