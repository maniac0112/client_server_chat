# Client-Server Chat (C++ using `select()`)

A lightweight multi-client chat server implemented in C++ using the POSIX `select()`  and `POLL()` API. The server handles multiple client connections concurrently and supports real-time message broadcasting from the server console.

---

## 🚀 Features

- ✅ Supports multiple clients using a single-threaded `select()` and `POLL()` API.
- ✅ Server can send messages to all connected clients via `stdin`
- ✅ Thread-safe message queue using `std::queue` + `std::mutex`
- ✅ Gracefully handles client disconnections
- ✅ Uses low-level system calls: `socket`, `bind`, `listen`, `accept`, `read`, `send`, `select`

---

## 🛠️ Build Instructions

Make sure you're compiling on a Unix-like system (Linux/macOS or WSL on Windows).

```bash
g++ -std=c++17 -pthread server.cpp -o chat_server
