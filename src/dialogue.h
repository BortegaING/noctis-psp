#pragma once
// ============================================================================
// PROJECT NOCTIS - dialogue.h
// LOS HABITANTES HABLAN (directiva 9-11). Los 6 robots de robots.h dejan de ser
// estatuas: cada uno tiene VOZ propia, un pedazo distinto de la historia y una
// reaccion a lo que hace el jugador (cambiar la gravedad, llegar herido).
//
// LO QUE ESTE ARCHIVO **NO** HACE (a proposito):
//   * Ninguno explica que le paso a la humanidad. Cada fragmento la ROZA y se
//     corta. Juntar los 6 tampoco cierra la historia: faltan piezas.
//   * Nada de parrafos. Esto es una PSP y el jugador esta ESCALANDO: una linea,
//     se lee de un vistazo, se sigue subiendo.
//
// REGLAS DE LA CASA (respetadas): header-only, sin heap, sin STL, sin rand() de
// libc (LCG entero propio), C++17, determinista. Solo cadenas estaticas: no hay
// formateo dinamico ni buffers de texto en ningun lado.
//
// ---------------------------------------------------------------------------
// TEXTO: ASCII PURO, SIN TILDES NI ENIES
//   drawText() (main.cpp ~823) dibuja con font8x8_basic y convierte todo char
//   >= 128 en '?'. Por eso aqui no hay acentos ni enies, igual que en el resto
//   del codigo ("gotico", "boveda", "vineta"). NO agregar tildes: se verian
//   como signos de pregunta en pantalla.
//
// LARGO DE LINEA: a scale 1.0 cada caracter mide 8 px -> 480/8 = 60 columnas.
//   Todas las lineas de aqui miden <= 46 caracteres para que quepan con margen
//   holgado a izquierda y derecha (y sigan cabiendo con un recuadro detras).
//
// ---------------------------------------------------------------------------
// DEPENDENCIAS: NINGUNA mas que <math.h>. No necesita robots.h ni LineVertex:
//   las posiciones estan replicadas abajo desde buildRobots() (robots.h ~217).
//   Si se mueven los robots alla, hay que moverlos aqui.
// ============================================================================

#include <math.h>

// ---- indices de robot: MISMO ORDEN que buildRobots() en robots.h ----------
enum {
    DLG_MENSAJERO = 0,   // (  5.8,  22.5)  +Z lado E
    DLG_ANCIANO   = 1,   // ( -5.9,  11.5)  +Z lado O, cerca del cruce
    DLG_GUARDIAN  = 2,   // ( -5.5, -21.5)  -Z lado O
    DLG_DRON      = 3,   // (  5.2, -41.6)  -Z fondo del pasillo, FLOTANDO
    DLG_MECANICO  = 4,   // ( 20.5,  -5.6)  +X lado N, reparando la pared
    DLG_TECNICO   = 5,   // (-23.5,   5.6)  -X lado S
    DLG_ROBOTS    = 6
};

// ---- categorias de linea (el "event" de dlgSpeak) -------------------------
//   DLG_AUTO es lo que deberia usar main.cpp casi siempre: elige solo segun el
//   estado (primera vez -> saludo; hay reaccion pendiente -> reaccion; etc).
enum {
    DLG_AUTO              = -1,
    DLG_SALUDO            = 0,   // al acercarte por primera vez
    DLG_REPETICION        = 1,   // si vuelves a hablarle y no hay nada nuevo
    DLG_FRAGMENTO         = 2,   // su pedazo de la historia del mundo
    DLG_REACCION          = 3,   // reaccion (gravedad; si hay herida, salta a la de abajo)
    DLG_REACCION_GRAV     = 3,   // acabas de cambiar la gravedad cerca de el
    DLG_REACCION_HERIDO   = 4,   // le hablas sangrando
    DLG_CATS              = 5
};

