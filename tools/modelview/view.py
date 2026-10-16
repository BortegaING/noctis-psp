"""NOCTIS -- visor de modelos FUERA del emulador (para disenar personaje y escenarios).

Compila tools/modelview/dump.cpp con el g++ de WSL, arma el modelo con el MISMO codigo del juego
(src/cand/hunter.h, src/rig.h, src/anim.h, src/cementerio_models.h) y lo dibuja con PIL desde
cualquier angulo (pintor: triangulos ordenados por profundidad, color promedio de sus vertices).

  python tools/modelview/view.py hero              -> build/mv/hero.png  (frente, lado, espalda, 3/4)
  python tools/modelview/view.py run               -> build/mv/run.png   (8 fases corriendo, de lado y de atras)
  python tools/modelview/view.py walk              -> build/mv/walk.png
  python tools/modelview/view.py model angel       -> build/mv/model_angel.png
  python tools/modelview/view.py scene             -> build/mv/scene.png (vista aerea + a ras del suelo, con niebla)
"""
import os, sys, struct, subprocess, math
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(ROOT, "build", "mv")
EXE = os.path.join(HERE, "dump")

def wsl_path(p):
    p = os.path.abspath(p).replace("\\", "/")
    return "/mnt/" + p[0].lower() + p[2:]

def build():
    os.makedirs(OUT, exist_ok=True)
    cmd = "g++ -O1 -std=c++17 -w -o %s %s" % (wsl_path(EXE), wsl_path(os.path.join(HERE, "dump.cpp")))
    r = subprocess.run(["wsl", "-d", "Ubuntu-24.04", "--", "bash", "-c", cmd], capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout, r.stderr); sys.exit("no compila")

def dump(args, name):
    path = os.path.join(OUT, name + ".bin")
    r = subprocess.run(["wsl", "-d", "Ubuntu-24.04", "--", wsl_path(EXE)] + [str(a) for a in args[:1]] + [wsl_path(path)] + [str(a) for a in args[1:]],
                       capture_output=True, text=True)
    if r.returncode != 0: print(r.stdout, r.stderr); sys.exit("fallo dump")
    sys.stderr.write(r.stderr)
    data = open(path, "rb").read()
    n = len(data) // 16
    v = struct.unpack("<" + "Ifff" * n, data)
    return [(v[k * 4], v[k * 4 + 1], v[k * 4 + 2], v[k * 4 + 3]) for k in range(n)]

def look(eye, tgt):
    f = [tgt[i] - eye[i] for i in range(3)]; l = math.sqrt(sum(x * x for x in f)); f = [x / l for x in f]
    up = (0.0, 1.0, 0.0)
    r = [f[1] * up[2] - f[2] * up[1], f[2] * up[0] - f[0] * up[2], f[0] * up[1] - f[1] * up[0]]
    l = math.sqrt(sum(x * x for x in r)) or 1.0; r = [x / l for x in r]
    u = [r[1] * f[2] - r[2] * f[1], r[2] * f[0] - r[0] * f[2], r[0] * f[1] - r[1] * f[0]]
    return f, r, u

def ground(size=6.0, y=0.0, col=(58, 62, 58)):
    """Un piso de damero para ver si los pies tocan el suelo."""
    out = []
    n = 6; s = size / n
    for a in range(-n, n):
        for b in range(-n, n):
            c = col if (a + b) & 1 else tuple(int(x * 0.85) for x in col)
            cc = c[0] | (c[1] << 8) | (c[2] << 16) | (255 << 24)
            x0, z0, x1, z1 = a * s, b * s, (a + 1) * s, (b + 1) * s
            out += [(cc, x0, y, z0), (cc, x1, y, z0), (cc, x1, y, z1), (cc, x0, y, z0), (cc, x1, y, z1), (cc, x0, y, z1)]
    return out

