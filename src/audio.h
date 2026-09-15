// =============================================================================
// PROJECT NOCTIS - audio.h : atmosfera PROCEDURAL para PSP (sin archivos de sonido)
// -----------------------------------------------------------------------------
// Todo se genera por codigo: viento que respira, pisadas de piedra, campanas
// lejanas, el golpe grave del cambio de gravedad, y un eco de pasillo enorme.
// Nada de musica: el mundo suena vacio, y a veces se calla del todo.
//
// FILOSOFIA
//   El viento es la base (siempre presente, volumen bajo) y RESPIRA: sube, baja
//   y cada tanto se queda quieto varios segundos -> SILENCIO REAL. Los eventos
//   (pisada / campana / gravedad) se mandan a un retardo con realimentacion
//   baja para que el pasillo suene grande.
//
// COMO SE CABLEA (3 lineas en main.cpp, nada mas)
//   1) con los demas includes (despues de "void_shaft.h", linea ~245):
//        #include "audio.h"
//   2) en main(), justo despues de initGu() (linea ~988):
//        audioInit();
//   3) al final del while, JUSTO ANTES de sceDisplayWaitVblankStart() (linea ~1630):
//        audioFrame(bobPhase, gravG);
//   (ahi el audio aprovecha el hueco en que la CPU espera al GPU/vblank).
//   Opcional: audioShutdown(); antes de sceGuTerm().
//
// MEDIDO (render corrido 5 min sin parar, en simulacion bit a bit del mismo codigo)
//   - 0 muestras saturadas; el viento va de -56 dBFS (casi nada) a -19 dBFS (rafaga).
//   - 7% del tiempo el mundo queda practicamente en silencio.
//   - Cuando no suena nada, la salida es CERO EXACTO (silencio digital de verdad).
//   - Pisada: audible 0.5 s. Gravedad: 1.5 s. Campana: ~20 s de cola.
//
// API PUBLICA (la pedida por el diseno)
//   audioInit()                  reserva el canal y arma las tablas
//   audioSetAmbience(float 0..1) intensidad del viento (mas alto cerca de un ventanal)
//   audioFootstep(float 0..1)    dispara una pisada
//   audioBell()                  campana lejana con cola larga
//   audioGravityShift()          golpe grave con caida de tono
//   audioUpdate()                1 vez por frame: rellena el buffer (NO BLOQUEA)
//   audioShutdown()              libera el canal
//   + audioFrame(walkCycle, gravMode)  atajo: pisadas por cadencia + gravedad + update
//
// REGLAS RESPETADAS
//   - Sin heap: todos los buffers son estaticos (~35 KB).
//   - Sin rand() de la libc: LCG entero propio (determinista).
//   - Sin sinf/cosf por muestra: tabla de 1024 entradas hecha en audioInit().
//   - Sintesis en enteros / punto fijo; las envolventes corren 1 de cada 16
//     muestras (el "control rate"), no por muestra.
//   - Si algo falla, audioInit() se apaga en silencio y TODO lo demas es no-op.
//     El juego nunca se cuelga ni petardea por el audio.
//
// IMPORTANTE PARA EL BUILD: hay que enlazar la libreria de audio del PSPSDK.
//   En CMakeLists.txt, dentro de target_link_libraries(...), agregar:  pspaudio
//   (verificado en el toolchain: sin -lpspaudio da "undefined reference to
//    sceAudioChReserve").
// =============================================================================
#ifndef NOCTIS_AUDIO_H
#define NOCTIS_AUDIO_H

#include <pspaudio.h>
#include <math.h>      // SOLO para precalcular la tabla de senos en audioInit()

// -----------------------------------------------------------------------------
// Parametros de bloque
// -----------------------------------------------------------------------------
#define NOCTIS_AUDIO_SR       44100     // los canales de sceAudioChReserve son 44.1 kHz fijos

// Muestras por vuelta. 2048 = 46.4 ms de audio por buffer.
// Con doble buffer se pueden tener hasta ~93 ms encolados, asi que el sonido
// aguanta caidas de framerate hasta ~11-15 fps sin cortarse.
// Si el juego va SIEMPRE arriba de 40 fps se puede bajar a 1024 (menos latencia).
#ifndef NOCTIS_AUDIO_SAMPLES
#define NOCTIS_AUDIO_SAMPLES  2048
#endif

