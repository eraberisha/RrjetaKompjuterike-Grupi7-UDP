# UDP File Server in C (Windows & Linux)

> **Project:** Client-Server File Management System using **UDP Protocol**  
> **Language:** C  
> **Platform:** Windows (MinGW) + Linux (cross-compatible)  
> **Team:** Era Berisha, Erblin Syla, Erëza Greiçevci, Fatos Rama  
> **Protocol:** UDP with session tracking, sequence numbers, ACKs, timeout, and admin priority  

---

## Project Description

This project implements a **lightweight, connectionless file server** using **UDP sockets** in C. It simulates a real-world file-sharing system where:

- **One admin client** has full control: download, delete, search, and view stats.
- **Multiple user clients** can only **read** files and list directories.
- The server **tracks active clients** using `client_id`, IP, and port.
- **30-second inactivity timeout** removes idle clients (with automatic reconnection).
- **Real-time traffic monitoring** via the `STATS` command and periodic logging to `server_stats.txt`.
- **File transfers** are split into **1400-byte chunks** with sequence numbers and ACKs for reliability over UDP.
- **Admin commands are processed with higher priority**.

The system is **fully cross-platform** (Windows + Linux) and uses **Winsock2** on Windows and **POSIX sockets** on Linux.