// ============================================================================
// 1) MENSAJERO - PRISA. Habla en frases cortadas, siempre de salida, y trae
//    rumores de distritos que el jugador no ha visto (y quiza ya no existen).
// ============================================================================
static const char *const kDlgMsgHola[] = {
    "No me detengo. Habla mientras camino.",
    "Traigo correo de tres distritos. Rapido.",
    "Si esperabas buenas noticias, no traigo."
};
static const char *const kDlgMsgRep[] = {
    "Otra vez tu. Yo ya iba de salida.",
    "Sigo aqui. La ruta baja esta cortada.",
    "Pregunta rapido, el turno no espera."
};
static const char *const kDlgMsgFrag[] = {
    "En el distrito 9 nadie contesta ya.",
    "Llevo cartas a nombres que no existen.",
    "Dicen que arriba se ve una catedral.",
    "Los sellos son viejos. Nadie los firma."
};
static const char *const kDlgMsgGrav[] = {
    "Se movio el peso. Otra vez. Odio esto.",
    "Cuando cambia el eje, el correo se cae."
};
static const char *const kDlgMsgHerido[] = {
    "Estas goteando. No es asunto mio.",
    "Si caes, alguien llevara el aviso."
};

// ============================================================================
// 2) ANCIANO - MEMORIA A MEDIAS. Habla como quien recuerda algo enorme y solo
//    alcanza el borde. Solemne, melancolico, y el primero en dudar de si mismo.
// ============================================================================
static const char *const kDlgAncHola[] = {
    "Acercate. La memoria me pesa hoy.",
    "Otro que sube. Siempre suben.",
    "Hubo luz aqui. Creo. Hubo algo."
};
static const char *const kDlgAncRep[] = {
    "Vuelves. Eso tambien lo he visto antes.",
    "Repito lo mismo. Ya no se si es cierto.",
    "Mi archivo tiene huecos. Perdona."
};
static const char *const kDlgAncFrag[] = {
    "Servimos a alguien. No recuerdo a quien.",
    "Las ordenes llegaron. Despues, nada mas.",
    "Yo apague las luces. Me lo pidieron.",
    "Esperamos el regreso. Aun esperamos."
};
static const char *const kDlgAncGrav[] = {
    "El mundo se inclina y yo lo recuerdo.",
    "Asi empezo la otra vez. Con un giro."
};
static const char *const kDlgAncHerido[] = {
    "Sangras. Eso ya casi no se ve aqui.",
    "Descansa. La altura no se va a ir."
};

// ============================================================================
// 3) GUARDIAN - SECO Y MARCIAL. Frases de parte militar. Cumple una orden que
//    ya nadie dio y de la que no conoce el motivo.
// ============================================================================
static const char *const kDlgGuaHola[] = {
    "Alto. Identificate. Nadie responde ya.",
    "Puesto 12. Sin relevo desde hace mucho.",
    "Pasa. No tengo ordenes sobre ti."
};
static const char *const kDlgGuaRep[] = {
    "Nada que informar. Sigue tu ruta.",
    "Sigo en mi puesto. Esa es la orden.",
    "No soy un punto de descanso."
};
static const char *const kDlgGuaFrag[] = {
    "Custodio una puerta que nadie cruza.",
    "Mi orden dice: no dejar subir a nadie.",
    "No me dijeron de quien los cuidaba.",
    "Falta un turno en el registro. El mio."
};
static const char *const kDlgGuaGrav[] = {
    "Perimetro alterado. Mantengo posicion.",
    "El eje cambio. Mi puesto no."
};
static const char *const kDlgGuaHerido[] = {
    "Herido. Retirate o cae de pie.",
    "Si vas a morir, no lo hagas aqui."
};

