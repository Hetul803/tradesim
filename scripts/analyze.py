#!/usr/bin/env python3
import os, glob, csv, math

LOG_DIR = os.path.join(os.path.dirname(__file__), "..", "logs")

def newest_session_prefix():
    files = glob.glob(os.path.join(LOG_DIR, "*_trades.csv"))
    if not files:
        return None
    # filenames look like logs/1693512345678901234_trades.csv
    # extract the numeric prefix
    pairs = []
    for f in files:
        base = os.path.basename(f)
        sess = base.split("_")[0]
        if sess.isdigit():
            pairs.append((int(sess), sess))
    if not pairs:
        return None
    pairs.sort(reverse=True)
    return pairs[0][1]

def load_trades(session):
    path = os.path.join(LOG_DIR, f"{session}_trades.csv")
    trades = []
    if not os.path.exists(path):
        return trades
    with open(path, newline="") as fp:
        r = csv.DictReader(fp)
        for row in r:
            try:
                ts = int(row["ts_ns"])
                qty = int(row["qty"])
                px  = float(row["px"])
                trades.append((ts, qty, px))
            except Exception:
                pass
    trades.sort(key=lambda x: x[0])
    return trades

def load_book(session):
    path = os.path.join(LOG_DIR, f"{session}_book.csv")
    rows = []
    if not os.path.exists(path):
        return rows
    with open(path, newline="") as fp:
        r = csv.DictReader(fp)
        for row in r:
            try:
                ts = int(row["ts_ns"])
                has_bid = row["has_bid"] == "1"
                has_ask = row["has_ask"] == "1"
                bid_px = float(row["bid_px"]) if row["bid_px"] else math.nan
                ask_px = float(row["ask_px"]) if row["ask_px"] else math.nan
                bid_qty = int(row["bid_qty"]) if row["bid_qty"] else 0
                ask_qty = int(row["ask_qty"]) if row["ask_qty"] else 0
                rows.append((ts, has_bid, bid_px, bid_qty, has_ask, ask_px, ask_qty))
            except Exception:
                pass
    rows.sort(key=lambda x: x[0])
    return rows

def vwap(trades):
    if not trades:
        return math.nan
    notional = sum(q*px for _, q, px in trades)
    volume   = sum(q for _, q, _ in trades)
    if volume == 0:
        return math.nan
    return notional / volume

def simple_vol_from_trades(trades):
    # crude volatility proxy: std of trade-to-trade log returns
    if len(trades) < 2:
        return math.nan
    rets = []
    last_px = trades[0][2]
    for _, _, px in trades[1:]:
        if px > 0 and last_px > 0:
            rets.append(math.log(px/last_px))
        last_px = px
    if not rets:
        return math.nan
    mu = sum(rets)/len(rets)
    var = sum((r-mu)**2 for r in rets)/(len(rets)-1) if len(rets)>1 else 0.0
    return math.sqrt(var)

def time_weighted_spread(book_rows):
    # average (ask-bid) over snapshots which have both sides
    spreads = []
    for _, hb, bp, _, ha, ap, _ in book_rows:
        if hb and ha and not(math.isnan(bp) or math.isnan(ap)):
            spreads.append(ap - bp)
    if not spreads:
        return math.nan
    return sum(spreads)/len(spreads)

def main():
    session = newest_session_prefix()
    if not session:
        print("No logs found in ./logs. Run the server + bots first.")
        return
    trades = load_trades(session)
    book   = load_book(session)

    # Metrics
    trade_count = len(trades)
    total_qty   = sum(q for _, q, _ in trades)
    turnover    = sum(q*px for _, q, px in trades)
    vwap_px     = vwap(trades)
    twap_spread = time_weighted_spread(book)
    vol_proxy   = simple_vol_from_trades(trades)

    print(f"=== Session {session} ===")
    print(f"Trades: {trade_count}")
    print(f"Total Qty: {total_qty}")
    print(f"Turnover: {turnover:.2f}")
    print(f"VWAP: {vwap_px:.4f}" if not math.isnan(vwap_px) else "VWAP: n/a")
    print(f"Avg Spread (time-weighted): {twap_spread:.4f}" if not math.isnan(twap_spread) else "Avg Spread: n/a")
    print(f"Volatility proxy (std of log-returns): {vol_proxy:.6f}" if not math.isnan(vol_proxy) else "Volatility proxy: n/a")
    print()
    print("Tip: Commit logs/ to .gitignore if you don’t want large data in Git.")

if __name__ == "__main__":
    main()

