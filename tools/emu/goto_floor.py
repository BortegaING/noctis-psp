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
    stick(0, 1); time.sleep(38.5); stick(0, 0); time.sleep(0.4)   # y+ = stick ARRIBA
    press("triangle"); time.sleep(0.4)
    press("select")                       # atajo de pruebas: abre los pisos 1..7
    for _ in range(n - 1): press("down")  # el menu arranca en el piso 1
    time.sleep(0.2); press("cross")
    time.sleep(5.2)
    print("en el piso", n)

if __name__ == "__main__": main()
