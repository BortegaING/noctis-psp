"""NOCTIS: lleva al jugador del cielo al piso N por el elevador (con el atajo de pruebas SELECT,
que abre los pisos 1..7). Sirve con el emulador oculto (hidden.ps1) o en pantalla (show.ps1).
Uso: python goto_floor.py N        (N = 1..7; tarda ~45 s: caminata de 38.5 s + carga)
"""
import sys, time
from psp_input import find_port, ws_connect, ws_send

def main():
    n = int(sys.argv[1])
    s = ws_connect(find_port())
    def stick(x, y): ws_send(s, {"event": "input.analog.send", "stick": "left", "x": x, "y": y})
    def press(b, ms=100):
        ws_send(s, {"event": "input.buttons.press", "button": b, "duration": max(2, ms // 16)})
        time.sleep(ms / 1000 + 0.15)
    press("cross"); time.sleep(1.0)       # menu principal: JUGAR (partida nueva, desde el cielo)
    # y+ = stick ARRIBA. 38.5 s CAMINANDO hasta el elevador. Desde que existe CORRER (nub a
    # fondo 3 s seguidos) hay que aflojarlo un instante cada 2.8 s (0.7 queda bajo el umbral
    # de 0.8 y reinicia la cuenta): si corre, llega antes y se pasa del elevador.
    t0 = time.time()
    while time.time() - t0 < 38.5:
        stick(0, 1); time.sleep(min(2.8, 38.5 - (time.time() - t0)))
        if time.time() - t0 < 38.5: stick(0, 0.7); time.sleep(0.12)
    stick(0, 0); time.sleep(0.4)
    press("triangle"); time.sleep(0.4)
    # atajo de pruebas: abre los pisos 1..7. Al piso 1 NO: con los pisos abiertos el piso 1 cuenta
    # como ganado y los ojos no aparecen (y el piso 1 ya esta abierto en una partida nueva)
    if n > 1: press("select")
    for _ in range(n - 1): press("down")  # el menu arranca en el piso 1
    time.sleep(0.2); press("cross")
    time.sleep(5.2)
    print("en el piso", n)

if __name__ == "__main__": main()
