"""NOCTIS: ojos + manos sobre PPSSPP via su WebSocket debugger (sin foco, sin tocar la pantalla).
Uso: python psp_eye.py [acciones...]
  boton:ms          -> presiona y suelta (cross circle square triangle up down left right start select l r)
  ax:X:ms / ay:Y:ms -> nub analogico (-1..1) mantenido ms
  wait:ms           -> espera sin tocar nada
  hold:boton / free:boton -> mantiene / suelta un boton (ej. hold:l ... shot ... free:l)
  sx:X / sy:Y       -> deja el nub inclinado hasta nuevo aviso (OJO: en el debugger de PPSSPP y+ es ARRIBA: sy:1 = adelante, sy:-1 = atras; sx:0 sy:0 suelta)
  shot[:nombre]     -> guarda el frame REAL del juego en build/<nombre>.png (gpu.buffer.screenshot)
Si no hay ningun shot, al final guarda build/eye.png.
Ej: python psp_eye.py shot:antes "ay:1.0:1500" shot:despues   (ay/sy positivo = stick ARRIBA)
A diferencia de PrintWindow (shot.ps1/play.ps1), lee el framebuffer del emulador: no se congela
y funciona con cualquier backend (GL/D3D/software).
"""
import os, sys, json, struct, time, base64, zlib
from psp_input import find_port, ws_connect, ws_send

BUILD = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "build")

def recvn(s, n):
    buf = b""
    while len(buf) < n:
        chunk = s.recv(n - len(buf))
        if not chunk: raise ConnectionError("PPSSPP cerro el WebSocket")
        buf += chunk
    return buf

def ws_recv(s):
    """Un mensaje completo (junta fragmentos; el PNG llega en un frame grande)."""
    msg = b""
    while True:
        b0, b1 = recvn(s, 2)
        n = b1 & 0x7F
        if n == 126: n = struct.unpack(">H", recvn(s, 2))[0]
        elif n == 127: n = struct.unpack(">Q", recvn(s, 8))[0]
        mask = recvn(s, 4) if b1 & 0x80 else None
        data = recvn(s, n)
        if mask: data = bytes(b ^ mask[i % 4] for i, b in enumerate(data))
        op = b0 & 0x0F
        if op == 8: raise ConnectionError("close del emulador")
        if op in (9, 10): continue          # ping/pong: el servidor no los exige
        msg += data
        if b0 & 0x80: return msg.decode(errors="ignore")

def wait_event(s, name, timeout=8.0):
    """Espera la respuesta a 'name' saltando las notificaciones que el emulador emite solo."""
    end = time.time() + timeout
    while time.time() < end:
        s.settimeout(max(0.1, end - time.time()))
        try: m = json.loads(ws_recv(s))
        except TimeoutError: break
        if m.get("event") == name: return m
        if m.get("event") == "error": raise SystemExit("PPSSPP error: " + m.get("message", str(m)))
    raise SystemExit("TIMEOUT esperando " + name)

# gpu.buffer.screenshot NO sirve en PPSSPP 1.20.4 + software renderer ("Could not download
# output", igual renderColor/renderDepth). En su lugar: breakpoint en el stub de
# sceDisplaySetFrameBuf (lo llama sceGuSwapBuffers con el frame YA terminado): a0 = direccion
# del buffer que se va a mostrar, a1 = ancho en pixeles de la fila, a2 = formato. Se lee esa
# VRAM con memory.read y se arma el PNG aca. Requiere SoftwareRenderer = True (con GPU real la
# VRAM no se actualiza).
SCR_W, SCR_H = 480, 272

def _lut16(fmt):
    """Tabla 16 bits -> RGB para 0=5650, 1=5551, 2=4444 (canales en orden R,G,B desde el bit 0)."""
    bits = {0: (5, 6, 5), 1: (5, 5, 5), 2: (4, 4, 4)}[fmt]
    lut = []
    for v in range(65536):
        px, sh = [], 0
        for n in bits:
            c = (v >> sh) & ((1 << n) - 1); sh += n
            px.append((c << (8 - n)) | (c >> (2 * n - 8) if n > 4 else c))
        lut.append(bytes(px))
    return lut

