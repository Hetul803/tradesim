# Design Notes (Step 1)

- Objective: Single-symbol matching engine + CLI to demonstrate price-time priority.
- Data structures: map<price, deque<Order>> for bids/asks; FIFO per level.
- Matching rules: Market orders consume best-priced levels; limit orders match if they cross, else rest.
- Order IDs: monotonic counter for cancels and auditability; trades have maker/taker IDs.
- What I learned: (write 2–3 bullets)
- Next steps: TCP server & text protocol, sample bots, risk limits, PnL metrics, CSV logs.