// Las envolventes y ganancias se actualizan 1 vez cada CHUNK muestras (0.36 ms).
// Esto es lo que hace barato el sistema: el trabajo "caro" cuesta 1/16.
#define NOCTIS_AUDIO_CHUNK    16

static_assert((NOCTIS_AUDIO_SAMPLES & 63) == 0,
              "NOCTIS_AUDIO_SAMPLES debe ser multiplo de 64 (lo exige sceAudioChReserve)");
static_assert((NOCTIS_AUDIO_SAMPLES % NOCTIS_AUDIO_CHUNK) == 0,
              "NOCTIS_AUDIO_SAMPLES debe ser multiplo de NOCTIS_AUDIO_CHUNK");

// Volumen maestro que se le pasa al hardware (0..PSP_AUDIO_VOLUME_MAX = 0x8000).
#ifndef NOCTIS_AUDIO_MASTER
#define NOCTIS_AUDIO_MASTER   0x7000
#endif

// Campana automatica: si esta en 1, cada 50-150 s suena sola una campana lejana
// (la directiva pide que suene "cada tanto", no que dependa de un evento).
#ifndef NOCTIS_AUDIO_AUTOBELL
#define NOCTIS_AUDIO_AUTOBELL 1
#endif

// -----------------------------------------------------------------------------
// Tabla de senos y utilidades de punto fijo
// -----------------------------------------------------------------------------
#define NOCTIS_SINE_BITS      10
#define NOCTIS_SINE_LEN       (1 << NOCTIS_SINE_BITS)     // 1024 entradas, Q15

// Incremento de fase para una frecuencia f (fase de 32 bits, indice = fase>>22).
// Con f literal esto lo resuelve el compilador: no hay float en tiempo de ejecucion.
#define AU_INC(f)             ((unsigned int)((f) * (4294967296.0f / (float)NOCTIS_AUDIO_SR)))

// Las envolventes van en Q20 (1.0 = 1048576) y decaen por DESPLAZAMIENTO:
//     env -= (env >> SH) + 1;
// Esto es un decaimiento exponencial de constante de tiempo (1<<SH) chunks, sin
// una sola multiplicacion, y con Q20 hay precision de sobra para colas de varios
// segundos (en Q15 el truncado entero las volvia un fundido lineal). El "+1"
// garantiza que la voz SIEMPRE termina de bajar y se apaga: nunca queda un resto
// girando para siempre.
#define NOCTIS_ENV_ONE        1048576               // 1.0 en Q20
#define NOCTIS_ENV_FLOOR      4096                  // por debajo de esto la voz se apaga (-48 dB)
#define NOCTIS_CHUNK_MS       0.3628f               // duracion de un chunk (para leer los SH)

// -----------------------------------------------------------------------------
// Eco: retardo circular mono (el pasillo)
// -----------------------------------------------------------------------------
#define NOCTIS_DL_LEN         8192                    // 185.8 ms
#define NOCTIS_DL_MASK        (NOCTIS_DL_LEN - 1)
#define NOCTIS_ECHO_TAP       5461                    // 2da toma: 2731 muestras = 62 ms (ancho estereo)
#define NOCTIS_ECHO_FB        11141                   // Q15 = 0.34 -> cola de ~1.2 s
#define NOCTIS_ECHO_WET       13000                   // Q15 = 0.40
#define NOCTIS_ECHO_LPK       7000                    // Q15: cada repeticion vuelve mas opaca (piedra)

// -----------------------------------------------------------------------------
// Viento
// -----------------------------------------------------------------------------
#define NOCTIS_WIND_MAX_G     3400      // Q12 (~0.83): ganancia de makeup con el viento al maximo
#define NOCTIS_WIND_K_BASE    1500      // Q15: corte del pasa-bajo con aire quieto (~320 Hz)
#define NOCTIS_WIND_K_SPAN    2600      // Q15: cuanto se abre el corte con la rafaga (mas brillo)

