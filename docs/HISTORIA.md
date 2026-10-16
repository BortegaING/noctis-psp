# NOCTIS: LA TORRE — propuesta de historia

Estado: PROPUESTA (2026-10-16). Benjamin elige el rumbo antes de construir.
Problema que resuelve: hoy el juego se termina en 30-50 minutos (7 peleas en fila, sin nada
entre una y otra). Meta: 3 a 5 horas, sin depender de rellenar con peleas mas largas.

---

## 1. Lo que el juego YA cuenta (y nadie ha unido)

Todo esto ya esta escrito en el codigo; la historia nueva no lo cambia, lo explica.

- **El sueño.** El Serafin te llama "soñador" y dice "Todo lo que ves aquí son tus sueños"
  y "Acepta tu muerte, y descansa". Al morir el juego dice "DESPIERTAS". Baphomet te maldice:
  "Jamás dormirás tranquilo. Nos veremos... abajo."
- **El cielo es un decorado.** El piso 1 tiene muros pintados de cielo que se APAGAN y se
  llenan de ojos: "EL CIELO TE MIRA".
- **Ruedas llenas de ojos.** Los Ofanim (Ezequiel 1) son ruedas cubiertas de ojos. El cielo
  del piso 0 esta lleno de ENGRANAJES colosales que giran. Son lo mismo.
- **Los siete sellos.** Los pisos 4-7 son los 4 primeros sellos del Apocalipsis (cap. 6).
  Faltan tres: el 5º (las almas bajo el altar), el 6º (el sol negro, el cielo que se enrolla
  como un pergamino) y el 7º ("silencio en el cielo como por media hora").
- **12 pisos.** El elevador marca 12; del 8 al 12 dice "???" / "PISO EN CONSTRUCCIÓN".
- **Seis robots sin usar** (src/dialogue.h, 84 lineas) que hablan de una humanidad que ya no
  esta: "Servimos a alguien. No recuerdo a quién." / "Yo apagué las luces. Me lo pidieron." /
  "Mi orden dice: no dejar subir a nadie." / "Puesto 12. Sin relevo desde hace mucho." /
  "Último mensaje humano: sin descifrar." / "Hay una señal debajo. No la entiendo." /
  "Llevo cartas a nombres que no existen." / "Así empezó la otra vez. Con un giro."

## 2. La verdad (recomendada): el ultimo soñador

Hace siglos el mundo se acabo (los siete sellos ya se abrieron, afuera). Los ultimos humanos
se durmieron dentro de la Torre: una maquina de sueños que mueven las ruedas del cielo. Los
robots la cuidan desde entonces, sin saber ya para quien.

Uno a uno, los durmientes murieron soñando. Queda uno: **tu**.

La Torre quiere apagarse. Mientras sigas soñando, sus guardianes no tienen relevo; por eso
todo lo que encuentras te pide lo mismo: que descanses. Los jinetes no son enemigos de
verdad: son el fin del mundo, recordado por tu sueño. Arriba, en el piso 12, esta tu cuerpo
dormido. Abajo, Baphomet espera: el es quien se alimenta de los sueños que nadie despierta.

Cada pieza que ya existe encaja:

| Lo que hay hoy | Lo que significa |
|---|---|
| "DESPIERTAS" al morir | No te dejas morir: el sueño te devuelve |
| El cielo pintado que se apaga | El decorado del sueño; el Anciano lo apago ("Me lo pidieron") |
| Ojos en los muros | Los Ofanim, los ojos de la maquina: te vigilan |
| "Acepta tu muerte, y descansa" | La Torre pidiendo apagarse |
| "Nos veremos... abajo" | El final secreto |
| "Puesto 12. Sin relevo" | El Guardian del piso 12, ultimo obstaculo |
| "Último mensaje humano: sin descifrar" | El hilo de toda la partida (ver 3.1) |

### Alternativas (elige una)
- **B. El moribundo.** No hay fin del mundo: el cazador se esta muriendo (un hospital, una
  batalla) y la Torre es su agonia. Cada piso es una etapa de soltar. Mas intimo, menos lore.
- **C. El portador de la luz.** Baphomet no exagera: el cazador es un angel caido que vuelve a
  subir al cielo, y los jinetes y el Serafin intentan frenarlo. Mas oscuro; el final "bueno"
  es dudoso.

## 3. Como se convierte en horas de juego

### 3.1 El hilo: el ultimo mensaje humano
UNIDAD 44 (robot archivista) vive en el cielo, junto al elevador. Tiene un mensaje humano
corrupto. Cada **memoria** que le traes descifra un trozo y lo muestra en pantalla:
"MENSAJE: 34 %". El mensaje completo es lo ultimo que dejo alguien que te conocio.

### 3.2 Memorias escondidas (3 por piso)
Fragmentos brillantes escondidos en cada piso. Al tomarlos, una tarjeta de texto con un
recuerdo del cazador (su vida antes de dormir). Algunas solo aparecen despues de vencer al
jefe; otras piden las mecanicas: el DASH para cruzar un hueco, CORRER para llegar antes de
que algo se cierre, DETENER EL TIEMPO para pasar entre aspas que giran.
- Pisos 1-7: 21 memorias. Pisos 8-12: 15 mas. Total 36.
- Menu MEMORIAS (pausa) para releerlas.

