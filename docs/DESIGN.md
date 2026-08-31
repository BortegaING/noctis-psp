# NOCTIS - Directiva de diseno (pilares)

Resumen estructurado de la vision. La prioridad absoluta, en orden:
**Atmosfera + Escala + Movilidad + Exploracion + Gameplay + Identidad visual + Rendimiento.**

## Vision
Action RPG de exploracion en 3a persona dentro de una megaestructura vertical
gigante. Sensacion dominante: BLAME! + gotico oscuro + horror cosmico +
gravedad + verticalidad + escala monumental. Identidad propia, no un clon.

## Mundo: THE ABYSS
- Sin superficie conocida; profundidades desconocidas; un vacio enorme.
- El vacio es protagonista visual: restos de edificios, torres suspendidas,
  luces lejanas, puentes rotos, siluetas gigantes que se pierden en oscuridad.
- Ilusion de profundidad infinita con niebla y distancia.

## Atmosfera
Soledad, misterio, vacio, grandeza, inquietud, decadencia, aislamiento,
fascinacion. Poca musica; usar silencio, niebla, ecos, viento, distancias,
luces lejanas. Momentos de pura contemplacion.

## Arquitectura y escala
Catedrales, torres, agujas, campanarios, contrafuertes, puentes, pozos
verticales, fabricas, ruinas, pasarelas. Deformada hasta tener identidad
propia. Escala extrema: estructuras de cientos/miles de metros que desaparecen
en niebla/nubes/oscuridad.

## Personajes
NPCs = robots/androides (la humanidad desaparecio hace muchisimo). Disenios
variados, siluetas reconocibles, estados de conservacion distintos, roles
(comerciantes, mecanicos, exploradores, guardianes, mensajeros, tecnicos,
investigadores). Rutinas, reacciones a eventos y a The Cathedral, personalidades
diversas. Nadie conoce toda la verdad.

## Recursos y crafting
Materiales ocultos que brillan sutil al acercarse (sonido + indicacion). Tipos
con nomenclatura propia (nucleo gravitacional raro, fragmento de aleacion
comun, cristal oscuro energetico, circuito antiguo, fragmento de nucleo muy
raro, metal de abismo para armas avanzadas). Crafting simple pero con
profundidad: fabricar/mejorar armas, desbloquear habilidades, municion/energia.

## Armas
- 10 a distancia (pistola laser, rifle gravitacional, carabina, canion de
  particulas, pistola de pulsos, rifle de precision, lanzador, fragmentacion,
  experimental, ancestral) diferenciadas por alcance/danio/cadencia/precision/
  energia/recarga/proyectil/efecto.
- 10 melee (katana, katana pesada, espada energetica, hoja gravitacional,
  industrial, lanza, guadania, mandoble, experimental, ancestral) diferenciadas
  por velocidad/alcance/danio/peso/combos/aereos/gravitacionales.
- Mejora con materiales + dinero + componentes raros. Nada excesivamente
  numerico: la habilidad del jugador pesa mas que las stats.

## Economia
Enemigos y gargolas dan dinero/materiales/componentes. Dinero para comprar
armas, mejoras, materiales, reparaciones, informacion.

## Enemigos
Familias: Gargoyles, Hollows, Constructs, Aberrations, Guardians, Titans. Cada
una con disenio, IA, animaciones, sonido, debilidades y comportamiento.
Gargolas biomecanicas: vuelan, trepan, emboscan, persiguen; variantes
(exploradora, rapida, pesada, voladora, francotiradora, defensiva, enjambre,
elite).

## Gravedad y movilidad (nucleo del gameplay)
Cambiar direccion gravitacional, caida libre, correr por paredes/techos,
ataques gravitacionales, exploracion aerea, combate 3D. Escalada integrada con
gravedad (pared -> suelo). Movimiento muy responsivo: velocidad, precision,
fluidez, control. Moverse por la ciudad debe ser divertido sin combatir.
Camara 100% en 3a persona en todo momento.

## The Cathedral (jefe/estructura)
No es un monstruo: es una megaestructura gotica viviente (catedral + fortaleza
+ maquina + organismo + ciudad). Enorme, ocupa una zona. Primer encuentro no se
muestra completa: catedral distante, una torre se mueve, suena una campana, se
desplaza, los robots reaccionan, las gargolas huyen. Ese primer encuentro es
una persecucion jugable (no se puede derrotar): escapar usando gravedad,
escalada, entorno y armas. Despues se puede entrar: dungeon vertical gigante.
Observa al jugador con eventos sutiles (campanas, ventanas que se iluminan,
estructuras que se orientan).

## Lugares imposibles
Pocas zonas donde las reglas se rompen: interior mayor que exterior, torres
imposibles, calles que rotan, ascensores absurdamente profundos, anomalias
gravitacionales. Momentos especiales.

## Descubrimiento
Mapa SIN iconos excesivos. El jugador lee el mundo por luces, sonidos,
siluetas, estructuras, NPCs y curiosidad. Recompensas por explorar; los lugares
peligrosos dan mejores premios.

## Controles PSP
- Stick: movimiento
- X: salto / accion
- Cuadrado: ataque melee
- Triangulo: interaccion
- Circulo: esquiva
- L: control gravitacional
- R: objetivo / camara
- L+R: cambio gravitacional rapido
- Start: menu | Select: mapa

## Rendimiento (objetivo 30 FPS estables)
LOD, occlusion/frustum culling, streaming, impostors, lightmaps, texture
atlases, batching, object pooling. Render por distancia: cercano alta calidad,
intermedio LOD, lejano geometria simple, muy lejano impostors/siluetas. The
Cathedral modular. IA por distancia (completa cerca, simple lejos, minima
fuera). Memory manager + pools; evitar allocaciones en gameplay y memory leaks.

## Guardado
3 ranuras manuales + 1 auto + 1 recuperacion, con checksums y backups. Guarda
posicion, distrito, gravedad, inventario, armas, mejoras, dinero, materiales,
misiones, eventos, progreso de The Cathedral.

## Direccion visual
Personajes estilizados/anime, expresivos; mundo oscuro, monumental, brutal.
Contraste intencional. Referencia tecnica PSP: Black Rock Shooter: The Game.

## Estandar
Iterativo (disenar, implementar, compilar, ejecutar, probar, perfilar, corregir,
optimizar, comparar). Nunca asumir que funciona porque compila. No sacrificar la
identidad para ahorrar recursos: convertir el limite del hardware en direccion
artistica. Estandar final: EXCELENTE dentro de lo que la PSP permite.