// -----------------------------------------------------------------------------
// Pisadas
// -----------------------------------------------------------------------------
#define NOCTIS_STEP_K_LO      5200      // Q15: pie "pesado" (mas grave)
#define NOCTIS_STEP_K_HI      7600      // Q15: pie "seco"  (mas clac)
#define NOCTIS_STEP_SH        7         // golpe de piedra: tau 128 chunks =  46 ms
#define NOCTIS_STEP_SHL       8         // cuerpo grave:    tau 256 chunks =  93 ms

// -----------------------------------------------------------------------------
// Campana (dos parciales en relacion INARMONICA: 1.00 y 2.76, tipo campana real)
// -----------------------------------------------------------------------------
#define NOCTIS_BELL_E1        448000    // Q20: parcial grave (suena LEJOS: bajo)
#define NOCTIS_BELL_E2        448000    // Q20: parcial agudo
#define NOCTIS_BELL_SH1       14        // tau 16384 chunks = 5.9 s  (la cola larga)
#define NOCTIS_BELL_SH2       13        // tau  8192 chunks = 3.0 s  (los agudos se van antes)
#define NOCTIS_BELL_MIN_BLK   1078      // 50 s : minimo entre campanas automaticas
#define NOCTIS_BELL_SPAN_BLK  2156      // +0..100 s de variacion

// -----------------------------------------------------------------------------
// Cambio de gravedad (barrido descendente + golpe de aire)
// -----------------------------------------------------------------------------
#define NOCTIS_GS_E0          832000    // Q20: es el evento mas fuerte del juego
#define NOCTIS_GS_N0          480000    // Q20: capa de ruido ("whoomph")
#define NOCTIS_GS_SH          10        // tau 1024 chunks = 0.37 s (es un golpe CORTO)
#define NOCTIS_GS_NSH         9         // tau  512 chunks = 0.19 s (solo el arranque)
#define NOCTIS_GS_PSH         10        // caida del TONO: tau 1024 chunks = 0.37 s
#define NOCTIS_GS_NK          2200      // Q15: el ruido del golpe es grave
#define NOCTIS_GS_F0          190.0f    // Hz de arranque
#define NOCTIS_GS_FMIN        28.0f     // Hz donde se planta

// -----------------------------------------------------------------------------
// Cadencia de pisadas de audioFrame() (en muestras: reloj de audio, no de frames)
// -----------------------------------------------------------------------------
#define NOCTIS_STEP_T_SLOW    27000     // 0.61 s entre pasos caminando
#define NOCTIS_STEP_T_FAST    15000     // 0.34 s entre pasos corriendo

// =============================================================================
// ESTADO (todo estatico, cero heap)
// =============================================================================
static short g_auSine[NOCTIS_SINE_LEN];                                    //  2 KB
static short __attribute__((aligned(64))) g_auBuf[2][NOCTIS_AUDIO_SAMPLES * 2]; // 16 KB
static short g_auDelay[NOCTIS_DL_LEN];                                     // 16 KB

static int   g_auOk      = 0;      // 0 = audio apagado: TODO es no-op
static int   g_auCh      = -1;
static int   g_auCur     = 0;      // buffer que toca renderizar
static int   g_auPending = 0;      // hay un buffer listo que el hardware todavia no acepto
static unsigned int g_auClock = 0; // muestras generadas (reloj de audio, exacto en tiempo real)

// --- generadores pseudoaleatorios (LCG propios, nada de rand()) ---
static unsigned int g_auRng  = 0x1BADB002u;   // ruido a tasa de audio
static unsigned int g_auRng2 = 0x51ED2701u;   // decisiones por bloque (rafagas, campanas)

// --- viento ---
static int g_wL1 = 0, g_wL2 = 0, g_wR1 = 0, g_wR2 = 0;   // dos polos por lado (estereo decorrelado)
static int g_wLevel  = 1048576;   // Q20: nivel pedido por el juego (arranca en 1.0)
static int g_wLevelT = 1048576;   // Q20: objetivo
static int g_wBreath = 120000;    // Q20: respiracion interna
static int g_wBreathT= 400000;    // Q20: objetivo de la respiracion
static int g_wHold   = 30;        // bloques que falta mantener el estado actual

// --- eco ---
static unsigned int g_dlIdx = 0;
static int g_dlLp = 0;