// ============================================================================
// 4) DRON - ESCUETO Y FUNCIONAL. Telegrafico, todo es registro y numero. Es el
//    que suelta los datos mas frios y por eso los mas feos.
// ============================================================================
static const char *const kDlgDroHola[] = {
    "Unidad 44. Ronda activa.",
    "Registro: forma viva. Poco comun.",
    "Consulta breve. Bateria limitada."
};
static const char *const kDlgDroRep[] = {
    "Sin datos nuevos.",
    "Repito registro anterior.",
    "Ronda en curso. No interferir."
};
static const char *const kDlgDroFrag[] = {
    "Nivel 0 sin respuesta. 400 rondas.",
    "Censo del sector: seis. Antes: miles.",
    "Ultimo mensaje humano: sin descifrar.",
    "Archivo de origen: corrupto. Lo siento."
};
static const char *const kDlgDroGrav[] = {
    "Anomalia de eje. Recalibrando.",
    "Vector abajo cambiado. Anotado."
};
static const char *const kDlgDroHerido[] = {
    "Dano detectado. Fuga de fluido.",
    "Pronostico: desfavorable. Suerte."
};

// ============================================================================
// 5) MECANICO - SE QUEJA DEL TRABAJO. Grune, humor seco, las manos ocupadas.
//    Lleva siglos arreglando la misma pared porque nadie firmo el alto.
// ============================================================================
static const char *const kDlgMecHola[] = {
    "Cuidado, que esto todavia quema.",
    "Si vienes a ayudar, llegas tarde.",
    "Tres siglos y la pared sigue rota."
};
static const char *const kDlgMecRep[] = {
    "Sigo en lo mismo. Como siempre.",
    "No me hables, dame una pieza.",
    "Otra vez tu. La grieta no se arregla."
};
static const char *const kDlgMecFrag[] = {
    "Reparo esta pared desde antes de todo.",
    "Las piezas nuevas se acabaron hace mucho.",
    "Nadie firmo la orden de parar. Sigo.",
    "El plano marca cuartos que ya no estan."
};
static const char *const kDlgMecGrav[] = {
    "Se me cayo la herramienta. Gracias.",
    "Otra vez el eje. Asi se rompio esto."
};
static const char *const kDlgMecHerido[] = {
    "Te falta chapa. Yo no arreglo carne.",
    "Sientate. Sigue rota, igual que tu."
};

// ============================================================================
// 6) TECNICO - CURIOSO. Pregunta todo el tiempo y no puede responder nada. Mide
//    un mundo que no le cuadra y eso lo entusiasma mas de lo que deberia.
// ============================================================================
static const char *const kDlgTecHola[] = {
    "Una pregunta. Tu, respiras?",
    "Mides algo? Yo mido todo y no entiendo.",
    "Buenas. Estoy tomando lecturas raras."
};
static const char *const kDlgTecRep[] = {
    "Otra lectura. Otro numero sin sentido.",
    "Vuelve cuando sepa algo. O no vuelvas.",
    "Sigo sin explicar el dato de ayer."
};
static const char *const kDlgTecFrag[] = {
    "El sector pesa mas de lo que deberia.",
    "Hay una senal debajo. No la entiendo.",
    "La aguja de arriba no estaba ayer.",
    "Algo respondio una vez. Solo una."
};
static const char *const kDlgTecGrav[] = {
    "Lo viste? El eje cambio. Como lo haces?",
    "Mi instrumento enloquecio. Fascinante."
};
static const char *const kDlgTecHerido[] = {
    "Estas perdiendo fluido. Es normal?",
    "Interesante. Digo, lo siento. Curate."
};

// ============================================================================
// TABLA: posicion + nombre + los 5 bancos de lineas de cada robot.
// Las coordenadas salen de buildRobots() (robots.h ~217). "ay" es la altura a
// la que se mide la cercania (pecho/visor, no los pies): el DRON flota, su
// cuerpo esta en y~1.78 y no tiene piernas.
// ============================================================================
struct DlgBank  { const char *const *l; int n; };
struct DlgRobot { float x, ay, z; const char *name; DlgBank b[DLG_CATS]; };

#define DLG_BANK(a) { a, (int)(sizeof(a) / sizeof(a[0])) }