def _png(path, w, h, rows):
    raw = b"".join(b"\x00" + r for r in rows)
    chunk = lambda t, d: struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))

def _call(s, ev, **kw):
    ws_send(s, dict(event=ev, **kw)); return wait_event(s, ev)

def screenshot(s, name, scale=2):
    stub = [f["address"] for f in _call(s, "hle.func.list")["functions"]
            if f.get("name", "").endswith("sceDisplaySetFrameBuf")]
    if not stub: raise SystemExit("NO_STUB sceDisplaySetFrameBuf (el juego no arranco?)")
    _call(s, "cpu.breakpoint.add", address=stub[0], enabled=True)
    try:
        wait_event(s, "cpu.stepping", 4)       # para en el proximo swap (~16 ms a 60 FPS)
        a0, stride, fmt = (_call(s, "cpu.getReg", name=r)["uintValue"] for r in ("a0", "a1", "a2"))
        bpp = 4 if fmt == 3 else 2
        mem = _call(s, "memory.read", address=a0 | 0x04000000, size=stride * SCR_H * bpp)
    finally:   # aunque falle, no dejar el juego congelado
        _call(s, "cpu.breakpoint.remove", address=stub[0])
        ws_send(s, {"event": "cpu.resume"})
    raw = base64.b64decode(mem["base64"])
    lut = _lut16(fmt) if bpp == 2 else None
    rows = []
    for y in range(SCR_H):
        line = raw[y * stride * bpp:(y * stride + SCR_W) * bpp]
        px = ([lut[v] for v in struct.unpack("<%dH" % SCR_W, line)] if lut
              else [line[i:i + 3] for i in range(0, len(line), 4)])
        row = b"".join(p * scale for p in px)
        rows += [row] * scale
    path = os.path.normpath(os.path.join(BUILD, name + ".png"))
    _png(path, SCR_W * scale, SCR_H * scale, rows)
    return "build\\%s.png %dx%d fmt=%d" % (name, SCR_W * scale, SCR_H * scale, fmt)

ALIAS = {"l": "ltrigger", "r": "rtrigger"}   # nombres del debugger de PPSSPP para los gatillos

def main():
    port = find_port()
    s = ws_connect(port)
    out, shots = [], 0
    stick = [0.0, 0.0]
    for arg in sys.argv[1:]:
        parts = arg.split(":")
        k = parts[0].lower()
        if len(parts) > 1 and k in ("hold", "free"): parts[1] = ALIAS.get(parts[1].lower(), parts[1])
        k = ALIAS.get(k, k)
        if k == "shot":
            out.append(screenshot(s, parts[1] if len(parts) > 1 else "eye")); shots += 1
        elif k == "wait":
            time.sleep(int(parts[1]) / 1000.0)
        elif k in ("sx", "sy"):        # deja el nub INCLINADO (para sacar fotos en plena marcha); sx:0 sy:0 lo suelta
            stick[0 if k == "sx" else 1] = float(parts[1])
            ws_send(s, {"event": "input.analog.send", "stick": "left", "x": stick[0], "y": stick[1]})
            time.sleep(0.05)
        elif k in ("hold", "free"):   # mantener/soltar un boton entre otras acciones (L apuntando, SELECT...)
            ws_send(s, {"event": "input.buttons.send", "buttons": {parts[1].lower(): k == "hold"}})
            time.sleep(0.05)
        elif k in ("ax", "ay"):
            v = float(parts[1]); ms = int(parts[2]) if len(parts) > 2 else 500
            ws_send(s, {"event": "input.analog.send", "stick": "left", "x": v if k == "ax" else 0.0, "y": v if k == "ay" else 0.0})
            time.sleep(ms / 1000.0)
            ws_send(s, {"event": "input.analog.send", "stick": "left", "x": 0.0, "y": 0.0})
        else:
            ms = int(parts[1]) if len(parts) > 1 else 120
            ws_send(s, {"event": "input.buttons.press", "button": k, "duration": max(2, ms // 16)})
            time.sleep(ms / 1000.0 + 0.05)
    if shots == 0: out.append(screenshot(s, "eye"))
    print("EYE ok port=%d -> %s" % (port, " | ".join(out)))
    s.close()

if __name__ == "__main__": main()
