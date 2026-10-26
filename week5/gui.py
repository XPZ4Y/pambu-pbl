#!/usr/bin/env python3
"""
gui.py — Tkinter frontend for page_sim_backend.

Talks to the C backend over an AF_UNIX stream socket using newline-
delimited JSON. Zero third-party dependencies.

Protocol (see backend.c):
    request:  {"cmd":"run","refs":[...],"frames":N,"algo":1..4}
              {"cmd":"ping"}
              {"cmd":"quit"}
    response: {"ok":true,"v":1,"cmd":"run","best":i,"results":[...]}
              {"ok":false,"v":1,"status":-N,"error":"..."}

Launch:
    ./page_sim_backend &            # or: make run
    python3 gui.py [socket-path]

Socket path defaults to $PAGE_SIM_SOCKET or ./page_sim.sock.
"""

import json
import os
import socket
import sys
import tkinter as tk
from tkinter import messagebox, ttk

SOCK_DEFAULT = os.environ.get("PAGE_SIM_SOCKET", "./page_sim.sock")
PROTO_EXPECTED = 1
TIMEOUT_S = 5.0


class Backend:
    """Thin client for the IPC backend. One request per call."""

    def __init__(self, path: str):
        self.path = path
        self.sock = None

    def connect(self):
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.settimeout(TIMEOUT_S)
        try:
            s.connect(self.path)
        except OSError as e:
            s.close()
            raise RuntimeError(f"cannot connect to {self.path}: {e}") from e
        self.sock = s
        # Handshake: verify protocol version up front.
        try:
            pong = self.request({"cmd": "ping"})
        except Exception:
            self.close()
            raise
        v = pong.get("v", -1)
        if v != PROTO_EXPECTED:
            self.close()
            raise RuntimeError(
                f"protocol mismatch: backend speaks v{v}, "
                f"frontend expects v{PROTO_EXPECTED}"
            )

    def request(self, obj: dict) -> dict:
        if self.sock is None:
            raise RuntimeError("not connected")
        line = json.dumps(obj, separators=(",", ":")) + "\n"
        self.sock.sendall(line.encode("utf-8"))
        buf = bytearray()
        while True:
            chunk = self.sock.recv(65536)
            if not chunk:
                raise RuntimeError("backend closed the connection")
            buf.extend(chunk)
            nl = buf.find(b"\n")
            if nl >= 0:
                return json.loads(bytes(buf[:nl]).decode("utf-8"))

    def close(self):
        if self.sock is not None:
            try:
                self.sock.sendall(b'{"cmd":"quit"}\n')
            except OSError:
                pass
            try:
                self.sock.close()
            finally:
                self.sock = None


