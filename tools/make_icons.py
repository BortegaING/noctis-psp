# Portada de NOCTIS: ICON0 (144x80) y PIC1 (480x272) desde una captura del juego, + header C.
import sys, os
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageEnhance
shot, assets, header = sys.argv[1], sys.argv[2], sys.argv[3]
os.makedirs(assets, exist_ok=True)

def font(size, bold=True):
    for n in (["georgiab.ttf", "timesbd.ttf"] if bold else ["georgia.ttf", "times.ttf"]):
        p = os.path.join(r"C:\Windows\Fonts", n)
        if os.path.exists(p): return ImageFont.truetype(p, size)
    return ImageFont.load_default()

def spaced(d, xy, text, f, fill, spacing, anchor_center=True):
    w = sum(d.textlength(ch, font=f) for ch in text) + spacing * (len(text) - 1)
    x, y = xy
    if anchor_center: x -= w / 2
    for ch in text:
        d.text((x, y), ch, font=f, fill=fill); x += d.textlength(ch, font=f) + spacing

base = Image.open(shot).convert("RGB")
# PIC1: la escena entera, un poco mas fria y suave, vineta y titulo
pic = base.resize((480, 272), Image.LANCZOS)
pic = ImageEnhance.Contrast(pic).enhance(1.08)
vig = Image.new("L", (480, 272), 0); dv = ImageDraw.Draw(vig)
for k in range(40): dv.rectangle([k, k, 479 - k, 271 - k], outline=int(150 * (1 - k / 40)))
pic = Image.composite(Image.new("RGB", (480, 272), (18, 22, 30)), pic, vig.filter(ImageFilter.GaussianBlur(6)))
d = ImageDraw.Draw(pic)
d.rectangle([0, 196, 479, 250], fill=(16, 20, 28))
spaced(d, (240, 202), "PROJECT NOCTIS", font(26), (236, 238, 244), 5)
spaced(d, (240, 232), "LA TORRE", font(12, False), (200, 176, 120), 6)
pic.save(os.path.join(assets, "PIC1.PNG"), optimize=True)

# ICON0: recorte centrado de la escena + titulo
w, h = base.size
crop = base.crop((int(w * 0.18), int(h * 0.08), int(w * 0.82), int(h * 0.08 + (w * 0.64) * 80 / 144)))
ic = crop.resize((144, 80), Image.LANCZOS)
d = ImageDraw.Draw(ic, "RGBA")
d.rectangle([0, 54, 143, 79], fill=(14, 18, 26, 215))
spaced(d, (72, 57), "NOCTIS", font(15), (240, 242, 248), 3)
d.rectangle([0, 0, 143, 79], outline=(200, 176, 120))
ic.save(os.path.join(assets, "ICON0.PNG"), optimize=True)

def arr(name, data):
    body = ",".join(str(b) for b in data)
    return f"static const unsigned char {name}[] = {{{body}}};\nstatic const unsigned int {name}Size = {len(data)};\n"
with open(header, "w") as f:
    f.write("// GENERADO por make_icons.py: assets/ICON0.PNG y assets/PIC1.PNG en bytes (guardado con icono)\n")
    f.write(arr("g_icon0Png", open(os.path.join(assets, "ICON0.PNG"), "rb").read()))
    f.write(arr("g_pic1Png", open(os.path.join(assets, "PIC1.PNG"), "rb").read()))
print("ok", os.path.getsize(os.path.join(assets, "ICON0.PNG")), os.path.getsize(os.path.join(assets, "PIC1.PNG")))
