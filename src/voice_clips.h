// GENERADO por tools/voice/make_voice.py -- NO editar a mano: las frases se cambian alla y se regenera.
// La voz de Baphomet: frases en IMA ADPCM dentro de assets/VOICE.ADP (src/voice_blob.S).
#pragma once
#define VOICE_RATE 16000
#define VC_NWHISPER 8   // las primeras: visiones de muerte
enum { VC_TRADITOR, VC_NEMO, VC_GENU, VC_CARO, VC_ILLE, VC_PUTRESCE, VC_MORIERIS, VC_VAE, VC_MALEDICTUS, VC_NUMQUAM, VC_DEUS, VC_N };
static const unsigned int kVoiceOff[VC_N] = { 0, 16933, 41512, 60186, 84053, 109681, 126441, 143710, 165287, 211031, 272492 };   // bytes
static const unsigned int kVoiceLen[VC_N] = { 16933, 24579, 18674, 23867, 25628, 16760, 17269, 21577, 45744, 61461, 30298 };
static const char *const kVoiceLat[VC_N] = {
    "TRADITOR",
    "NEMO TE AUDIT",
    "GENUFLECTE",
    "CARO TUA MEA EST",
    "ILLE NON VENIET",
    "PUTRESCE",
    "MORIERIS",
    "VAE TIBI",
    "MALEDICTUS SIS, LUCIS PORTATOR...",
    "NUMQUAM IN PACE DORMIES. IN INFERIS TE EXSPECTO.",
    "DEUS TUUS TE RELIQUIT",
};
static const char *const kVoiceEs[VC_N] = {
    "traidor",
    "nadie te escucha",
    "arrodíllate",
    "tu carne es mía",
    "Él no vendrá",
    "púdrete",
    "morirás",
    "¡ay de ti!",
    "(Maldito seas, portador de la luz...)",
    "(Jamás dormirás tranquilo. Te espero abajo.)",
    "tu Dios te abandonó",
};