class App:
    def __init__(self, root: tk.Tk, backend: Backend):
        self.root = root
        self.backend = backend
        root.title("Page Replacement Simulator")
        root.geometry("1000x650")

        # ---- input bar ----
        top = ttk.Frame(root, padding=(10, 10, 10, 4))
        top.pack(fill="x")

        ttk.Label(top, text="refs ; frames ; algo(1-4)").pack(anchor="w")
        row = ttk.Frame(top)
        row.pack(fill="x", pady=(2, 0))

        self.entry = ttk.Entry(row)
        self.entry.pack(side="left", fill="x", expand=True)
        self.entry.insert(0, "1 2 3 4 1 2 5 ; 3 ; 4")
        self.entry.bind("<Return>", lambda _e: self.run())

        ttk.Button(row, text="Run", command=self.run).pack(side="left", padx=(6, 0))
        ttk.Button(row, text="Quit", command=self.quit).pack(side="left", padx=(6, 0))

        # ---- status line ----
        self.status = tk.StringVar(value="ready")
        ttk.Label(root, textvariable=self.status, anchor="w",
                  padding=(10, 0, 10, 6)).pack(fill="x")

        # ---- algorithm selector for the trace view ----
        sel = ttk.Frame(root, padding=(10, 0, 10, 6))
        sel.pack(fill="x")
        ttk.Label(sel, text="Show trace for:").pack(side="left")
        self.algo_pick = ttk.Combobox(sel, state="readonly", width=14)
        self.algo_pick.pack(side="left", padx=(6, 0))
        self.algo_pick.bind("<<ComboboxSelected>>", lambda _e: self.render_trace())

        # ---- trace table (one row per step) ----
        self.tv = ttk.Treeview(root, show="headings")
        self.tv.pack(fill="both", expand=True, padx=10, pady=(0, 10))

        # ---- comparison table (summary) ----
        ttk.Label(root, text="Comparison").pack(anchor="w", padx=10)
        self.tv_cmp = ttk.Treeview(
            root, show="headings", height=4,
            columns=("algo", "hits", "faults", "rate", "best"),
        )
        for col, w in (("algo", 160), ("hits", 80), ("faults", 80),
                       ("rate", 100), ("best", 80)):
            self.tv_cmp.heading(col, text=col)
            self.tv_cmp.column(col, width=w, anchor="center")
        self.tv_cmp.pack(fill="x", padx=10, pady=(0, 10))

        self.last_response = None
        self._algo_names = {}   # display name -> result index

    # ---------- input handling ----------

    def parse_input(self, text: str):
        """'1 2 3 ; 3 ; 4' -> dict, or None on parse error."""
        try:
            refs_s, frames_s, algo_s = [part.strip() for part in text.split(";")]
            refs = [int(x) for x in refs_s.split()]
            frames = int(frames_s)
            algo = int(algo_s)
        except ValueError as e:
            messagebox.showerror("Parse error", f"invalid input: {e}")
            return None
        return {"cmd": "run", "refs": refs, "frames": frames, "algo": algo}

    def run(self):
        req = self.parse_input(self.entry.get())
        if req is None:
            return
        try:
            resp = self.backend.request(req)
        except Exception as e:
            self.status.set(f"error: {e}")
            messagebox.showerror("Backend error", str(e))
            return

        if not resp.get("ok"):
            status = resp.get("status", 0)
            err = resp.get("error", "unknown error")
            # status -3 is IPC_ERR_CORE: backend runs, simulation core missing.
            if status == -3:
                messagebox.showwarning("Core not linked", err)
            elif status == -2:
                messagebox.showerror("Invalid input", err)
            else:
                messagebox.showerror("Backend error", err)
            self.status.set(f"error (status {status}): {err}")
            return

        self.last_response = resp
        self.fill_algo_picker(resp["results"])
        self.render_trace()
        self.render_comparison(resp)
        n = resp.get("best", -1)
        best_name = (resp["results"][n]["name"] if 0 <= n < len(resp["results"])
                     else "(none)")
        self.status.set(
            f"ok — {len(resp['results'])} result(s), best={best_name}"
        )

    # ---------- rendering ----------

    def fill_algo_picker(self, results):
        self._algo_names = {r["name"]: i for i, r in enumerate(results)}
        self.algo_pick["values"] = list(self._algo_names)
        best = (self.last_response or {}).get("best", -1)
        if 0 <= best < len(results):
            self.algo_pick.set(results[best]["name"])
        else:
            self.algo_pick.set(results[0]["name"])

    def current_result(self):
        if not self.last_response:
            return None
        name = self.algo_pick.get()
        idx = self._algo_names.get(name)
        if idx is None:
            return None
        return self.last_response["results"][idx]

    def render_trace(self):
        r = self.current_result()
        self.tv.delete(*self.tv.get_children())
        if not r:
            return

        n_frames = r["frames"]
        cols = ["page"] + [f"F{i}" for i in range(n_frames)] + ["status", "victim"]
        self.tv["columns"] = cols
        for c in cols:
            w = 70 if c in ("page", "status", "victim") else 50
            self.tv.heading(c, text=c)
            self.tv.column(c, width=w, anchor="center")

        for step in r["steps"]:
            frames = ["." if v is None else v for v in step["frames"]]
            victim = "-" if step["victim"] < 0 else step["victim"]
            values = [step["page"]] + frames + [step["status"], victim]
            tag = "hit" if step["status"] == "HIT" else "fault"
            self.tv.insert("", "end", values=values, tags=(tag,))

        self.tv.tag_configure("hit",   background="#e8f7e8")
        self.tv.tag_configure("fault", background="#fdecec")

    def render_comparison(self, resp):
        self.tv_cmp.delete(*self.tv_cmp.get_children())
        best = resp.get("best", -1)
        for i, r in enumerate(resp["results"]):
            if r["faults"] < 0:
                hits = faults = rate = "N/A"
            else:
                total = r["total_refs"] or 1
                hits = str(r["hits"])
                faults = str(r["faults"])
                rate = f"{100.0 * r['hits'] / total:.1f}%"
            self.tv_cmp.insert("", "end", values=(
                r["name"], hits, faults, rate,
                "BEST" if i == best else "",
            ))

    # ---------- shutdown ----------

    def quit(self):
        try:
            self.backend.close()
        finally:
            self.root.destroy()


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else SOCK_DEFAULT
    backend = Backend(path)
    try:
        backend.connect()
    except Exception as e:
        sys.stderr.write(f"gui.py: {e}\n")
        sys.stderr.write("hint: start the backend first (make run)\n")
        return 1

    root = tk.Tk()
    App(root, backend)
    root.protocol("WM_DELETE_WINDOW", lambda: App_ref_quit(root))
    root.mainloop()
    return 0


# Small indirection so WM_DELETE_WINDOW has a stable handle.
def App_ref_quit(root):
    # Find the App instance bound to the root.
    for child in root.winfo_children():
        pass
    # Tkinter doesn't give us the instance, so we look it up via a tag.
    app = getattr(root, "_app_instance", None)
    if app:
        app.quit()
    else:
        root.destroy()


if __name__ == "__main__":
    sys.exit(main())