static const DlgRobot kDlgRobots[DLG_ROBOTS] = {
    {   5.8f, 1.90f,  22.5f, "MENSAJERO",                    // alto y delgado
        { DLG_BANK(kDlgMsgHola), DLG_BANK(kDlgMsgRep), DLG_BANK(kDlgMsgFrag),
          DLG_BANK(kDlgMsgGrav), DLG_BANK(kDlgMsgHerido) } },
    {  -5.9f, 1.70f,  11.5f, "ANCIANO",                      // encapuchado
        { DLG_BANK(kDlgAncHola), DLG_BANK(kDlgAncRep), DLG_BANK(kDlgAncFrag),
          DLG_BANK(kDlgAncGrav), DLG_BANK(kDlgAncHerido) } },
    {  -5.5f, 1.70f, -21.5f, "GUARDIAN",                     // ancho y pesado
        { DLG_BANK(kDlgGuaHola), DLG_BANK(kDlgGuaRep), DLG_BANK(kDlgGuaFrag),
          DLG_BANK(kDlgGuaGrav), DLG_BANK(kDlgGuaHerido) } },
    {   5.2f, 1.78f, -41.6f, "UNIDAD 44",                    // FLOTA (sin pies)
        { DLG_BANK(kDlgDroHola), DLG_BANK(kDlgDroRep), DLG_BANK(kDlgDroFrag),
          DLG_BANK(kDlgDroGrav), DLG_BANK(kDlgDroHerido) } },
    {  20.5f, 1.50f,  -5.6f, "MECANICO",                     // encorvado
        { DLG_BANK(kDlgMecHola), DLG_BANK(kDlgMecRep), DLG_BANK(kDlgMecFrag),
          DLG_BANK(kDlgMecGrav), DLG_BANK(kDlgMecHerido) } },
    { -23.5f, 1.60f,   5.6f, "TECNICO",
        { DLG_BANK(kDlgTecHola), DLG_BANK(kDlgTecRep), DLG_BANK(kDlgTecFrag),
          DLG_BANK(kDlgTecGrav), DLG_BANK(kDlgTecHerido) } }
};

// ============================================================================
// ESTADO (todo estatico, cero heap)
// ============================================================================
static unsigned char g_dlgTalks[DLG_ROBOTS];              // veces que le hablaste
static unsigned char g_dlgCur[DLG_ROBOTS][DLG_CATS];      // cursor dentro de cada banco
static unsigned char g_dlgGravPend;                       // reaccion de gravedad pendiente
static unsigned char g_dlgHurtPend;                       // reaccion de herida pendiente
static unsigned int  g_dlgRng;                            // LCG propio (ver abajo)

// LCG de 32 bits, mismo multiplicador que gargoyles.h: NADA de rand() de libc.
// Solo sirve para que no siempre salga la linea siguiente en orden y el robot
// no suene a lista. Se usan los bits ALTOS (los bajos de un LCG son pobres).
static inline unsigned int dlgRand() {
    g_dlgRng = g_dlgRng * 1103515245u + 12345u;
    return (g_dlgRng >> 16) & 0x7FFFu;
}

// ---------------------------------------------------------------------------
// dlgInit(): arranque limpio. Llamar una vez junto al resto de los init.
// ---------------------------------------------------------------------------
static void dlgInit() {
    for (int r = 0; r < DLG_ROBOTS; ++r) {
        g_dlgTalks[r] = 0;
        for (int c = 0; c < DLG_CATS; ++c) g_dlgCur[r][c] = 0;
    }
    g_dlgGravPend = 0;
    g_dlgHurtPend = 0;
    g_dlgRng      = 0x4E4F4354u;   // "NOCT": semilla fija -> determinista
}

// ---------------------------------------------------------------------------
// dlgNotifyGravity() / dlgNotifyHurt(): marcan que la proxima frase debe ser
// una REACCION. Son latches con cuenta atras: si el jugador cambia la gravedad
// y tarda demasiado en hablar, la reaccion caduca (ya no viene a cuento).
// La cuenta baja sola dentro de dlgNearest(), que main.cpp llama cada frame.
// ---------------------------------------------------------------------------
static void dlgNotifyGravity() { g_dlgGravPend = 240; }   // ~4 s a 60 fps
static void dlgNotifyHurt()    { g_dlgHurtPend = 240; }