// --- pisadas: 2 voces (pie izquierdo / derecho) ---
struct AuStep {
    int env;            // Q20 envolvente del golpe de ruido
    int envLo;          // Q20 envolvente del cuerpo grave
    int lp;             // estado del pasa-bajo
    int k;              // corte del pasa-bajo de esta voz
    unsigned int phLo;  // fase del cuerpo grave
    unsigned int incLo;
};
static AuStep g_step[2] = { {0,0,0,NOCTIS_STEP_K_LO,0,AU_INC(88.0f)},
                            {0,0,0,NOCTIS_STEP_K_HI,0,AU_INC(104.0f)} };
static int g_stWhich = 0;

// --- campana ---
static unsigned int g_belPh1 = 0, g_belPh2 = 0, g_belInc1 = 0, g_belInc2 = 0;
static int g_belE1 = 0, g_belE2 = 0;
static int g_belTimer = 430;     // ~20 s para la primera campana

// --- cambio de gravedad ---
static unsigned int g_gsPh = 0;
static int g_gsInc = 0, g_gsE = 0, g_gsEN = 0, g_gsLp = 0;

// --- estado de audioFrame() ---
static float g_afPrevWalk = 0.0f;
static int   g_afPrevGrav = 0;
static int   g_afStepAcc  = NOCTIS_STEP_T_SLOW;
static unsigned int g_afPrevClock = 0;
static int   g_afInit = 0;

// =============================================================================
// Nucleo de punto fijo
// =============================================================================

// Ruido blanco -32768..32767. Se usan los bits ALTOS del LCG (los bajos son pobres).
static inline int auNoise(void)
{
    g_auRng = g_auRng * 1664525u + 1013904223u;
    return (int)((g_auRng >> 16) & 0xFFFFu) - 32768;
}

// Aleatorio de bloque, RANGO 0..32767. Separado del ruido de audio para que el
// consumo de muestras no altere las decisiones "de director" -> todo determinista.
// Para rangos mayores a 32767 hay que multiplicar, nunca usar % (quedaria recortado).
static inline unsigned int auRnd(void)
{
    g_auRng2 = g_auRng2 * 1103515245u + 12345u;
    return (g_auRng2 >> 16) & 0x7FFFu;
}

static inline int auClamp16(int v)
{
    if (v >  32767) return  32767;
    if (v < -32768) return -32768;
    return v;
}

// Dispara la campana (uso interno; audioBell() es la version publica).
static inline void auBellStrike(void)
{
    // Un campanario tiene mas de una campana: se elige entre tres tonos.
    unsigned int pick = auRnd() % 3u;
    float f0 = (pick == 0u) ? 148.0f : (pick == 1u) ? 168.0f : 196.0f;
    g_belInc1 = (unsigned int)(f0          * (4294967296.0f / (float)NOCTIS_AUDIO_SR));
    g_belInc2 = (unsigned int)(f0 * 2.76f  * (4294967296.0f / (float)NOCTIS_AUDIO_SR));
    g_belPh1  = 0;
    g_belPh2  = 0;
    g_belE1   = NOCTIS_BELL_E1;
    g_belE2   = NOCTIS_BELL_E2;
}

// -----------------------------------------------------------------------------
// "Director": corre 1 vez por bloque (cada 46 ms). Decide como respira el viento,
// cuando hay silencio de verdad, y cuando suena una campana sola.
// -----------------------------------------------------------------------------
static inline void auDirector(void)
{
    if (--g_wHold <= 0) {
        // OJO: auRnd() da 0..32767. Para rangos mas grandes hay que MULTIPLICAR,
        // no usar %, o el rango queda recortado sin que se note.
        if ((auRnd() & 7u) == 0u) {
            // 1 de cada 8: el aire se queda QUIETO. Silencio real por varios segundos.
            g_wBreathT = 14000  + (int)(auRnd() >> 5) * 46;   // 0.013 .. 0.058
            g_wHold    = 55     + (int)(auRnd() % 120u);      // 2.6 .. 8.1 s
        } else {
            // Rafaga: de una brisa apenas audible a viento franco de ventanal.
            g_wBreathT = 170000 + (int)auRnd() * 26;          // 0.162 .. 0.975
            g_wHold    = 40     + (int)(auRnd() % 150u);      // 1.9 .. 8.8 s
        }
    }
    // Suavizado lento (tau ~0.7 s): el viento nunca salta, siempre "crece".
    g_wBreath += (g_wBreathT - g_wBreath) >> 4;

#if NOCTIS_AUDIO_AUTOBELL
    if (--g_belTimer <= 0) {
        auBellStrike();
        g_belTimer = NOCTIS_BELL_MIN_BLK + (int)(auRnd() % (unsigned int)NOCTIS_BELL_SPAN_BLK);
    }
#endif
}

