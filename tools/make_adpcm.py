# MP3 -> mono 32 kHz -> IMA ADPCM 4 bits (2 muestras por byte, nibble bajo primero).
# El juego lo decodifica en el hilo de audio (audio.h, audioTrackPlay).
import sys
import numpy as np, soundfile as sf
src, dst, rate = sys.argv[1], sys.argv[2], 32000
x, sr = sf.read(src, dtype="float64", always_2d=True)
x = x.mean(axis=1)
# pasa-bajos (sinc enventanada, corte 14.5 kHz) antes de bajar de 44.1 a 32 kHz
n = np.arange(-32, 33); fc = 14500 / sr
h = 2 * fc * np.sinc(2 * fc * n) * np.hamming(len(n)); h /= h.sum()
x = np.convolve(x, h, mode="same")
t = np.arange(0, len(x) / sr, 1 / rate)
y = np.interp(t, np.arange(len(x)) / sr, x)
y = y / (np.abs(y).max() + 1e-9) * 0.89                       # pico a -1 dB
pcm = np.clip(np.round(y * 32767), -32768, 32767).astype(np.int32)

STEP = [7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,
        157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,
        1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,
        10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767]
IDX = [-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8]
pred, idx = 0, 0
out = bytearray((len(pcm) + 1) // 2)
for k, s in enumerate(pcm.tolist()):
    step = STEP[idx]; diff = s - pred; code = 0
    if diff < 0: code = 8; diff = -diff
    d = step >> 3
    if diff >= step: code |= 4; diff -= step; d += step
    step >>= 1
    if diff >= step: code |= 2; diff -= step; d += step >> 0
    step >>= 1
    if diff >= step: code |= 1; d += step
    pred = pred - d if code & 8 else pred + d
    pred = -32768 if pred < -32768 else (32767 if pred > 32767 else pred)
    idx += IDX[code]; idx = 0 if idx < 0 else (88 if idx > 88 else idx)
    if k & 1: out[k >> 1] |= code << 4
    else:     out[k >> 1] = code
open(dst, "wb").write(out)
print("ok", len(pcm), "muestras", len(out), "bytes", round(len(pcm) / rate, 1), "s")