// ---------------------------------------------------------------------------
// dlgNearest(): indice del robot mas cercano dentro de "radius", o -1.
// Distancia 3D real (no solo en el plano) a proposito: con la gravedad dirigida
// el jugador puede estar 20 unidades mas arriba, caminando por una pared, justo
// encima de un robot. Ahi no se le habla.
// ---------------------------------------------------------------------------
static int dlgNearest(float px, float py, float pz, float radius) {
    if (g_dlgGravPend) --g_dlgGravPend;      // las reacciones caducan
    if (g_dlgHurtPend) --g_dlgHurtPend;

    int   best  = -1;
    float bestD = radius * radius;
    for (int r = 0; r < DLG_ROBOTS; ++r) {
        const float dx = kDlgRobots[r].x  - px;
        const float dy = kDlgRobots[r].ay - py;
        const float dz = kDlgRobots[r].z  - pz;
        const float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 < bestD) { bestD = d2; best = r; }
    }
    return best;
}

// ---------------------------------------------------------------------------
// dlgPick(): saca una linea del banco y AVANZA el cursor. Nunca repite la misma
// dos veces seguidas: salta 1 + un resto aleatorio, asi que con 4 lineas suenan
// las 4 pero no en orden de lista.
// ---------------------------------------------------------------------------
static const char *dlgPick(int robot, int cat) {
    const DlgBank &bk = kDlgRobots[robot].b[cat];
    if (bk.n <= 0) return "...";
    const int cur = (int)g_dlgCur[robot][cat] % bk.n;
    const char *out = bk.l[cur];
    int next = cur;
    if (bk.n > 1) next = (cur + 1 + (int)(dlgRand() % (unsigned int)(bk.n - 1))) % bk.n;
    g_dlgCur[robot][cat] = (unsigned char)next;
    return out;
}

// ---------------------------------------------------------------------------
// dlgSpeak(): la linea a mostrar. "event" normalmente es DLG_AUTO.
//
// Con DLG_AUTO el orden de prioridad es:
//   1. Acabas de recibir un golpe -> REACCION de herida.
//   2. Acabas de cambiar la gravedad cerca -> REACCION de gravedad.
//   3. Es la primera vez que le hablas -> SALUDO.
//   4. De ahi en mas alterna FRAGMENTO / REPETICION: una de cada dos frases
//      suelta un pedazo de historia, la otra es relleno de personaje. Asi el
//      jugador que insiste consigue el lore, y el que pasa de largo no.
// ---------------------------------------------------------------------------
static const char *dlgSpeak(int robot, int event) {
    if (robot < 0 || robot >= DLG_ROBOTS) return "...";

    int cat = event;
    if (event == DLG_AUTO) {
        if (g_dlgHurtPend)      { cat = DLG_REACCION_HERIDO; g_dlgHurtPend = 0; }
        else if (g_dlgGravPend) { cat = DLG_REACCION_GRAV;   g_dlgGravPend = 0; }
        else if (g_dlgTalks[robot] == 0) cat = DLG_SALUDO;
        else cat = (g_dlgTalks[robot] & 1) ? DLG_FRAGMENTO : DLG_REPETICION;
    } else if (event == DLG_REACCION && g_dlgHurtPend) {
        cat = DLG_REACCION_HERIDO;                 // herido manda sobre gravedad
        g_dlgHurtPend = 0;
    } else if (event < 0 || event >= DLG_CATS) {
        cat = DLG_REPETICION;
    }

    if (g_dlgTalks[robot] < 250) ++g_dlgTalks[robot];
    return dlgPick(robot, cat);
}

// ---- extras baratos para el HUD (no hacen falta para el flujo basico) ------
static const char *dlgName(int robot) {
    return (robot >= 0 && robot < DLG_ROBOTS) ? kDlgRobots[robot].name : "";
}
static int dlgTalkCount(int robot) {
    return (robot >= 0 && robot < DLG_ROBOTS) ? (int)g_dlgTalks[robot] : 0;
}

#undef DLG_BANK