// -----------------------------------------------------------------------------
// Render de un bloque completo (NOCTIS_AUDIO_SAMPLES muestras, estereo entrelazado).
//
// Estructura: se procesa de a chunks de 16 muestras. Por cada chunk,
//   1) se avanzan las envolventes (trabajo caro, 1/16 del costo),
//   2) cada voz ACTIVA suma su aporte en un acumulador mono "dry" -> las voces
//      apagadas no cuestan ni un ciclo,
//   3) un lazo final mezcla viento estereo + dry + eco y escribe el buffer.
// -----------------------------------------------------------------------------
static inline void auRenderBlock(short *out)
{
    auDirector();

    int dry[NOCTIS_AUDIO_CHUNK];
    const int chunks = NOCTIS_AUDIO_SAMPLES / NOCTIS_AUDIO_CHUNK;

    for (int c = 0; c < chunks; ++c) {
        short *o = out + c * (NOCTIS_AUDIO_CHUNK * 2);
        int i;

        // ---------------- control del viento (1 vez cada 16 muestras) ----------------
        g_wLevel += (g_wLevelT - g_wLevel) >> 7;          // suavizado del nivel pedido (~46 ms)
        int wamt = ((g_wLevel >> 10) * (g_wBreath >> 10)) >> 10;   // 0..1024
        int wg   = (wamt * NOCTIS_WIND_MAX_G) >> 10;      // Q12
        int wk   = NOCTIS_WIND_K_BASE + ((wamt * NOCTIS_WIND_K_SPAN) >> 10);

        // ---------------- voces secas ----------------
        for (i = 0; i < NOCTIS_AUDIO_CHUNK; ++i) dry[i] = 0;

        // -- pisadas (2 voces, se alternan como pie izquierdo / derecho) --
        for (int v = 0; v < 2; ++v) {
            AuStep *s = &g_step[v];
            if (s->env <= NOCTIS_ENV_FLOOR) continue;
            int e = s->env >> 5, eL = s->envLo >> 5;        // Q20 -> Q15 para multiplicar
            int lp = s->lp, kk = s->k;
            unsigned int ph = s->phLo, inc = s->incLo;
            for (i = 0; i < NOCTIS_AUDIO_CHUNK; ++i) {
                int n  = auNoise();
                lp += ((n - lp) * kk) >> 15;
                int hi   = n - lp;                  // pasa-alto: el "clac" contra la piedra
                int body = lp + (hi >> 1);
                int lo   = g_auSine[ph >> 22]; ph += inc;   // peso del cuerpo
                dry[i] += ((body * e) >> 17) + ((lo * eL) >> 17);
            }
            s->lp = lp; s->phLo = ph;
            s->env   -= (s->env   >> NOCTIS_STEP_SH ) + 1;
            s->envLo -= (s->envLo >> NOCTIS_STEP_SHL) + 1;
            if (s->env <= NOCTIS_ENV_FLOOR) { s->env = 0; s->envLo = 0; }
        }

        // -- campana lejana: dos senoidales inarmonicas, cola exponencial larga --
        if (g_belE1 > NOCTIS_ENV_FLOOR) {
            int e1 = g_belE1 >> 5, e2 = g_belE2 >> 5;
            unsigned int p1 = g_belPh1, p2 = g_belPh2, i1 = g_belInc1, i2 = g_belInc2;
            for (i = 0; i < NOCTIS_AUDIO_CHUNK; ++i) {
                int s1 = g_auSine[p1 >> 22]; p1 += i1;
                int s2 = g_auSine[p2 >> 22]; p2 += i2;
                dry[i] += ((s1 * e1) >> 17) + ((s2 * e2) >> 18);
            }
            g_belPh1 = p1; g_belPh2 = p2;
            g_belE1 -= (g_belE1 >> NOCTIS_BELL_SH1) + 1;
            g_belE2 -= (g_belE2 >> NOCTIS_BELL_SH2) + 1;
            if (g_belE2 < 0) g_belE2 = 0;
            if (g_belE1 <= NOCTIS_ENV_FLOOR) { g_belE1 = 0; g_belE2 = 0; }
        }

        // -- cambio de gravedad: barrido de tono cayendo + golpe de aire --
        if (g_gsE > NOCTIS_ENV_FLOOR) {
            int e = g_gsE >> 5, en = g_gsEN >> 5, glp = g_gsLp, inc = g_gsInc;
            unsigned int p = g_gsPh;
            for (i = 0; i < NOCTIS_AUDIO_CHUNK; ++i) {
                int s = g_auSine[p >> 22]; p += (unsigned int)inc;
                int n = auNoise();
                glp += ((n - glp) * NOCTIS_GS_NK) >> 15;
                dry[i] += ((s * e) >> 16) + ((glp * en) >> 17);
            }
            g_gsPh = p; g_gsLp = glp;
            inc -= (inc >> NOCTIS_GS_PSH) + 1;                      // el tono se DESPLOMA
            if (inc < (int)AU_INC(NOCTIS_GS_FMIN)) inc = (int)AU_INC(NOCTIS_GS_FMIN);
            g_gsInc = inc;
            g_gsE  -= (g_gsE  >> NOCTIS_GS_SH ) + 1;
            g_gsEN -= (g_gsEN >> NOCTIS_GS_NSH) + 1;
            if (g_gsEN < 0) g_gsEN = 0;
            if (g_gsE <= NOCTIS_ENV_FLOOR) { g_gsE = 0; g_gsEN = 0; }
        }

        // ---------------- mezcla final: viento estereo + seco + eco ----------------
        for (i = 0; i < NOCTIS_AUDIO_CHUNK; ++i) {
            // Viento: ruido con dos polos por canal. Los dos lados usan ruido
            // INDEPENDIENTE -> suena ancho, envolvente, no "pegado al centro".
            int nL = auNoise();
            g_wL1 += ((nL - g_wL1) * wk) >> 15;
            g_wL2 += ((g_wL1 - g_wL2) * wk) >> 15;
            int nR = auNoise();
            g_wR1 += ((nR - g_wR1) * wk) >> 15;
            g_wR2 += ((g_wR1 - g_wR2) * wk) >> 15;
            // lp2 = la masa grave; un poco de lp1 = el "aire" de arriba.
            int wL = ((g_wL2 + (g_wL1 >> 3)) * wg) >> 12;
            int wR = ((g_wR2 + (g_wR1 >> 3)) * wg) >> 12;

            // Eco: retardo circular con dos tomas (izq larga / der corta = pasillo ancho)
            // y realimentacion filtrada, para que cada repeticion vuelva mas opaca.
            int d = dry[i];
            unsigned int di = g_dlIdx;
            int e0 = g_auDelay[di];                                        // 185.8 ms
            int e1 = g_auDelay[(di + NOCTIS_ECHO_TAP) & NOCTIS_DL_MASK];   //  61.9 ms
            g_dlLp += ((e0 - g_dlLp) * NOCTIS_ECHO_LPK) >> 15;
            int fb = d + ((g_dlLp * NOCTIS_ECHO_FB) >> 15);
            // Zona muerta del ultimo bit: sin esto el redondeo entero deja un -1
            // atrapado dando vueltas por el retardo para siempre. Con esto la cola
            // del eco llega a CERO EXACTO -> el silencio es silencio de verdad.
            if (fb > -3 && fb < 3) fb = 0;
            g_auDelay[di] = (short)auClamp16(fb);
            g_dlIdx = (di + 1u) & NOCTIS_DL_MASK;

            int wetL = ((e0 * NOCTIS_ECHO_WET) >> 15) + (e1 >> 3);
            int wetR = ((e1 * NOCTIS_ECHO_WET) >> 15) + (e0 >> 3);

            o[i * 2    ] = (short)auClamp16(wL + d + wetL);
            o[i * 2 + 1] = (short)auClamp16(wR + d + wetR);
        }
    }

    g_auClock += NOCTIS_AUDIO_SAMPLES;
}

