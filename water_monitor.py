"""
Water Level Monitor - Aplicatie Python PC
Citeste date de la Arduino Nano via Serial si afiseaza grafic live.
Necesita: pip install pyserial matplotlib
"""

import serial
import serial.tools.list_ports
import json
import threading
import time
import csv
import os
from datetime import datetime
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.animation import FuncAnimation
import collections

# ==================== CONFIGURARE ====================
BAUD_RATE   = 9600
MAX_POINTS  = 120       # 60 secunde la 500ms interval
LOG_FILE    = "water_level_log.csv"

# ==================== DATE GLOBALE ====================
times       = collections.deque(maxlen=MAX_POINTS)
levels      = collections.deque(maxlen=MAX_POINTS)
pump_states = collections.deque(maxlen=MAX_POINTS)

latest_data = {
    "level": 0.0,
    "pump": False,
    "mode": "AUTO",
    "sp_low": 25,
    "sp_high": 80,
    "raw": 0,
    "alert": "OK"
}

serial_port   = None
serial_thread = None
running       = False
log_writer    = None
log_file_obj  = None

# ==================== SERIAL ====================
def find_arduino():
    """Cauta automat portul Arduino."""
    ports = serial.tools.list_ports.comports()
    for p in ports:
        if any(kw in p.description for kw in ["Arduino", "CH340", "USB Serial", "COM"]):
            return p.device
    if ports:
        return ports[0].device
    return None

def serial_reader(port, baud, log_cb, status_cb):
    global serial_port, running, latest_data, log_writer

    try:
        serial_port = serial.Serial(port, baud, timeout=1)
        status_cb(f"Conectat pe {port} @ {baud} baud", "green")
        time.sleep(2)  # Asteapta Arduino reset

        while running:
            try:
                line = serial_port.readline().decode("utf-8", errors="ignore").strip()
                if not line:
                    continue
                if line.startswith("{"):
                    data = json.loads(line)
                    latest_data.update(data)
                    ts = datetime.now()
                    times.append(ts)
                    levels.append(data.get("level", 0))
                    pump_states.append(1 if data.get("pump", False) else 0)

                    # Log CSV
                    if log_writer:
                        log_writer.writerow([
                            ts.strftime("%Y-%m-%d %H:%M:%S"),
                            data.get("level", 0),
                            1 if data.get("pump", False) else 0,
                            data.get("mode", ""),
                            data.get("sp_low", 0),
                            data.get("sp_high", 0),
                            data.get("alert", "")
                        ])
                        log_file_obj.flush()

                    log_cb(f"[{ts.strftime('%H:%M:%S')}] Nivel: {data.get('level',0):.1f}% | Pompa: {'ON' if data.get('pump') else 'OFF'} | {data.get('alert','')}")

            except json.JSONDecodeError:
                pass
            except Exception as e:
                log_cb(f"[ERR] {e}")

    except serial.SerialException as e:
        status_cb(f"Eroare serial: {e}", "red")
    finally:
        if serial_port and serial_port.is_open:
            serial_port.close()
        status_cb("Deconectat", "red")

def send_command(cmd):
    if serial_port and serial_port.is_open:
        serial_port.write((cmd + "\n").encode())

