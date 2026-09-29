
## The basics

Sockets let two processes exchange data whether they're on the same machine or across a network. The two main flavors for IPC are:

- **Unix domain sockets (AF_UNIX):** local-only, addressed by a filesystem path, and faster than TCP on loopback. This is usually the best choice for processes on the same host.
- **Internet sockets (AF_INET/AF_INET6):** TCP or UDP, addressed by IP and port. They work across machines, or locally via `127.0.0.1`.

The typical flow is:

- **Server:** `socket()` → `bind()` → `listen()` → `accept()` → `recv()`/`send()` → `close()`
- **Client:** `socket()` → `connect()` → `send()`/`recv()` → `close()`

## Example: Unix domain socket (Python)

**server.py**
```python
import socket, os

PATH = "/tmp/demo.sock"
if os.path.exists(PATH):
    os.remove(PATH)

srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
srv.bind(PATH)
srv.listen(1)
print("Waiting for client...")

conn, _ = srv.accept()
with conn:
    data = conn.recv(1024)
    print("Received:", data.decode())
    conn.sendall(b"Hello from server")
srv.close()
os.remove(PATH)
```

**client.py**
```python
import socket

cli = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
cli.connect("/tmp/demo.sock")
cli.sendall(b"Hello from client")
print("Reply:", cli.recv(1024).decode())
cli.close()
```

Run `server.py` first, then `client.py` in another terminal.

## Things that commonly trip people up

- **TCP is a byte stream, not messages.** One `send()` doesn't guarantee one `recv()`. Use length-prefixing or delimiters to frame messages.
- **Always handle partial reads and writes.** Use `sendall()` in Python, or loop in C.
- **Clean up socket files.** A stale Unix socket path causes "Address already in use" on `bind()`.
- **Handle multiple clients** with `fork()`, threads, or `select`/`poll`/`epoll`.

Is this for a course lab or assignment, and if so, do you need it in C, Python, or another language? If you share the requirements, I can tailor the code to them.