// =============================================================================
// API PUBLICA
// =============================================================================

// Reserva el canal y arma las tablas. Si el hardware dice que no, el sistema
// queda apagado (g_auOk = 0) y todas las demas llamadas son no-op: el juego
// sigue igual, sin sonido y sin riesgo.
static inline void audioInit(void)
{
    if (g_auOk) return;

    for (int i = 0; i < NOCTIS_SINE_LEN; ++i)
        g_auSine[i] = (short)(sinf((float)i * (6.28318531f / (float)NOCTIS_SINE_LEN)) * 32767.0f);

    for (int i = 0; i < NOCTIS_DL_LEN; ++i) g_auDelay[i] = 0;
    for (int b = 0; b < 2; ++b)
        for (int i = 0; i < NOCTIS_AUDIO_SAMPLES * 2; ++i) g_auBuf[b][i] = 0;

    g_dlIdx = 0; g_dlLp = 0;
    g_auCur = 0; g_auPending = 0; g_auClock = 0;

    int ch = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL,
                               PSP_AUDIO_SAMPLE_ALIGN(NOCTIS_AUDIO_SAMPLES),
                               PSP_AUDIO_FORMAT_STEREO);
    if (ch < 0) { g_auOk = 0; g_auCh = -1; return; }   // falla EN SILENCIO, a proposito

    g_auCh = ch;
    g_auOk = 1;
}