def render(verts, eye, tgt, W=320, H=360, fov=40.0, fog=None, bg=((70, 74, 84), (30, 32, 38))):
    """Rasterizador con z-buffer y color interpolado por vertice (como el GE con GU_SMOOTH)."""
    import numpy as np
    f, r, u = look(eye, tgt)
    k = (H * 0.5) / math.tan(math.radians(fov) * 0.5)
    V = np.array([(x, y, z) for (c, x, y, z) in verts], dtype=np.float64).reshape(-1, 3)
    C = np.array([(c & 255, (c >> 8) & 255, (c >> 16) & 255) for (c, x, y, z) in verts], dtype=np.float64).reshape(-1, 3)
    D = V - np.array(eye)
    zv = D @ np.array(f); xv = D @ np.array(r); yv = D @ np.array(u)
    sx = W * 0.5 + k * xv / np.maximum(zv, 1e-3); sy = H * 0.5 - k * yv / np.maximum(zv, 1e-3)
    if fog:
        n0, f0, fc = fog
        a = np.clip((zv - n0) / (f0 - n0), 0.0, 1.0)[:, None]
        C = C + (np.array(fc, dtype=np.float64) - C) * a
    img = np.zeros((H, W, 3), dtype=np.float64)
    t = np.linspace(0, 1, H)[:, None, None]
    img[:] = np.array(bg[0]) + (np.array(bg[1]) - np.array(bg[0])) * t
    zb = np.full((H, W), 1e9)
    ntri = len(V) // 3
    for q in range(ntri):
        a0, a1, a2 = q * 3, q * 3 + 1, q * 3 + 2
        if zv[a0] < 0.05 or zv[a1] < 0.05 or zv[a2] < 0.05: continue
        x0, y0, x1, y1, x2, y2 = sx[a0], sy[a0], sx[a1], sy[a1], sx[a2], sy[a2]
        minx = max(int(math.floor(min(x0, x1, x2))), 0); maxx = min(int(math.ceil(max(x0, x1, x2))), W - 1)
        miny = max(int(math.floor(min(y0, y1, y2))), 0); maxy = min(int(math.ceil(max(y0, y1, y2))), H - 1)
        if minx > maxx or miny > maxy: continue
        den = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2)
        if abs(den) < 1e-9: continue
        px, py = np.meshgrid(np.arange(minx, maxx + 1) + 0.5, np.arange(miny, maxy + 1) + 0.5)
        w0 = ((y1 - y2) * (px - x2) + (x2 - x1) * (py - y2)) / den
        w1 = ((y2 - y0) * (px - x2) + (x0 - x2) * (py - y2)) / den
        w2 = 1.0 - w0 - w1
        m = (w0 >= -1e-6) & (w1 >= -1e-6) & (w2 >= -1e-6)
        if not m.any(): continue
        z = w0 * zv[a0] + w1 * zv[a1] + w2 * zv[a2]
        sub = zb[miny:maxy + 1, minx:maxx + 1]
        m &= z < sub
        if not m.any(): continue
        sub[m] = z[m]
        col = (w0[..., None] * C[a0] + w1[..., None] * C[a1] + w2[..., None] * C[a2])
        img[miny:maxy + 1, minx:maxx + 1][m] = col[m]
    return Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))

