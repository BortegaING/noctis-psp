"""NOCTIS -- la VOZ de Baphomet: maldiciones en latin (Benjamin, 2026-10-16: "generale una voz que diga
maldiciones en latin, tiene que sonar intimidante, no tiene que dar risa").

  python tools/voice/make_voice.py        -> assets/VOICE.ADP + src/voice_clips.h
                                             + build/voice/*.wav y el Escritorio/NOCTIS_voz_baphomet.wav (para oir)

Las frases estan en CLIPS (abajo): cambiar ahi y volver a correr. Cadena:
  1) Windows dice cada frase con "Microsoft Raul" (tools/voice/tts_winrt.ps1; lectura castellana =
     latin eclesiastico). La ortografia de lo que LEE va acentuada donde cae el golpe en latin.
  2) Se toca LENTO: x0.80 de velocidad baja 3.9 semitonos sin artefactos (como una cinta lenta: la voz
     sale mas grande y pesada, no "de robot").
  3) Capas, todas BAJAS (lo que da risa es el efecto "demonio" exagerado):
       - una octava abajo al 22 %, solo graves: el pecho de algo enorme;
       - un SUSURRO de la misma frase (el espectro de la voz con fase al azar), al 20 %, 30 ms tarde: es lo que
         da escalofrio, como si alguien mas lo dijera pegado a la oreja.
  4) Saturacion suave, sin agudos (corte 5 kHz) y la CUEVA: reverberacion larga (2.6 s) y oscura.
  5) IMA ADPCM 4 bits a 16 kHz (igual que la cancion del serafin): cada frase arranca su decodificador
     de cero, asi el juego la puede tocar sola.
"""
import os, sys, subprocess
import numpy as np, soundfile as sf

# PERILLAS (si suena a caricatura: bajar SUB y WHISPER; si suena a locutor: subir CAVE)
SPEED   = 0.80    # velocidad (y tono: 0.80 = -3.9 semitonos)
SUB     = 0.22    # capa una octava abajo
WHISPER = 0.20    # el susurro (con mas, la voz se vuelve ronca y tapa los agudos)
DRIVE   = 1.25    # saturacion
CAVE    = 0.42    # cuanto eco de cueva

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(ROOT, "build", "voice")
SR = 16000                       # la voz de Windows sale a 16 kHz y el juego la toca a 16 kHz

# (enum, en pantalla (latin), que significa, lo que LEE la voz)
CLIPS = [
    ("VC_TRADITOR",  "TRADITOR",          "traidor",                 "Tráditor."),
    ("VC_NEMO",      "NEMO TE AUDIT",     "nadie te escucha",        "Némo te... áudit."),
    ("VC_GENU",      "GENUFLECTE",        "arrodíllate",             "Genuflécte."),
    ("VC_CARO",      "CARO TUA MEA EST",  "tu carne es mía",         "Cáro túa... méa est."),
    ("VC_ILLE",      "ILLE NON VENIET",   "Él no vendrá",            "Íle... non véniet."),
    ("VC_PUTRESCE",  "PUTRESCE",          "púdrete",                 "Putrésce."),
    ("VC_MORIERIS",  "MORIERIS",          "morirás",                 "Moriéris."),
    ("VC_VAE",       "VAE TIBI",          "¡ay de ti!",              "Ve... tíbi."),
    ("VC_MALEDICTUS", "MALEDICTUS SIS, LUCIS PORTATOR...", "(Maldito seas, portador de la luz...)",
                     "Maledíctus sis... <break time='250ms'/> lúcis portátor."),
    ("VC_NUMQUAM",   "NUMQUAM IN PACE DORMIES. IN INFERIS TE EXSPECTO.", "(Jamás dormirás tranquilo. Te espero abajo.)",
                     "Númquam in páce dórmies. <break time='450ms'/> In ínferis... te exspécto."),
    ("VC_DEUS",      "DEUS TUUS TE RELIQUIT", "tu Dios te abandonó",  "Déus túus... te relíquit."),
]
N_WHISPER = 8                    # las 8 primeras son las de las visiones de muerte

# ------------------------------------------------------------------ DSP (solo numpy)
def fft_filter(x, lo=None, hi=None, order_hi=8, order_lo=4):
    n = 1 << int(np.ceil(np.log2(len(x) + 1)))
    X = np.fft.rfft(x, n); f = np.fft.rfftfreq(n, 1.0 / SR); H = np.ones_like(f)
    if hi: H *= 1.0 / np.sqrt(1.0 + (f / hi) ** order_hi)
    if lo: H *= 1.0 / np.sqrt(1.0 + (lo / np.maximum(f, 1e-3)) ** order_lo)
    return np.fft.irfft(X * H, n)[:len(x)]

