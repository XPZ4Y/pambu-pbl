# Client–Server Communication

## The basics

In the client–server model, a **server** waits for requests and a **client** starts the conversation by sending one. The server does the work and sends back a response. One server usually handles many clients.

- **Server:** passive, always listening on a known address (IP + port).
- **Client:** active, connects when it needs something.
- **Protocol:** the agreed rules for messages (e.g. HTTP, FTP, SMTP, or your own).

The typical flow is:

- **Server:** `socket()` → `bind()` → `listen()` → `accept()` → `recv()` / `send()` → `close()`
- **Client:** `socket()` → `connect()` → `send()` / `recv()` → `close()`

Request–response over the network:

```
Client                         Server
  | --- connect (TCP handshake) --> |
  | --- request  ----------------> |
  | <--- response ---------------- |
  | --- close  ------------------> |
```

## Example: TCP client–server over the network (Python)

### server.py

```python
import socket

HOST, PORT = "127.0.0.1", 5000

srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind((HOST, PORT))
srv.listen(5)
print(f"Server listening on {HOST}:{PORT}")

while True:
    conn, addr = srv.accept()
    with conn:
        print("Connected by", addr)
        data = conn.recv(1024)
        if not data:
            continue
        print("Request:", data.decode())
        conn.sendall(b"Hello from server: " + data)
```

### client.py

```python
import socket

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as cli:
    cli.connect(("127.0.0.1", 5000))
    cli.sendall(b"Hello from client")
    print("Reply:", cli.recv(1024).decode())
```

Run `server.py` first, then `client.py` in another terminal. To use it across machines, bind the server to `0.0.0.0` and connect the client to the server's IP.

## Handling multiple clients

The server above serves one client at a time. Simple fix: a thread per client.

```python
import socket, threading

def handle(conn, addr):
    with conn:
        while True:
            data = conn.recv(1024)
            if not data:
                break
            conn.sendall(b"Echo: " + data)

srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("127.0.0.1", 5000))
srv.listen()

while True:
    conn, addr = srv.accept()
    threading.Thread(target=handle, args=(conn, addr), daemon=True).start()
```

Other options: `select` / `poll` / `epoll`, or `asyncio` for many connections.

## TCP vs UDP

- **TCP (`SOCK_STREAM`):** connection-based, reliable, ordered. Used by HTTP, SSH, email.
- **UDP (`SOCK_DGRAM`):** connectionless, faster, no delivery guarantee. Used by DNS, video calls, games.

## Things that commonly trip people up

- **TCP is a byte stream, not messages.** One `send()` doesn't guarantee one `recv()`. Use length-prefixing or delimiters (e.g. `\n`) to frame messages.
- **Start the server before the client**, or you get `ConnectionRefusedError`.
- **"Address already in use"** after restarting the server: set `SO_REUSEADDR` before `bind()`.
- **Port must match** on both sides, and ports below 1024 usually need admin/root.
- **Firewalls** can block connections across machines.
- **Always close sockets** (use `with`) and handle disconnects (`recv()` returning `b""` means the peer closed).