def sheet(imgs, cols, path, labels=None):
    w, h = imgs[0].size
    rows = (len(imgs) + cols - 1) // cols
    s = Image.new("RGB", (w * cols, h * rows), (0, 0, 0))
    dr = ImageDraw.Draw(s)
    for k, im in enumerate(imgs):
        s.paste(im, ((k % cols) * w, (k // cols) * h))
        if labels: dr.text(((k % cols) * w + 6, (k // cols) * h + 4), labels[k], fill=(230, 230, 230))
    s.save(path); print("->", path)

def orbit(target, dist, az_deg, el_deg):
    a, e = math.radians(az_deg), math.radians(el_deg)
    return (target[0] + dist * math.sin(a) * math.cos(e), target[1] + dist * math.sin(e), target[2] + dist * math.cos(a) * math.cos(e))

def main():
    build()
    what = sys.argv[1] if len(sys.argv) > 1 else "hero"
    if what == "hero":
        v = dump(["hero", 0, 0, 0, 1, 0, 0], "hero") + ground()
        T = (0, 1.75, 0)
        views = [(180, 8, "frente"), (90, 8, "lado der"), (0, 12, "espalda"), (215, 14, "3/4 frente"), (0, 26, "camara de juego")]
        imgs = [render(v, orbit(T, 6.5 if az != 0 or el < 20 else 9.0, az, el), T) for az, el, _ in views]
        sheet(imgs, 5, os.path.join(OUT, "hero.png"), [x[2] for x in views])
    elif what in ("run", "walk"):
        run = what == "run"
        imgs, labels = [], []
        phases = [k * math.pi / 4 for k in range(8)]
        for p in phases:
            v = dump(["hero", "%.3f" % p, 1.0, 1.0 if run else 0.0, 1, 0, 0], "pose") + ground()
            T = (0, 1.6, 0)
            imgs.append(render(v, orbit(T, 7.0, 90, 4), T, W=220, H=300)); labels.append("lado %.2f" % p)
        for p in phases:
            v = dump(["hero", "%.3f" % p, 1.0, 1.0 if run else 0.0, 1, 0, 0], "pose") + ground()
            T = (0, 1.9, -1.0)
            imgs.append(render(v, (0, 4.5, 9.0), T, W=220, H=300, fov=34)); labels.append("atras %.2f" % p)
        sheet(imgs, 8, os.path.join(OUT, what + ".png"), labels)
    elif what == "model":
        name = sys.argv[2]
        v = dump(["model", name], "model_" + name)
        ys = [x[2] for x in v]; xs = [x[1] for x in v]; zs = [x[3] for x in v]
        cy = (min(ys) + max(ys)) * 0.5; size = max(max(ys) - min(ys), max(xs) - min(xs), max(zs) - min(zs))
        T = ((min(xs) + max(xs)) * 0.5, cy, (min(zs) + max(zs)) * 0.5)
        views = [(180, 10, "frente"), (90, 10, "lado"), (0, 10, "atras"), (220, 30, "3/4")]
        imgs = [render(v, orbit(T, size * 2.2 + 1.0, az, el), T) for az, el, _ in views]
        sheet(imgs, 4, os.path.join(OUT, "model_" + name + ".png"), [x[2] for x in views])
    elif what == "models":   # todos los del cementerio, cada uno en 3/4 de frente
        names = sys.argv[2:] or ["lapida_r0", "lapida_r1", "lapida_o", "cruz", "celta", "obelisco", "tumba_reja", "angel",
                                 "mausoleo", "cipres0", "arbol0", "farol", "banco", "fantasma", "reja", "porton", "capilla"]
        imgs = []
        for name in names:
            v = dump(["model", name], "m")
            ys = [x[2] for x in v]; xs = [x[1] for x in v]; zs = [x[3] for x in v]
            size = max(max(ys) - min(ys), max(xs) - min(xs), max(zs) - min(zs))
            T = ((min(xs) + max(xs)) * 0.5, (min(ys) + max(ys)) * 0.5, (min(zs) + max(zs)) * 0.5)
            imgs.append(render(v + ground(size * 0.8), orbit(T, size * 1.9 + 0.8, 205, 18), T, W=240, H=260))
        sheet(imgs, 6, os.path.join(OUT, "models.png"), names)
    elif what == "catedral":
        v = dump(["catedral"], "catedral")
        fog = (40.0, 170.0, (20, 20, 28))
        bg = ((24, 22, 30), (12, 12, 16))
        a = render(v, (0, 4.5, 41), (0, 6, 20), W=480, H=272, fov=66, fog=fog, bg=bg)
        b = render(v, (9, 9, 4), (-2, 8, -20), W=480, H=272, fov=66, fog=fog, bg=bg)
        c = render(v, (0, 5.5, 2), (0, 5.5, -12), W=480, H=272, fov=60, fog=fog, bg=bg)
        sheet([a, b, c], 3, os.path.join(OUT, "catedral.png"), ["entrada", "nave", "el vigia"])
    elif what == "scene":
        v = dump(["scene"], "scene")
        fog = (6.0, 60.0, (176, 178, 180))
        a = render(v, (0, 95, 60), (0, 0, -12), W=480, H=360, fov=55)
        b = render(v, (0, 3.2, 34), (0, 2.2, 20), W=480, H=272, fov=66, fog=fog, bg=((176, 178, 180), (176, 178, 180)))
        c = render(v, (-6, 4.5, 6), (2, 2.4, -14), W=480, H=272, fov=66, fog=fog, bg=((176, 178, 180), (176, 178, 180)))
        sheet([a.resize((480, 272)), b, c], 3, os.path.join(OUT, "scene.png"), ["aerea", "llegada", "camino"])

if __name__ == "__main__":
    main()