# ==================== GUI ====================
class WaterMonitorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Water Level Monitor")
        self.root.configure(bg="#1a1a2e")
        self.root.geometry("900x650")
        self.root.resizable(True, True)

        self._build_ui()
        self._init_log()
        self._start_animation()

    # ---------- LOG CSV ----------
    def _init_log(self):
        global log_writer, log_file_obj
        new_file = not os.path.exists(LOG_FILE)
        log_file_obj = open(LOG_FILE, "a", newline="")
        log_writer = csv.writer(log_file_obj)
        if new_file:
            log_writer.writerow(["timestamp", "level_%", "pump", "mode", "sp_low", "sp_high", "alert"])

    # ---------- UI ----------
    def _build_ui(self):
        root = self.root
        BG      = "#1a1a2e"
        PANEL   = "#16213e"
        ACCENT  = "#0f3460"
        TEXT    = "#e0e0e0"
        GREEN   = "#00b894"
        RED     = "#d63031"
        YELLOW  = "#fdcb6e"
        BLUE    = "#74b9ff"

        # === TOP BAR ===
        top = tk.Frame(root, bg=ACCENT, pady=6)
        top.pack(fill=tk.X)
        tk.Label(top, text="💧 WATER LEVEL MONITOR", bg=ACCENT, fg=TEXT,
                 font=("Consolas", 14, "bold")).pack(side=tk.LEFT, padx=12)

        self.status_label = tk.Label(top, text="⬤ Deconectat", bg=ACCENT, fg=RED,
                                     font=("Consolas", 10))
        self.status_label.pack(side=tk.RIGHT, padx=12)

        # === MAIN FRAME ===
        main = tk.Frame(root, bg=BG)
        main.pack(fill=tk.BOTH, expand=True, padx=8, pady=6)

        # --- LEFT: Metrics + Control ---
        left = tk.Frame(main, bg=BG, width=220)
        left.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 6))
        left.pack_propagate(False)

        # Level gauge
        gauge_frame = tk.LabelFrame(left, text="NIVEL", bg=PANEL, fg=TEXT,
                                    font=("Consolas", 9, "bold"), bd=1, relief=tk.GROOVE)
        gauge_frame.pack(fill=tk.X, pady=(0, 4))

        self.level_var = tk.StringVar(value="0.0%")
        tk.Label(gauge_frame, textvariable=self.level_var, bg=PANEL, fg=GREEN,
                 font=("Consolas", 28, "bold")).pack(pady=4)

        self.alert_var = tk.StringVar(value="OK")
        self.alert_label = tk.Label(gauge_frame, textvariable=self.alert_var,
                                    bg=PANEL, fg=GREEN, font=("Consolas", 10, "bold"))
        self.alert_label.pack(pady=(0, 6))

        # Pump status
        pump_frame = tk.LabelFrame(left, text="POMPA", bg=PANEL, fg=TEXT,
                                   font=("Consolas", 9, "bold"), bd=1, relief=tk.GROOVE)
        pump_frame.pack(fill=tk.X, pady=4)

        self.pump_var = tk.StringVar(value="OFF")
        self.pump_label = tk.Label(pump_frame, textvariable=self.pump_var,
                                   bg=PANEL, fg=RED, font=("Consolas", 20, "bold"))
        self.pump_label.pack(pady=6)

        self.mode_var = tk.StringVar(value="AUTO")
        tk.Label(pump_frame, textvariable=self.mode_var, bg=PANEL, fg=BLUE,
                 font=("Consolas", 10)).pack(pady=(0, 6))

        # Control buttons
        ctrl_frame = tk.LabelFrame(left, text="CONTROL", bg=PANEL, fg=TEXT,
                                   font=("Consolas", 9, "bold"), bd=1, relief=tk.GROOVE)
        ctrl_frame.pack(fill=tk.X, pady=4)

        btn_cfg = {"bg": ACCENT, "fg": TEXT, "font": ("Consolas", 9, "bold"),
                   "relief": tk.FLAT, "bd": 0, "pady": 6, "cursor": "hand2"}

        tk.Button(ctrl_frame, text="▶ PORNESTE POMPA",
                  command=lambda: send_command("PUMP_ON"), **btn_cfg).pack(fill=tk.X, padx=4, pady=2)
        tk.Button(ctrl_frame, text="■ OPRESTE POMPA",
                  command=lambda: send_command("PUMP_OFF"), **btn_cfg).pack(fill=tk.X, padx=4, pady=2)
        tk.Button(ctrl_frame, text="⟳ MOD AUTO",
                  command=lambda: send_command("MODE_AUTO"), **btn_cfg).pack(fill=tk.X, padx=4, pady=2)
        tk.Button(ctrl_frame, text="✋ MOD MANUAL",
                  command=lambda: send_command("MODE_MANUAL"), **btn_cfg).pack(fill=tk.X, padx=4, pady=2)

        # Setpoint control
        sp_frame = tk.LabelFrame(left, text="SETPOINT", bg=PANEL, fg=TEXT,
                                 font=("Consolas", 9, "bold"), bd=1, relief=tk.GROOVE)
        sp_frame.pack(fill=tk.X, pady=4)

        row1 = tk.Frame(sp_frame, bg=PANEL)
        row1.pack(fill=tk.X, padx=4, pady=2)
        tk.Label(row1, text="LOW %:", bg=PANEL, fg=TEXT, font=("Consolas", 9)).pack(side=tk.LEFT)
        self.sp_low_var = tk.IntVar(value=25)
        sp_low_spin = tk.Spinbox(row1, from_=5, to=90, textvariable=self.sp_low_var,
                                 width=5, bg=ACCENT, fg=TEXT, font=("Consolas", 9))
        sp_low_spin.pack(side=tk.RIGHT)

        row2 = tk.Frame(sp_frame, bg=PANEL)
        row2.pack(fill=tk.X, padx=4, pady=2)
        tk.Label(row2, text="HIGH %:", bg=PANEL, fg=TEXT, font=("Consolas", 9)).pack(side=tk.LEFT)
        self.sp_high_var = tk.IntVar(value=80)
        sp_high_spin = tk.Spinbox(row2, from_=10, to=95, textvariable=self.sp_high_var,
                                  width=5, bg=ACCENT, fg=TEXT, font=("Consolas", 9))
        sp_high_spin.pack(side=tk.RIGHT)

        tk.Button(sp_frame, text="APLICA SETPOINT",
                  command=self._apply_setpoint, **btn_cfg).pack(fill=tk.X, padx=4, pady=4)

        # Connect
        conn_frame = tk.LabelFrame(left, text="CONEXIUNE", bg=PANEL, fg=TEXT,
                                   font=("Consolas", 9, "bold"), bd=1, relief=tk.GROOVE)
        conn_frame.pack(fill=tk.X, pady=4)

        port_row = tk.Frame(conn_frame, bg=PANEL)
        port_row.pack(fill=tk.X, padx=4, pady=2)
        tk.Label(port_row, text="PORT:", bg=PANEL, fg=TEXT, font=("Consolas", 9)).pack(side=tk.LEFT)
        self.port_var = tk.StringVar(value=find_arduino() or "COM3")
        tk.Entry(port_row, textvariable=self.port_var, bg=ACCENT, fg=TEXT,
                 font=("Consolas", 9), width=8).pack(side=tk.RIGHT)

        self.conn_btn = tk.Button(conn_frame, text="CONECTEAZA",
                                  command=self._toggle_connect, **btn_cfg)
        self.conn_btn.pack(fill=tk.X, padx=4, pady=4)

        # --- RIGHT: Chart + Log ---
        right = tk.Frame(main, bg=BG)
        right.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        # Chart
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(6, 4),
                                                        gridspec_kw={"height_ratios": [3, 1]})
        self.fig.patch.set_facecolor("#1a1a2e")
        for ax in (self.ax1, self.ax2):
            ax.set_facecolor("#16213e")
            ax.tick_params(colors="#e0e0e0", labelsize=7)
            for spine in ax.spines.values():
                spine.set_edgecolor("#0f3460")

        self.ax1.set_ylabel("Nivel (%)", color="#e0e0e0", fontsize=8)
        self.ax1.set_ylim(0, 105)
        self.ax2.set_ylabel("Pompă", color="#e0e0e0", fontsize=8)
        self.ax2.set_ylim(-0.1, 1.1)
        self.ax2.set_yticks([0, 1])
        self.ax2.set_yticklabels(["OFF", "ON"], color="#e0e0e0", fontsize=7)
        self.fig.tight_layout(pad=1.5)

        canvas = FigureCanvasTkAgg(self.fig, master=right)
        canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        self.canvas = canvas

        # Log text
        log_frame = tk.LabelFrame(right, text="LOG SERIAL", bg=PANEL, fg=TEXT,
                                  font=("Consolas", 9, "bold"), bd=1, relief=tk.GROOVE, height=100)
        log_frame.pack(fill=tk.X, pady=(4, 0))
        log_frame.pack_propagate(False)

        self.log_text = tk.Text(log_frame, bg="#0d0d1a", fg="#00ff88", font=("Consolas", 8),
                                state=tk.DISABLED, wrap=tk.WORD)
        self.log_text.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)

        # Color refs
        self._GREEN  = GREEN
        self._RED    = RED
        self._YELLOW = YELLOW
        self._BLUE   = BLUE

    # ---------- ANIMATION ----------
    def _start_animation(self):
        self.anim = FuncAnimation(self.fig, self._update_chart,
                                  interval=500, blit=False, cache_frame_data=False)

    def _update_chart(self, frame):
        if not times:
            return

        self.ax1.cla()
        self.ax2.cla()

        for ax in (self.ax1, self.ax2):
            ax.set_facecolor("#16213e")
            ax.tick_params(colors="#e0e0e0", labelsize=7)
            for spine in ax.spines.values():
                spine.set_edgecolor("#0f3460")

        t_list = list(times)
        l_list = list(levels)
        p_list = list(pump_states)

        # Nivel line
        self.ax1.plot(t_list, l_list, color="#00b894", linewidth=1.5, label="Nivel")
        self.ax1.fill_between(t_list, l_list, alpha=0.15, color="#00b894")

        # Setpoint lines
        sp_low  = latest_data["sp_low"]
        sp_high = latest_data["sp_high"]
        self.ax1.axhline(sp_low,  color="#fdcb6e", linestyle="--", linewidth=1, alpha=0.7, label=f"SP Low {sp_low}%")
        self.ax1.axhline(sp_high, color="#e17055", linestyle="--", linewidth=1, alpha=0.7, label=f"SP High {sp_high}%")

        self.ax1.set_ylabel("Nivel (%)", color="#e0e0e0", fontsize=8)
        self.ax1.set_ylim(0, 105)
        self.ax1.legend(loc="upper right", fontsize=7, facecolor="#1a1a2e",
                        edgecolor="#0f3460", labelcolor="#e0e0e0")

        # Pompă
        self.ax2.step(t_list, p_list, color="#74b9ff", linewidth=1.5, where="post")
        self.ax2.fill_between(t_list, p_list, alpha=0.2, color="#74b9ff", step="post")
        self.ax2.set_ylabel("Pompă", color="#e0e0e0", fontsize=8)
        self.ax2.set_ylim(-0.1, 1.1)
        self.ax2.set_yticks([0, 1])
        self.ax2.set_yticklabels(["OFF", "ON"], color="#e0e0e0", fontsize=7)

        self.fig.tight_layout(pad=1.5)

        # Update labels
        d = latest_data
        self.level_var.set(f"{d['level']:.1f}%")
        self.pump_var.set("ON" if d["pump"] else "OFF")
        self.pump_label.config(fg=self._GREEN if d["pump"] else self._RED)
        self.mode_var.set(f"Mod: {d['mode']}")
        self.sp_low_var.set(d["sp_low"])
        self.sp_high_var.set(d["sp_high"])

        alert = d["alert"]
        self.alert_var.set(alert)
        if alert == "OK":
            self.alert_label.config(fg=self._GREEN)
        else:
            self.alert_label.config(fg=self._RED)

    # ---------- LOG ----------
    def _log(self, msg):
        self.root.after(0, self._append_log, msg)

    def _append_log(self, msg):
        self.log_text.config(state=tk.NORMAL)
        self.log_text.insert(tk.END, msg + "\n")
        self.log_text.see(tk.END)
        # Pastreaza max 200 linii
        lines = int(self.log_text.index("end-1c").split(".")[0])
        if lines > 200:
            self.log_text.delete("1.0", f"{lines-200}.0")
        self.log_text.config(state=tk.DISABLED)

    # ---------- STATUS ----------
    def _set_status(self, msg, color):
        self.root.after(0, lambda: self.status_label.config(
            text=f"⬤ {msg}", fg=color))

    # ---------- CONNECT ----------
    def _toggle_connect(self):
        global running, serial_thread
        if not running:
            running = True
            port = self.port_var.get()
            self.conn_btn.config(text="DECONECTEAZA")
            serial_thread = threading.Thread(
                target=serial_reader,
                args=(port, BAUD_RATE, self._log, self._set_status),
                daemon=True
            )
            serial_thread.start()
        else:
            running = False
            self.conn_btn.config(text="CONECTEAZA")
            if serial_port and serial_port.is_open:
                serial_port.close()

    # ---------- SETPOINT ----------
    def _apply_setpoint(self):
        low  = self.sp_low_var.get()
        high = self.sp_high_var.get()
        if low >= high:
            messagebox.showwarning("Setpoint invalid", "SP Low trebuie să fie < SP High!")
            return
        send_command(f"SP_LOW:{low}")
        time.sleep(0.05)
        send_command(f"SP_HIGH:{high}")
        self._log(f"[CMD] Setpoint aplicat: LOW={low}% HIGH={high}%")

    def on_close(self):
        global running
        running = False
        if log_file_obj:
            log_file_obj.close()
        self.root.destroy()


# ==================== MAIN ====================
if __name__ == "__main__":
    root = tk.Tk()
    app = WaterMonitorApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()
