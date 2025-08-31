#!/usr/bin/env python3
import os
import time
import streamlit as st
from trader_client import LineClient

st.set_page_config(page_title="TradeSim Trader", layout="wide")

# --- Sidebar: connection & identity ---
st.sidebar.header("Connection")
default_host = "localhost"
default_port = 5555

host = st.sidebar.text_input("Host", value=default_host)
port = st.sidebar.number_input("Port", min_value=1, max_value=65535, value=default_port, step=1)
client_name = st.sidebar.text_input("Your client name", value="student1")

if "conn" not in st.session_state:
    st.session_state.conn = None
if "greeting" not in st.session_state:
    st.session_state.greeting = ""
if "last_book" not in st.session_state:
    st.session_state.last_book = ""
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

cols = st.sidebar.columns(2)
if cols[0].button("Connect", use_container_width=True):
    try:
        connect()
    except Exception as e:
        st.sidebar.error(f"Connect failed: {e}")

if cols[1].button("Disconnect", use_container_width=True):
    disconnect()

st.sidebar.caption("Tip: choose a unique client name per student.")

# --- Main: status ---
st.title("AUM TradeSim — Trader UI")
st.write("Place orders and query the book/trades in real time.")

if st.session_state.greeting:
    st.info(f"Server: {st.session_state.greeting}")

connected = st.session_state.conn is not None

# --- Query widgets ---
qcols = st.columns(3)
if qcols[0].button("BOOK (Top of Book)", disabled=not connected, use_container_width=True):
    try:
        msg = st.session_state.conn.request_reply("BOOK")
        st.session_state.last_book = msg
    except Exception as e:
        st.error(f"BOOK failed: {e}")

if qcols[1].button("TRADES (List)", disabled=not connected, use_container_width=True):
    try:
        # server may stream multiple lines; read until a break (heuristic)
        st.session_state.conn.send_line("TRADES")
        # Read the first line; if "(no trades)" or "TRADE ..." we keep reading until no more immediately
        lines = []
        st.session_state.conn.sock.settimeout(0.2)
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
        st.session_state.trades_log = lines
    except Exception as e:
        st.error(f"TRADES failed: {e}")

if qcols[2].button("Clear Log", use_container_width=True):
    st.session_state.trades_log = []
    st.session_state.last_book = ""

# --- New Market Order ---
st.subheader("New Market Order")
mcols = st.columns(4)
m_side = mcols[0].selectbox("Side", ["BUY","SELL"])
m_qty  = mcols[1].number_input("Qty", min_value=1, max_value=1_000_000, value=5, step=1)
place_mkt = mcols[3].button("Place MARKET", disabled=not connected, use_container_width=True)
if place_mkt:
    try:
        cmd = f"NEW MARKET {m_side} {m_qty} CLIENT {client_name}"
        reply = st.session_state.conn.request_reply(cmd)
        st.success(f"{cmd} → {reply}")
    except Exception as e:
        st.error(f"Order failed: {e}")

# --- New Limit Order ---
st.subheader("New Limit Order")
lcols = st.columns(5)
l_side = lcols[0].selectbox("Side ", ["BUY","SELL"], key="l_side")
l_qty  = lcols[1].number_input("Qty ", min_value=1, max_value=1_000_000, value=5, step=1, key="l_qty")
l_px   = lcols[2].number_input("Price ", min_value=0.01, max_value=1_000_000.0, value=10.00, step=0.01, format="%.2f", key="l_px")
place_lmt = lcols[4].button("Place LIMIT", disabled=not connected, use_container_width=True, key="l_button")
if place_lmt:
    try:
        cmd = f"NEW LIMIT {l_side} {int(l_qty)} @ {float(l_px):.2f} CLIENT {client_name}"
        reply = st.session_state.conn.request_reply(cmd)
        st.success(f"{cmd} → {reply}")
    except Exception as e:
        st.error(f"Order failed: {e}")

# --- Last book + Trades log ---
st.divider()
bcol, tcol = st.columns(2)
with bcol:
    st.markdown("**Last BOOK**")
    st.code(st.session_state.last_book or "(press BOOK)", language="text")
with tcol:
    st.markdown("**Trades Log (server TRADES)**")
    if st.session_state.trades_log:
        st.code("\n".join(st.session_state.trades_log), language="text")
    else:
        st.text("(no trades yet)")

# Footer
st.caption("Connected" if connected else "Not connected")

