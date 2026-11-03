#!/usr/bin/env python3
"""Tkinter GUI for the Week 4 Banker's Algorithm deadlock lab.

The GUI uses the tested ``deadlock_lab`` executable as its backend, so the
visible safety result and sequence are always produced by the C implementation.

Launch from this directory after building the project:
    make build
    python3 gui.py

An alternate executable path can be supplied as the only positional argument.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import subprocess
import sys
import tkinter as tk
from tkinter import messagebox, ttk

PROCESS_LIMITS = (1, 8)
RESOURCE_LIMITS = (1, 5)
SCENARIOS = ("safe", "deadlock", "mixed")


@dataclass(frozen=True)
class AnalysisResult:
    scenario: str
    available: tuple[int, ...]
    safe: bool
    completed: int
    processes: int
    sequence: tuple[str, ...]


class Backend:
    """Small adapter that invokes the Week 4 C executable once per request."""

    def __init__(self, executable: Path):
        self.executable = executable

    def run(self, scenario: str, processes: int, resources: int) -> AnalysisResult:
        if scenario not in SCENARIOS:
            raise ValueError(f"unknown scenario: {scenario}")
        if not PROCESS_LIMITS[0] <= processes <= PROCESS_LIMITS[1]:
            raise ValueError("processes must be between 1 and 8")
        if not RESOURCE_LIMITS[0] <= resources <= RESOURCE_LIMITS[1]:
            raise ValueError("resource types must be between 1 and 5")
        if not self.executable.is_file():
            raise RuntimeError(
                f"backend not found at {self.executable}. Run 'make build' first."
            )

        try:
            completed = subprocess.run(
                [str(self.executable), scenario, str(processes), str(resources)],
                text=True,
                capture_output=True,
                timeout=5,
                check=False,
            )
        except OSError as exc:
            raise RuntimeError(f"could not start backend: {exc}") from exc
        except subprocess.TimeoutExpired as exc:
            raise RuntimeError("backend timed out") from exc

        # The CLI uses exit status 3 to report a correctly analyzed unsafe state.
        if completed.returncode not in (0, 3):
            detail = completed.stderr.strip() or completed.stdout.strip() or "no output"
            raise RuntimeError(f"backend failed: {detail}")
        return parse_output(completed.stdout, expected_processes=processes)


def parse_output(output: str, expected_processes: int) -> AnalysisResult:
    """Parse the stable presentation output produced by ``deadlock_lab``."""
    fields: dict[str, str] = {}
    for line in output.splitlines():
        key, separator, value = line.partition(":")
        if separator:
            fields[key.strip()] = value.strip()

    required = {"Scenario", "Available", "State", "Completed", "Sequence"}
    missing = required.difference(fields)
    if missing:
        raise RuntimeError(f"backend returned incomplete output: missing {', '.join(sorted(missing))}")

    try:
        available = tuple(int(value) for value in fields["Available"].split())
        completed_text, total_text = fields["Completed"].split("/", maxsplit=1)
        completed = int(completed_text)
        total = int(total_text)
    except ValueError as exc:
        raise RuntimeError("backend returned malformed numeric output") from exc

    if not available or total != expected_processes or not 0 <= completed <= total:
        raise RuntimeError("backend returned inconsistent analysis output")

    state = fields["State"]
    if state not in {"SAFE", "UNSAFE"}:
        raise RuntimeError(f"backend returned an unknown state: {state}")

    sequence = tuple(re.findall(r"P\d+", fields["Sequence"]))
    if len(sequence) != completed:
        raise RuntimeError("backend returned an inconsistent sequence")

    return AnalysisResult(
        scenario=fields["Scenario"],
        available=available,
        safe=state == "SAFE",
        completed=completed,
        processes=total,
        sequence=sequence,
    )


class App:
    def __init__(self, root: tk.Tk, backend: Backend):
        self.root = root
        self.backend = backend
        root.title("Banker's Algorithm Deadlock Lab")
        root.minsize(700, 480)
        root.geometry("820x540")
        root.columnconfigure(0, weight=1)
        root.rowconfigure(2, weight=1)

        controls = ttk.LabelFrame(root, text="Simulation setup", padding=12)
        controls.grid(row=0, column=0, sticky="ew", padx=12, pady=(12, 6))
        for column in range(6):
            controls.columnconfigure(column, weight=1 if column in (1, 3, 5) else 0)

        self.processes = tk.StringVar(value="4")
        self.resources = tk.StringVar(value="3")
        self.scenario = tk.StringVar(value="safe")
        self.status = tk.StringVar(value="Ready. Choose inputs and run the simulation.")

        ttk.Label(controls, text="Processes (1-8)").grid(row=0, column=0, sticky="w")
        process_input = ttk.Spinbox(
            controls, from_=PROCESS_LIMITS[0], to=PROCESS_LIMITS[1],
            textvariable=self.processes, width=8,
        )
        process_input.grid(row=0, column=1, sticky="ew", padx=(6, 14))

        ttk.Label(controls, text="Resource types (1-5)").grid(row=0, column=2, sticky="w")
        resource_input = ttk.Spinbox(
            controls, from_=RESOURCE_LIMITS[0], to=RESOURCE_LIMITS[1],
            textvariable=self.resources, width=8,
        )
        resource_input.grid(row=0, column=3, sticky="ew", padx=(6, 14))

        ttk.Label(controls, text="Scenario").grid(row=0, column=4, sticky="w")
        scenario_input = ttk.Combobox(
            controls, state="readonly", values=SCENARIOS,
            textvariable=self.scenario, width=12,
        )
        scenario_input.grid(row=0, column=5, sticky="ew", padx=(6, 0))

        buttons = ttk.Frame(controls)
        buttons.grid(row=1, column=0, columnspan=6, sticky="ew", pady=(12, 0))
        ttk.Button(buttons, text="Run simulation", command=self.run).pack(side="left")
        ttk.Button(buttons, text="Reset", command=self.reset).pack(side="left", padx=(8, 0))
        ttk.Button(buttons, text="Quit", command=self.quit).pack(side="right")

        ttk.Label(root, textvariable=self.status, anchor="w", padding=(12, 4)).grid(
            row=1, column=0, sticky="ew"
        )

        results = ttk.Frame(root, padding=(12, 0, 12, 12))
        results.grid(row=2, column=0, sticky="nsew")
        results.columnconfigure(0, weight=1)
        results.rowconfigure(1, weight=1)

        self.summary = ttk.LabelFrame(results, text="Analysis result", padding=12)
        self.summary.grid(row=0, column=0, sticky="ew", pady=(0, 8))
        self.summary.columnconfigure(1, weight=1)
        self.state_value = ttk.Label(self.summary, text="Not run")
        self.available_value = ttk.Label(self.summary, text="-")
        self.completed_value = ttk.Label(self.summary, text="-")
        self.sequence_value = ttk.Label(self.summary, text="-", wraplength=620)
        for row, (label, value) in enumerate((
            ("System state", self.state_value),
            ("Available", self.available_value),
            ("Completed", self.completed_value),
            ("Safe sequence", self.sequence_value),
        )):
            ttk.Label(self.summary, text=f"{label}:").grid(row=row, column=0, sticky="nw", padx=(0, 12), pady=2)
            value.grid(row=row, column=1, sticky="w", pady=2)

        trace = ttk.LabelFrame(results, text="Execution trace", padding=8)
        trace.grid(row=1, column=0, sticky="nsew")
        trace.columnconfigure(0, weight=1)
        trace.rowconfigure(0, weight=1)
        self.trace = tk.Text(trace, height=10, wrap="word", state="disabled")
        self.trace.grid(row=0, column=0, sticky="nsew")

        root.bind("<Return>", self.run)
        root.protocol("WM_DELETE_WINDOW", self.quit)
        process_input.focus_set()

    def read_inputs(self) -> tuple[str, int, int] | None:
        try:
            processes = int(self.processes.get())
            resources = int(self.resources.get())
        except ValueError:
            messagebox.showerror("Invalid input", "Processes and resource types must be whole numbers.")
            return None
        if not PROCESS_LIMITS[0] <= processes <= PROCESS_LIMITS[1]:
            messagebox.showerror("Invalid input", "Processes must be between 1 and 8.")
            return None
        if not RESOURCE_LIMITS[0] <= resources <= RESOURCE_LIMITS[1]:
            messagebox.showerror("Invalid input", "Resource types must be between 1 and 5.")
            return None
        return self.scenario.get(), processes, resources

    def run(self, _event: object | None = None) -> None:
        values = self.read_inputs()
        if values is None:
            return
        scenario, processes, resources = values
        try:
            result = self.backend.run(scenario, processes, resources)
        except (RuntimeError, ValueError) as exc:
            self.status.set(f"Error: {exc}")
            messagebox.showerror("Simulation error", str(exc))
            return
        self.render(result)

    def render(self, result: AnalysisResult) -> None:
        state = "SAFE" if result.safe else "UNSAFE"
        sequence = " -> ".join(result.sequence) if result.sequence else "No safe sequence exists."
        self.state_value.configure(text=state)
        self.available_value.configure(text=" ".join(map(str, result.available)))
        self.completed_value.configure(text=f"{result.completed}/{result.processes}")
        self.sequence_value.configure(text=sequence)
        self.status.set(f"{result.scenario.title()} scenario analyzed: {state}.")

        if result.safe:
            text = "A safe completion order was found.\n\n" + sequence
        else:
            unfinished = result.processes - result.completed
            text = (
                "No safe completion order exists for this scenario.\n\n"
                f"Completed before the safety check stalled: {result.completed}/{result.processes}.\n"
                f"Processes still blocked: {unfinished}."
            )
        self.trace.configure(state="normal")
        self.trace.delete("1.0", tk.END)
        self.trace.insert("1.0", text)
        self.trace.configure(state="disabled")

    def reset(self) -> None:
        self.processes.set("4")
        self.resources.set("3")
        self.scenario.set("safe")
        self.state_value.configure(text="Not run")
        self.available_value.configure(text="-")
        self.completed_value.configure(text="-")
        self.sequence_value.configure(text="-")
        self.trace.configure(state="normal")
        self.trace.delete("1.0", tk.END)
        self.trace.configure(state="disabled")
        self.status.set("Ready. Choose inputs and run the simulation.")

    def quit(self) -> None:
        self.root.destroy()


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    if len(args) > 1:
        sys.stderr.write(f"Usage: {Path(sys.argv[0]).name} [path-to-deadlock_lab]\n")
        return 2
    default_backend = Path(__file__).resolve().parent / "build" / "deadlock_lab"
    backend = Backend(Path(args[0]).expanduser() if args else default_backend)
    root = tk.Tk()
    App(root, backend)
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