def speed(x, k):
    """Tocar a k de velocidad (k < 1: mas lento Y mas grave, como una cinta)."""
    n = int(len(x) / k); t = np.arange(n) * k
    return np.interp(t, np.arange(len(x)), x)

NF, HOP = 1024, 128
WIN = np.hanning(NF)
def stft(x):
    x = np.concatenate([np.zeros(NF), x, np.zeros(NF)])
    m = 1 + (len(x) - NF) // HOP
    return np.array([np.fft.rfft(x[i * HOP:i * HOP + NF] * WIN) for i in range(m)])
def istft(S, n):
    y = np.zeros(NF + HOP * (len(S) - 1)); w = np.zeros_like(y)
    for i, s in enumerate(S):
        y[i * HOP:i * HOP + NF] += np.fft.irfft(s, NF) * WIN; w[i * HOP:i * HOP + NF] += WIN ** 2
    y /= np.maximum(w, 1e-6)
    return y[NF:NF + n]

def stretch(x, r):
    """Vocoder de fase: dura 1/r sin cambiar el tono."""
    S = stft(x); m = len(S)
    steps = np.arange(0, m - 1, r)
    om = 2 * np.pi * HOP * np.arange(NF // 2 + 1) / NF
    ph = np.angle(S[0]); out = []
    for t in steps:
        i = int(t); fr = t - i
        mag = (1 - fr) * np.abs(S[i]) + fr * np.abs(S[i + 1])
        out.append(mag * np.exp(1j * ph))
        dp = np.angle(S[i + 1]) - np.angle(S[i]) - om
        dp -= 2 * np.pi * np.round(dp / (2 * np.pi))
        ph = ph + om + dp
    return istft(np.array(out), int(len(x) / r))

def fit(y, n):
    return y[:n] if len(y) >= n else np.concatenate([y, np.zeros(n - len(y))])

def octave_down(x):
    return fit(speed(stretch(x, 2.0), 0.5), len(x))

def whisper(x, seed):
    """El mismo espectro con fase al azar: la frase SUSURRADA."""
    S = stft(x); rng = np.random.default_rng(seed)
    W = np.abs(S) * np.exp(2j * np.pi * rng.random(S.shape))
    return fft_filter(istft(W, len(x)), lo=900)

def cave_ir(rt60=2.6, length=3.0):
    n = int(length * SR); t = np.arange(n) / SR
    rng = np.random.default_rng(7)
    ir = rng.standard_normal(n) * np.exp(-6.91 * t / rt60)
    bright, dark = fft_filter(ir, hi=4200), fft_filter(ir, hi=1300)
    w = np.clip(t / 0.7, 0, 1)                       # cada rebote mas opaco: piedra
    ir = bright * (1 - w) + dark * w
    ir[:int(0.028 * SR)] = 0.0                       # la pared mas cercana esta lejos
    ir /= np.sqrt(np.sum(ir ** 2))
    for d, g in ((0.031, 0.55), (0.047, 0.38), (0.073, 0.30), (0.109, 0.22), (0.151, 0.15)):
        ir[int(d * SR)] += g                         # primeras reflexiones
    return ir

def convolve(x, h):
    n = 1 << int(np.ceil(np.log2(len(x) + len(h))))
    return np.fft.irfft(np.fft.rfft(x, n) * np.fft.rfft(h, n), n)[:len(x) + len(h) - 1]

def trim(x, th=0.02):
    a = np.abs(x); idx = np.where(a > th * a.max())[0]
    return x[max(0, idx[0] - 160):idx[-1] + 320] if len(idx) else x

def process(raw, seed, ir):
    x = trim(raw)
    x = fft_filter(x, lo=70, hi=6500)
    x = speed(x, SPEED)                              # mas lento y grave (-3.9 semitonos)
    x /= np.abs(x).max() + 1e-9
    sub = fft_filter(octave_down(x), hi=900) * SUB   # el pecho
    wh = whisper(x, seed)
    wh *= WHISPER * np.sqrt(np.mean(x ** 2)) / (np.sqrt(np.mean(wh ** 2)) + 1e-9)
    d = int(0.030 * SR)
    v = x + sub + np.concatenate([np.zeros(d), wh[:-d]])
    v = np.tanh(DRIVE * v / np.abs(v).max()) / np.tanh(DRIVE)   # saturacion suave
    v = fft_filter(v, lo=60, hi=5000)
    wet = convolve(v, ir)
    wet *= CAVE * np.sqrt(np.mean(v ** 2)) / (np.sqrt(np.mean(wet[:len(v)] ** 2)) + 1e-9)
    y = wet.copy(); y[:len(v)] += v
    y = fft_filter(y, lo=55, hi=5200)
    env = np.abs(y); k = len(y) - 1
    while k > len(v) and env[k] < 0.004 * env.max(): k -= 1     # corta la cola cuando ya no se oye
    y = y[:k + 1]
    fade = min(len(y), int(0.25 * SR)); y[-fade:] *= np.linspace(1, 0, fade)
    return y / (np.abs(y).max() + 1e-9) * 0.89

# ------------------------------------------------------------------ IMA ADPCM (igual que tools/make_adpcm.py)
STEP = [7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,
        157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,
        1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,
        10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767]
IDX = [-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8]
def adpcm(y):
    pcm = np.clip(np.round(y * 32767), -32768, 32767).astype(np.int32).tolist()
    if len(pcm) & 1: pcm.append(0)
    pred, idx = 0, 0; out = bytearray(len(pcm) // 2)
    for k, s in enumerate(pcm):
        step = STEP[idx]; diff = s - pred; code = 0
        if diff < 0: code = 8; diff = -diff
        d = step >> 3
        if diff >= step: code |= 4; diff -= step; d += step
        step >>= 1
        if diff >= step: code |= 2; diff -= step; d += step
        step >>= 1
        if diff >= step: code |= 1; d += step
        pred = pred - d if code & 8 else pred + d
        pred = -32768 if pred < -32768 else (32767 if pred > 32767 else pred)
        idx += IDX[code]; idx = 0 if idx < 0 else (88 if idx > 88 else idx)
        if k & 1: out[k >> 1] |= code << 4
        else:     out[k >> 1] = code
    return bytes(out)

def main():
    os.makedirs(OUT, exist_ok=True)
    lines = os.path.join(OUT, "frases.txt")
    with open(lines, "w", encoding="utf-8") as f:
        for c in CLIPS: f.write(c[3] + "\n")
    r = subprocess.run(["powershell", "-ExecutionPolicy", "Bypass", "-File", os.path.join(HERE, "tts_winrt.ps1"), lines, OUT],
                       capture_output=True, text=True)
    print(r.stdout.strip(), r.stderr.strip())
    if r.returncode != 0: sys.exit("fallo la voz de Windows")
    ir = cave_ir()
    blob, offs, lens, allv = bytearray(), [], [], []
    for k, c in enumerate(CLIPS):
        raw, sr = sf.read(os.path.join(OUT, "raw_%02d.wav" % k), dtype="float64", always_2d=True)
        assert sr == SR, sr
        y = process(raw.mean(axis=1), 100 + k, ir)
        sf.write(os.path.join(OUT, "vc_%02d_%s.wav" % (k, c[0][3:].lower())), y, SR, subtype="PCM_16")
        a = adpcm(y); offs.append(len(blob)); lens.append(len(a)); blob += a
        allv += [y, np.zeros(int(0.5 * SR))]
        print("  %-14s %4.1f s  %6d bytes" % (c[0], len(y) / SR, len(a)))
    open(os.path.join(ROOT, "assets", "VOICE.ADP"), "wb").write(bytes(blob))
    desk = os.path.join(os.path.expanduser("~"), "Desktop", "NOCTIS_voz_baphomet.wav")
    sf.write(desk, np.concatenate(allv), SR, subtype="PCM_16")
    def q(s): return '"' + s.replace('"', '\\"') + '"'
    h = ["// GENERADO por tools/voice/make_voice.py -- NO editar a mano: las frases se cambian alla y se regenera.",
         "// La voz de Baphomet: frases en IMA ADPCM dentro de assets/VOICE.ADP (src/voice_blob.S).",
         "#pragma once", "#define VOICE_RATE %d" % SR, "#define VC_NWHISPER %d   // las primeras: visiones de muerte" % N_WHISPER,
         "enum { " + ", ".join(c[0] for c in CLIPS) + ", VC_N };",
         "static const unsigned int kVoiceOff[VC_N] = { " + ", ".join(str(o) for o in offs) + " };   // bytes",
         "static const unsigned int kVoiceLen[VC_N] = { " + ", ".join(str(l) for l in lens) + " };",
         "static const char *const kVoiceLat[VC_N] = {"] + ["    " + q(c[1]) + "," for c in CLIPS] + ["};",
         "static const char *const kVoiceEs[VC_N] = {"] + ["    " + q(c[2]) + "," for c in CLIPS] + ["};", ""]
    open(os.path.join(ROOT, "src", "voice_clips.h"), "w", encoding="utf-8", newline="\n").write("\n".join(h))
    print("VOICE.ADP: %d bytes (%.1f s de voz)  ->  %s" % (len(blob), len(blob) * 2 / SR, desk))

if __name__ == "__main__":
    main()