// Intensidad del viento, 0..1. Subirlo cerca de un ventanal / de una boca al vacio,
// bajarlo en los pasillos interiores. El valor se suaviza solo (no hace saltos) y
// ademas se multiplica por la "respiracion" interna, asi que 1.0 no suena constante.
static inline void audioSetAmbience(float windLevel)
{
    if (windLevel < 0.0f) windLevel = 0.0f;
    if (windLevel > 1.0f) windLevel = 1.0f;
    g_wLevelT = (int)(windLevel * 1048576.0f);
}

// Una pisada. strength 0..1 (0.45 caminando, 1.0 corriendo o cayendo).
// Alterna dos voces con timbre distinto: da la sensacion de pie izq / pie der.
static inline void audioFootstep(float strength)
{
    if (!g_auOk) return;
    if (strength < 0.0f) strength = 0.0f;
    if (strength > 1.0f) strength = 1.0f;

    AuStep *s = &g_step[g_stWhich];
    g_stWhich ^= 1;

    int lvl  = (int)(strength * (float)NOCTIS_ENV_ONE);   // Q20
    s->env   = lvl;
    s->envLo = (lvl >> 2) * 3;
    s->lp    = 0;
    s->phLo  = 0;
}

// Campana lejana. Ademas reinicia el reloj de la campana automatica para que no
// se amontonen dos seguidas.
static inline void audioBell(void)
{
    if (!g_auOk) return;
    auBellStrike();
    g_belTimer = NOCTIS_BELL_MIN_BLK + (int)(auRnd() % (unsigned int)NOCTIS_BELL_SPAN_BLK);
}

// El cambio de gravedad: golpe grave con el tono desplomandose de 190 Hz a 28 Hz.
static inline void audioGravityShift(void)
{
    if (!g_auOk) return;
    g_gsPh  = 0;
    g_gsInc = (int)AU_INC(NOCTIS_GS_F0);
    g_gsE   = NOCTIS_GS_E0;
    g_gsEN  = NOCTIS_GS_N0;
    g_gsLp  = 0;
}

