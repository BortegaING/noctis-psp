/* La VOZ de Baphomet (assets/VOICE.ADP: frases en IMA ADPCM mono 16 kHz, una detras de otra; la
   genera tools/voice/make_voice.py, que escribe tambien src/voice_clips.h con donde empieza cada una).
   Va DENTRO del EBOOT, como la cancion del serafin. */
    .section .rodata
    .balign 16
    .global g_voiceAdp
g_voiceAdp:
    .incbin "VOICE.ADP"
    .byte 0
