#!/usr/bin/env python3
import os, glob, math
import pandas as pd
import streamlit as st
from streamlit_autorefresh import st_autorefresh  # << add-on for timed refresh

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
LOG_DIR = os.path.join(ROOT, "logs")

def list_sessions():
    trades = glob.glob(os.path.join(LOG_DIR, "*_trades.csv"))
    sess = []
    for f in trades:
        base = os.path.basename(f)
        prefix = base.split("_")[0]
        if prefix.isdigit():
            sess.append(int(prefix))
    return sorted(sess, reverse=True)

@st.cache_data(ttl=5.0)
def load_trades(session):
    path = os.path.join(LOG_DIR, f"{session}_trades.csv")
    if not os.path.exists(path):
        return pd.DataFrame(columns=["ts_ns","maker_id","taker_id","qty","px",
                                     "maker_client","taker_client","maker_side","taker_side"])
    df = pd.read_csv(path)
    for col in ["maker_client","taker_client","maker_side","taker_side"]:
        if col not in df.columns:
            df[col] = ""
    return df

@st.cache_data(ttl=5.0)
def load_book(session):
    path = os.path.join(LOG_DIR, f"{session}_book.csv")
    if not os.path.exists(path):
        return pd.DataFrame(columns=["ts_ns","has_bid","bid_px","bid_qty","has_ask","ask_px","ask_qty"])
    return pd.read_csv(path)

def compute_mid(book_df):
    df = book_df.copy()
    df["bid_px"] = pd.to_numeric(df["bid_px"], errors="coerce")
    df["ask_px"] = pd.to_numeric(df["ask_px"], errors="coerce")
    df["mid"] = (df["bid_px"] + df["ask_px"]) / 2
    return df

def compute_spread(book_df):
    df = book_df.copy()
    df["bid_px"] = pd.to_numeric(df["bid_px"], errors="coerce")
    df["ask_px"] = pd.to_numeric(df["ask_px"], errors="coerce")
    df["spread"] = df["ask_px"] - df["bid_px"]
    return df

def compute_client_pnl(trades_df, last_mid):
    if trades_df.empty:
        return pd.DataFrame(columns=["client","pos","cash","unreal","mtm"])
    m = trades_df[["maker_client","maker_side","qty","px"]].rename(
        columns={"maker_client":"client", "maker_side":"side"}
    )
    t = trades_df[["taker_client","taker_side","qty","px"]].rename(
        columns={"taker_client":"client", "taker_side":"side"}
    )
    df = pd.concat([m, t], ignore_index=True)
    df = df[df["client"] != ""]
    df["qty"] = pd.to_numeric(df["qty"], errors="coerce").fillna(0).astype(int)
    df["px"]  = pd.to_numeric(df["px"], errors="coerce").fillna(0.0)
    df["pos_delta"]  = df.apply(lambda r: r["qty"] if r["side"]=="BUY" else -r["qty"], axis=1)
    df["cash_delta"] = df.apply(lambda r: -r["qty"]*r["px"] if r["side"]=="BUY" else r["qty"]*r["px"], axis=1)
    agg = df.groupby("client").agg(pos=("pos_delta","sum"), cash=("cash_delta","sum")).reset_index()
    if pd.isna(last_mid):
        agg["unreal"] = float("nan"); agg["mtm"] = agg["cash"]
    else:
        agg["unreal"] = agg["pos"] * last_mid; agg["mtm"] = agg["cash"] + agg["unreal"]
    return agg.sort_values("mtm", ascending=False)

def main():
    st.set_page_config(page_title="TradeSim Dashboard", layout="wide")
    st.title("AUM TradeSim — Live Dashboard")

    # Sidebar: session + refresh
    sessions = list_sessions()
    if not sessions:
        st.info("No log sessions found in ./logs. Start the server and bots to generate data.")
        return

    session = st.sidebar.selectbox("Session", sessions, index=0, format_func=str)
    auto = st.sidebar.checkbox("Auto-refresh", value=True)
    interval_ms = st.sidebar.slider("Refresh interval (ms)", min_value=500, max_value=5000, value=2000, step=500)
    if auto:
        st_autorefresh(interval=interval_ms, key="tradesim_autorefresh")

    trades_df = load_trades(session)
    book_df   = load_book(session)
    mid_df    = compute_mid(book_df)
    spread_df = compute_spread(book_df)

    # KPIs
    c1, c2, c3, c4 = st.columns(4)
    trade_count = len(trades_df)
    total_qty   = trades_df["qty"].sum() if "qty" in trades_df else 0
    turnover    = (trades_df["qty"] * trades_df["px"]).sum() if trade_count>0 else 0.0
    vwap        = (trades_df["qty"] * trades_df["px"]).sum()/total_qty if total_qty>0 else float("nan")
    c1.metric("Trades", f"{trade_count}")
    c2.metric("Volume (sh)", f"{int(total_qty)}")
    c3.metric("Turnover ($)", f"{turnover:,.2f}")
    c4.metric("VWAP", f"{vwap:.4f}" if not math.isnan(vwap) else "n/a")

    # last mid
    last_mid = float("nan")
    if not mid_df.empty:
        mids = mid_df["mid"].dropna()
        if not mids.empty:
            last_mid = float(mids.iloc[-1])

    # charts
    left, right = st.columns(2)
    with left:
        st.subheader("Mid Price")
        if not mid_df.empty and mid_df["mid"].notna().any():
            st.line_chart(mid_df[["mid"]].dropna())
        else:
            st.write("No mid-price data yet (ensure clients call BOOK).")
    with right:
        st.subheader("Bid-Ask Spread")
        if not spread_df.empty and spread_df["spread"].notna().any():
            st.line_chart(spread_df[["spread"]].dropna())
        else:
            st.write("No spread data yet.")

    # per-client PnL
    st.subheader("Per-client PnL / Position")
    pnl = compute_client_pnl(trades_df, last_mid)
    st.dataframe(pnl, use_container_width=True)

if __name__ == "__main__":
    main()

