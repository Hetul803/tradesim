#!/usr/bin/env python3
import os, glob, csv, math

LOG_DIR = os.path.join(os.path.dirname(__file__), "..", "logs")

def newest_session_prefix():
    files = glob.glob(os.path.join(LOG_DIR, "*_trades.csv"))
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
    rows = []
    if not os.path.exists(path):
        return rows
    with open(path, newline="") as fp:
        r = csv.DictReader(fp)
        for row in r:
            try:
                qty = int(row["qty"])
                px  = float(row["px"])
                maker_client = row["maker_client"]
                taker_client = row["taker_client"]
                maker_side = row["maker_side"]
                taker_side = row["taker_side"]
                rows.append((maker_client, taker_client, maker_side, taker_side, qty, px))
            except Exception:
                pass
    return rows

def load_last_mid(session):
    path = os.path.join(LOG_DIR, f"{session}_book.csv")
    last_bid = last_ask = math.nan
    if not os.path.exists(path):
        return math.nan
    with open(path, newline="") as fp:
        r = csv.DictReader(fp)
        for row in r:
            try:
                hb = row["has_bid"] == "1"
                ha = row["has_ask"] == "1"
                if hb: last_bid = float(row["bid_px"])
                if ha: last_ask = float(row["ask_px"])
            except Exception:
                pass
    if not math.isnan(last_bid) and not math.isnan(last_ask):
        return 0.5*(last_bid + last_ask)
    if not math.isnan(last_bid): return last_bid
    if not math.isnan(last_ask): return last_ask
    return math.nan

def main():
    sess = newest_session_prefix()
    if not sess:
        print("No sessions found in logs/")
        return
    trades = load_trades(sess)
    if not trades:
        print(f"Session {sess}: No trades")
        return

    pos, cash = {}, {}

    for maker_client, taker_client, maker_side, taker_side, qty, px in trades:
        if maker_client:
            pos[maker_client]  = pos.get(maker_client, 0) + ( qty if maker_side=="BUY" else -qty )
            cash[maker_client] = cash.get(maker_client, 0.0) - ( qty*px if maker_side=="BUY" else -qty*px )
        if taker_client:
            pos[taker_client]  = pos.get(taker_client, 0) + ( qty if taker_side=="BUY" else -qty )
            cash[taker_client] = cash.get(taker_client, 0.0) - ( qty*px if taker_side=="BUY" else -qty*px )

    last_mid = load_last_mid(sess)
    print(f"=== Session {sess} ===")
    print(f"Last mid: {last_mid:.4f}" if not math.isnan(last_mid) else "Last mid: n/a")
    print()
    print(f"{'CLIENT':<16} {'POS(sh)':>8} {'CASH($)':>12} {'MtM_PnL($)':>12} {'Realized($)':>12} {'Unreal($)':>12}")
    print("-"*72)
    for client in sorted(set(pos.keys()) | set(cash.keys())):
        p = pos.get(client, 0)
        c = cash.get(client, 0.0)
        unreal = (p * last_mid) if not math.isnan(last_mid) else float('nan')
        mtm = c + (unreal if not math.isnan(unreal) else 0.0)
        print(f"{client:<16} {p:>8d} {c:>12.2f} {mtm:>12.2f} {c:>12.2f} {(unreal if not math.isnan(unreal) else 0.0):>12.2f}")

if __name__ == "__main__":
    main()

