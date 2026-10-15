/* Pista del serafin (assets/SERAPH.ADP: IMA ADPCM mono 32 kHz, la genera
   tools/make_adpcm.py desde el MP3). Va DENTRO del EBOOT: no depende de rutas del
   memory stick, funciona igual en PPSSPP y en la consola. */
    .section .rodata
    .balign 16
    .global g_seraphAdp
    .global g_seraphAdpEnd
g_seraphAdp:
    .incbin "SERAPH.ADP"
g_seraphAdpEnd:
    .byte 0
