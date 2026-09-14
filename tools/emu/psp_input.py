"""NOCTIS: inyecta input a PPSSPP via su WebSocket debugger (sin foco, ventana fuera de pantalla).
Uso: python psp_input.py "start:120" "triangle:100" "up:1500" "ax:1.0:800"
  boton:ms       -> presiona y suelta (cross circle square triangle up down left right start select l r)
  ax:X:ms / ay:Y:ms -> nub analogico (-1..1) mantenido ms
Puerto: se lee de PPSSPP_PORT o se autodetecta el LISTENING del proceso.
"""
import socket, os, sys, json, struct, time, base64, subprocess, re

def find_port():
    p = os.environ.get("PPSSPP_PORT")
    if p: return int(p)
    out = subprocess.run(["netstat","-ano"], capture_output=True, text=True).stdout
    pid = subprocess.run(["powershell","-Command","(Get-Process PPSSPPWindows64).Id"], capture_output=True, text=True).stdout.strip()
    for line in out.splitlines():
        if "LISTENING" in line and line.strip().endswith(pid):
            m = re.search(r":(\d+)\s", line)
            if m: return int(m.group(1))
    raise SystemExit("NO_PORT (PPSSPP con RemoteDebuggerOnStartup?)")

def ws_connect(port):
    s = socket.create_connection(("127.0.0.1", port), timeout=5)
    key = base64.b64encode(os.urandom(16)).decode()
    s.sendall((f"GET /debugger HTTP/1.1\r\nHost: 127.0.0.1:{port}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
               f"Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n").encode())
    resp = s.recv(4096).decode(errors="ignore")
    if "101" not in resp.split("\r\n")[0]: raise SystemExit("WS_HANDSHAKE_FAIL: " + resp[:120])
    return s

def ws_send(s, obj):
    data = json.dumps(obj).encode()
    mask = os.urandom(4)
    hdr = bytes([0x81])
    n = len(data)
    if n < 126: hdr += bytes([0x80 | n])
    elif n < 65536: hdr += bytes([0x80 | 126]) + struct.pack(">H", n)
    else: hdr += bytes([0x80 | 127]) + struct.pack(">Q", n)
    s.sendall(hdr + mask + bytes(b ^ mask[i % 4] for i, b in enumerate(data)))

def ws_drain(s, t=0.15):
    s.settimeout(t); out = b""
    try:
        while True:
            chunk = s.recv(65536)
            if not chunk: break
            out += chunk
    except Exception: pass
    return out

def main():
    port = find_port()
    s = ws_connect(port)
    ws_drain(s)
    log = []
    for arg in sys.argv[1:]:
        parts = arg.split(":")
        k = parts[0].lower()
        if k in ("ax", "ay"):
            v = float(parts[1]); ms = int(parts[2]) if len(parts) > 2 else 500
            ws_send(s, {"event": "input.analog.send", "stick": "left", "x": v if k == "ax" else 0.0, "y": v if k == "ay" else 0.0})
            time.sleep(ms / 1000.0)
            ws_send(s, {"event": "input.analog.send", "stick": "left", "x": 0.0, "y": 0.0})
        else:
            ms = int(parts[1]) if len(parts) > 1 else 120
            # 'press' con duration en FRAMES garantiza que el juego vea el boton (send+release
            # rapido puede caer entre dos muestreos y perderse)
            ws_send(s, {"event": "input.buttons.press", "button": k, "duration": max(2, ms // 16)})
            time.sleep(ms / 1000.0 + 0.05)
        time.sleep(0.1)
        log.append(arg)
    r = ws_drain(s, 0.3)
    # muestra el ultimo mensaje del emulador (errores del protocolo salen aqui)
    txt = re.findall(rb'\{.*?\}', r)
    print("INPUT ok port=%d sent=%s resp=%s" % (port, ",".join(log), (txt[-1][:160].decode(errors='ignore') if txt else "-")))
    s.close()

if __name__ == "__main__": main()
