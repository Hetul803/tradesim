#!/usr/bin/env python3
import re
import time
import streamlit as st
from trader_client import LineClient
from streamlit_autorefresh import st_autorefresh

st.set_page_config(page_title="TradeSim Trader", layout="wide")

# ---------------- Helpers ----------------

BOOK_RE = re.compile(r"BOOK\s+BID\s+(?P<bid>none|\d+@\d+(?:\.\d+)?)\s+\|\s+ASK\s+(?P<ask>none|\d+@\d+(?:\.\d+)?)")

def parse_book_line(s: str):
    """
    Parses 'BOOK BID 7@10.30 | ASK 5@10.40' (or 'none') -> (bid_qty,bid_px,ask_qty,ask_px)
    Returns None if parse fails.
    """
    m = BOOK_RE.search(s.strip())
    if not m:
        return None
    def parse_side(tok):
        if tok == "none":
            return (None, None)
        q, px = tok.split("@", 1)
        return (int(q), float(px))
    bq, bp = parse_side(m.group("bid"))
    aq, ap = parse_side(m.group("ask"))
    return bq, bp, aq, ap

def fmt_px(px):
    return f"{px:.2f}" if px is not None else "—"

def fmt_qty(q):
    return f"{q:d}" if q is not None else "—"

# ---------------- Sidebar: connection ----------------

st.sidebar.header("Connection")
host = st.sidebar.text_input("Host", value="127.0.0.1")
port = st.sidebar.number_input("Port", 1, 65535, value=5555, step=1)
client_name = st.sidebar.text_input("Your client name", value="student1")

auto = st.sidebar.checkbox("Auto-refresh", value=True)
interval_ms = st.sidebar.slider("Refresh interval (ms)", 500, 5000, value=1500, step=500)
if auto:
    st_autorefresh(interval=interval_ms, key="trader_ui_autorefresh")

if "conn" not in st.session_state:
    st.session_state.conn = None
if "greeting" not in st.session_state:
    st.session_state.greeting = ""
if "last_book_raw" not in st.session_state:
    st.session_state.last_book_raw = ""
if "trades_log" not in st.session_state:
    st.session_state.trades_log = []

def connect():
    conn = LineClient()
    greet = conn.connect(host, int(port))
    st.session_state.conn = conn
    st.session_state.greeting = greet
    st.toast("Connected")

def disconnect():
    conn = st.session_state.conn
    if conn:
        try:
            conn.send_line("QUIT")
        except Exception:
            pass
        conn.close()
    st.session_state.conn = None
    st.toast("Disconnected")

ccols = st.sidebar.columns(2)
if ccols[0].button("Connect", use_container_width=True):
    try:
        connect()
    except Exception as e:
        st.sidebar.error(f"Connect failed: {e}")
if ccols[1].button("Disconnect", use_container_width=True):
    disconnect()

# ---------------- Main header ----------------

st.title("AUM TradeSim — Trader UI")
if st.session_state.greeting:
    st.info(st.session_state.greeting)

connected = st.session_state.conn is not None

# ---------------- Top-of-book panel ----------------

# Always try to refresh BOOK when connected (for live mid/spread)
bid_qty = bid_px = ask_qty = ask_px = None
if connected:
    try:
        raw = st.session_state.conn.request_reply("BOOK")
        st.session_state.last_book_raw = raw
    except Exception as e:
        st.warning(f"BOOK failed: {e}")

parsed = parse_book_line(st.session_state.last_book_raw) if st.session_state.last_book_raw else None
if parsed:
    bid_qty, bid_px, ask_qty, ask_px = parsed

mid = (bid_px + ask_px) / 2.0 if (bid_px is not None and ask_px is not None) else None
spread = (ask_px - bid_px) if (bid_px is not None and ask_px is not None) else None

m1, m2, m3 = st.columns(3)
m1.metric("Best Bid", f"{fmt_qty(bid_qty)} @ {fmt_px(bid_px)}")
m2.metric("Best Ask", f"{fmt_qty(ask_qty)} @ {fmt_px(ask_px)}")
m3.metric("Mid / Spread", f"{fmt_px(mid)} / {fmt_px(spread)}")

