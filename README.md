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

## 📦 Quick Start

### Clone the Repository
```bash
git clone https://github.com/Hetul803/tradesim.git
cd tradesim