### 3.3 El cielo como hub: los seis robots
El piso 0 deja de ser un pasillo: cada robot tiene su losa y su papel. Lo que dicen cambia
con cada jefe vencido (volver a hablarles es parte del juego).
- **UNIDAD 44**: el mensaje (3.1).
- **MECANICO** ("Nadie firmó la orden de parar. Sigo."): repara armas con los materiales que
  sueltan los jefes. Desbloquea las 10 armas a distancia y las 10 de cuerpo a cuerpo que ya
  estan en game_data.h y weapons_geo.h y hoy no se usan.
- **MENSAJERO** ("Llevo cartas a nombres que no existen."): te da cartas. Son para los
  durmientes muertos del piso 8: entregarlas es una mision secundaria.
- **ANCIANO** ("Yo apagué las luces."): cuenta el pasado a pedazos. Fue el quien apago el
  cielo del piso 1.
- **TECNICO** ("Hay una señal debajo."): la pista del final secreto.
- **GUARDIAN** ("Mi orden dice: no dejar subir a nadie."): no te deja pasar al piso 8 sin
  suficientes memorias. Es tambien el jefe del piso 12.

### 3.4 Los pisos 8 a 12 (los sellos que faltan)
| Piso | Nombre | Que es | Como se juega |
|---|---|---|---|
| 8 | EL ALTAR | 5º sello: las almas bajo el altar | Sin jefe. Capsulas de durmientes muertos; leer sus nombres, entregar las cartas del Mensajero. Exploracion y relato |
| 9 | EL SOL NEGRO | 6º sello: terremoto, luna de sangre, estrellas que caen, el cielo se enrolla | Huida: las losas se derrumban detras, caen estrellas. Correr y dash |
| 10 | EL SILENCIO | 7º sello: "silencio en el cielo" | Piso sin sonido. Los ojos vuelven: si corres o disparas te ven. Sigilo |
| 11 | LA CATEDRAL | Las siete trompetas; la catedral viva de DESIGN.md | Jefe: la catedral que te persigue y te vigila con campanas |
| 12 | EL PUESTO 12 | El Guardian y tu cuerpo dormido | Pelea con el Guardian o, con todas las memorias, relevarlo de su puesto. Despues: el final |

### 3.5 Finales
- **DESCANSAR.** Aceptas lo que pidio el Serafin. La Torre se apaga; los robots por fin
  descansan. Final triste y en paz.
- **DESPERTAR.** Abres el septimo sello. Sales: el cielo de verdad, no pintado. El mensaje
  de UNIDAD 44 completo se lee entero. Final con esperanza.
- **ABAJO (secreto).** Con la maldicion de Baphomet y la señal del Tecnico se abre el piso
  -1. Baphomet: "Le dije que nos veríamos." Pelea final y la verdad de quien construyo la
  Torre.

La eleccion del Serafin (rendirse / pensar en Dios) y el cuestionario del perfume de
Baphomet empiezan a tener consecuencias: dialogos distintos y que finales se abren.

## 4. Cuantas horas (estimacion)
| Parte | Tiempo |
|---|---|
| Hoy (pisos 0-7) | 30-50 min |
| Memorias + hub + robots en pisos 0-7 | +45-60 min |
| Armas del Mecanico (materiales, probar armas) | +20-30 min |
| Pisos 8-12 | +1.5-2 h |
| Finales distintos (2a y 3a vuelta) | +1 h |
| **Total** | **3.5-5 h** |

## 5. Orden de construccion
1. **Fase 1, "La voz de la Torre"** (una sesion): prologo al JUGAR (4 tarjetas), UNIDAD 44 en
   el cielo con dialogos que cambian segun el progreso, 2-3 memorias en los pisos 1-7, menu
   MEMORIAS. Ademas arreglar la descripcion del XMB de los pisos 3-7 (todos dicen "Piso 2").
2. **Fase 2, "El taller"**: Mecanico, materiales de los jefes, armas desbloqueables.
3. **Fase 3**: pisos 8, 9 y 10 + Mensajero/cartas + Guardian.
4. **Fase 4**: pisos 11 y 12, los tres finales, el piso -1.

## Notas tecnicas
- Guardado: `eventFlags` usa los bits 0-12 y 31. Las memorias (36 bits) caben en
  `invResLo`/`invResHi` del NoctisSave, que en el mundo del cielo no se usan (eran los
  recursos del sector viejo).
- Todo es texto: tarjetas y dialogos son baratos en la PSP. Lo caro son los pisos 8-12
  (escenario + jefe, ~1000 lineas cada uno, como los jinetes).
- Robots: los modelos y dialogos estan en src/robots.h y src/dialogue.h (sector viejo).
- Regla de los robots que se mantiene: NINGUNO explica del todo que le paso a la humanidad.
  La verdad la cuentan las memorias y el mensaje, a pedazos.