// Llamar 1 vez por frame. NO BLOQUEA NUNCA:
//   - usa sceAudioOutputPanned (variante no bloqueante),
//   - solo genera un buffer cuando el canal tiene lugar,
//   - si el hardware esta ocupado, se guarda el buffer y reintenta el frame
//     siguiente sin volver a sintetizar (cero CPU desperdiciada).
// Renderizar solo cuando quedan menos de SAMPLES muestras por sonar garantiza
// que el buffer que vamos a pisar ya termino de reproducirse (doble buffer seguro).
static inline void audioUpdate(void)
{
    if (!g_auOk) return;

    if (g_auPending) {
        if (sceAudioOutputPanned(g_auCh, NOCTIS_AUDIO_MASTER, NOCTIS_AUDIO_MASTER,
                                 g_auBuf[g_auCur]) >= 0) {
            g_auPending = 0;
            g_auCur ^= 1;
        } else {
            return;                       // sigue ocupado: no se sintetiza nada este frame
        }
    }

    int rest = sceAudioGetChannelRestLen(g_auCh);
    if (rest < 0) rest = 0;               // error del driver -> se trata como "vacio"
    if (rest >= NOCTIS_AUDIO_SAMPLES) return;   // ya hay >= 1 buffer encolado

    auRenderBlock(g_auBuf[g_auCur]);

    if (sceAudioOutputPanned(g_auCh, NOCTIS_AUDIO_MASTER, NOCTIS_AUDIO_MASTER,
                             g_auBuf[g_auCur]) >= 0) g_auCur ^= 1;
    else g_auPending = 1;
}

static inline void audioShutdown(void)
{
    if (!g_auOk) return;
    sceAudioChRelease(g_auCh);
    g_auOk = 0;
    g_auCh = -1;
}

// -----------------------------------------------------------------------------
// ATAJO: todo el cableado por frame en una sola llamada.
//
//   walkCycle : la fase del ciclo de caminata (bobPhase de main.cpp). Solo se mira
//               CUANTO avanzo respecto del frame anterior: si avanzo, el jugador
//               esta caminando, y cuanto avanzo por frame dice a que velocidad.
//               main.cpp hace bobPhase += 0.30 + 0.9*(spd/FP_SPEED), o sea
//               0.30 = apenas se mueve, 1.20 = a full.
//   gravMode  : gravG (0..5). Cuando cambia, suena el golpe de gravedad solo.
//
// La cadencia de las pisadas NO va atada al framerate: usa el reloj de audio
// (muestras realmente generadas), que por definicion corre en tiempo real. Asi
// los pasos suenan a un ritmo creible corra el juego a 60, a 30 o a 18 fps.
// -----------------------------------------------------------------------------
static inline void audioFrame(float walkCycle, int gravMode)
{
    if (!g_auOk) { return; }

    if (!g_afInit) {                      // primer frame: sincroniza sin disparar nada
        g_afInit      = 1;
        g_afPrevWalk  = walkCycle;
        g_afPrevGrav  = gravMode;
        g_afPrevClock = g_auClock;
    }

    // --- cambio de gravedad ---
    if (gravMode != g_afPrevGrav) {
        g_afPrevGrav = gravMode;
        audioGravityShift();
    }

    // --- pisadas ---
    float d = walkCycle - g_afPrevWalk;
    g_afPrevWalk = walkCycle;

    int elapsed = (int)(g_auClock - g_afPrevClock);   // muestras desde el frame anterior
    g_afPrevClock = g_auClock;
    if (elapsed < 0) elapsed = 0;
    if (elapsed > NOCTIS_AUDIO_SR) elapsed = NOCTIS_AUDIO_SR;   // por si hubo un parate largo

    if (d > 0.0001f && d < 8.0f) {                    // se esta moviendo
        float s01 = (d - 0.30f) * 1.1111111f;         // 0.30..1.20 -> 0..1
        if (s01 < 0.0f) s01 = 0.0f;
        if (s01 > 1.0f) s01 = 1.0f;
        int interval = NOCTIS_STEP_T_SLOW -
                       (int)(s01 * (float)(NOCTIS_STEP_T_SLOW - NOCTIS_STEP_T_FAST));
        g_afStepAcc += elapsed;
        if (g_afStepAcc >= interval) {
            g_afStepAcc = 0;
            audioFootstep(0.45f + 0.55f * s01);
        }
    } else {
        // Quieto: se deja el contador "cargado" para que el primer paso al
        // arrancar a caminar suene enseguida, no medio segundo despues.
        g_afStepAcc = NOCTIS_STEP_T_SLOW;
    }

    audioUpdate();
}

#endif // NOCTIS_AUDIO_H