# ---------------- Order entry ----------------

st.subheader("New Market Order")
mcols = st.columns(4)
m_side = mcols[0].selectbox("Side", ["BUY","SELL"])
m_qty  = mcols[1].number_input("Qty", 1, 1_000_000, value=5, step=1)
place_mkt = mcols[3].button("Place MARKET", disabled=not connected, use_container_width=True)
if place_mkt:
    try:
        cmd = f"NEW MARKET {m_side} {int(m_qty)} CLIENT {client_name}"
        reply = st.session_state.conn.request_reply(cmd)
        st.success(f"{cmd} → {reply}")
    except Exception as e:
        st.error(f"Order failed: {e}")

st.subheader("New Limit Order")
lcols = st.columns(5)
l_side = lcols[0].selectbox("Side ", ["BUY","SELL"], key="l_side")
l_qty  = lcols[1].number_input("Qty ", 1, 1_000_000, value=5, step=1, key="l_qty")
# If we have a mid, prefill price near mid for convenience
prefill_px = float(f"{(mid if mid is not None else 10.00):.2f}")
l_px   = lcols[2].number_input("Price ", 0.01, 1_000_000.0, value=prefill_px, step=0.01, format="%.2f", key="l_px")
place_lmt = lcols[4].button("Place LIMIT", disabled=not connected, use_container_width=True, key="l_button")
if place_lmt:
    try:
        cmd = f"NEW LIMIT {l_side} {int(l_qty)} @ {float(l_px):.2f} CLIENT {client_name}"
        reply = st.session_state.conn.request_reply(cmd)
        st.success(f"{cmd} → {reply}")
    except Exception as e:
        st.error(f"Order failed: {e}")

# ---------------- Trades blotter ----------------

st.subheader("Trades (server TRADES)")
tcols = st.columns(3)
if tcols[0].button("Refresh TRADES", disabled=not connected, use_container_width=True):
    try:
        # ask server to dump trades; read lines quickly
        st.session_state.conn.send_line("TRADES")
        lines = []
        # short non-blocking read window
        st.session_state.conn.sock.settimeout(0.15)
        try:
            while True:
                line = st.session_state.conn.read_line()
                if line:
                    lines.append(line)
                else:
                    break
        except Exception:
            pass
        st.session_state.conn.sock.settimeout(None)
        if not lines:
            lines = ["(no trades)"]
        st.session_state.trades_log = lines[-200:]  # keep last 200 lines
    except Exception as e:
        st.error(f"TRADES failed: {e}")

if tcols[1].button("Clear", use_container_width=True):
    st.session_state.trades_log = []

# Pretty print the last trades
if st.session_state.trades_log:
    # Extract "TRADE <qty>@<px>" if present
    recs = []
    for ln in st.session_state.trades_log:
        ln = ln.strip()
        if ln.startswith("TRADE "):
            body = ln[6:]
            if "@" in body:
                q, px = body.split("@", 1)
                try:
                    recs.append((int(q), float(px)))
                except:
                    pass
    if recs:
        # Make a simple 2-column table
        qtys = [r[0] for r in recs]
        pxs  = [r[1] for r in recs]
        c1, c2, c3 = st.columns([1,1,1])
        c1.metric("Trades (count)", f"{len(recs)}")
        c2.metric("Last Trade Px", f"{pxs[-1]:.2f}")
        c3.metric("Avg Trade Px", f"{(sum(pxs)/len(pxs)):.2f}")
        st.dataframe(
            {"Qty": qtys[::-1], "Price": [f"{p:.2f}" for p in pxs[::-1]]},
            use_container_width=True,
            hide_index=True
        )
    else:
        st.code("\n".join(st.session_state.trades_log), language="text")
else:
    st.text("(no trades yet)")

st.caption("Connected" if connected else "Not connected")

