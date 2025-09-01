# AUM TradeSim — Educational Trading Simulator

**Author:** Hetul Patel  
**Institution:** Auburn University at Montgomery (AUM)  
**Owner:** Hetul Patel (GitHub: [Hetul803](https://github.com/Hetul803))  

---

## 📖 Overview
AUM TradeSim is a **C++ order-matching engine and trading simulator** designed to teach students, professors, and researchers about **financial market microstructure, algorithmic trading, and strategy design**.

This platform simulates a live exchange where users can place **limit and market orders**, view the **order book**, analyze **PnL**, and compete in **classroom trading competitions**. It features automated bots, a modular design, and browser-based UIs for ease of use.

---

## 🛠 Tech Stack
- **Core Engine:** C++17 (clang++)
- **Networking:** TCP sockets (POSIX)
- **Bots:** Market-making and random order bots in C++
- **Frontend:** Python 3.11 with Streamlit for UI & dashboards
- **Analytics:** Pandas-based analytics, CSV log storage
- **Platforms:** macOS & Linux (tested on macOS Ventura)

---

## 🚀 Features
- Matching engine with live order book and trade logging
- Limit and market order support
- Per-client **PnL and position tracking**
- Automated bots to simulate liquidity and randomness
- Streamlit-based **Trader UI** to place orders in a web browser
- Streamlit **Dashboard** to visualize trades, spread, mid-price, and leaderboards
- Modular design for extending with new symbols, depth views, and strategy support

---
## 📂 Project Structure

```text
tradesim/
├── Makefile               # Build instructions
├── README.md              # Project documentation
├── cli/                   # CLI client
├── engine/                # Matching engine code
├── net/                   # TCP networking code
├── bots/                  # Market-making and random bots
├── scripts/               # Analytics and UIs
│   ├── dashboard.py       # Streamlit dashboard
│   ├── trade_ui.py        # Trader web UI
│   ├── trader_client.py   # Python TCP client library
│   └── pnl.py             # PnL analysis script
└── logs/                  # Trade & book CSV logs (gitignored)

---



```
## 📦 Quick Start
```bash
# Clone the repository
git clone https://github.com/Hetul803/tradesim.git
cd tradesim

# macOS dependencies
xcode-select --install
brew install clang-format make python

# Linux dependencies
sudo apt-get update
sudo apt-get install build-essential clang python3-pip python3-venv

# Build project
make

Running the Simulator
# Start the matching engine
./build/tradesim_server

# Run trading bots in separate terminals (optional)
./build/bot_mm localhost 5555 mm1 200 200
./build/bot_random localhost 5555 rand1 300 150

# Trader UI (browser-based)
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install streamlit pandas
streamlit run scripts/trade_ui.py

# Dashboard (analytics view)
streamlit run scripts/dashboard.py

# CLI PnL report
./scripts/pnl.py
```
👨‍🏫 Classroom and Research Use
AUM TradeSim was developed by Hetul Patel (MSCS) as a teaching and research platform for:
Quantitative finance
Algorithmic trading
Fintech and software engineering education
Market microstructure studies
Suggested Classroom Setup
Instructor runs a central server (lab computer or cloud VM).
Students connect with Trader UI from their laptops.
Bots provide liquidity and competition.
Dashboard tracks performance and leaderboard results in real time.


🧩 Roadmap
```bash
Order IDs and cancel functionality
Full order book depth visualization
Multiple trading symbols
Docker image for one-command setup
Advanced analytics (queue depth, latency)
WebSocket streaming for low-latency dashboards
Student competition leaderboard

```
📝 License & Ownership
```bash
This simulator is owned and maintained by Hetul Patel.
Developed for educational and research purposes at Auburn University at Montgomery.
You are welcome to fork, contribute, or extend this code, but please credit:
Hetul Patel.
GitHub: Hetul803
