# NOCTIS - Arquitectura del vertical slice

Meta: distrito "Campanario" jugable a ~30 FPS en PSP. Todo en C++17, sin STL
pesada en el loop, sin allocaciones por frame. Se divide en modulos con
fronteras claras para poder programarlos en paralelo (agentes) sin pisarse.

## Capas

    src/
      core/        arranque, loop, tiempo, callbacks, profiler (FPS/ms)
      math/        vec3, mat4, quat; usar VFPU cuando convenga
      gfx/         wrapper de sceGu: init, camara perspectiva, draw de mallas,
                   niebla (sceGuFog), estados; texturas (atlas)
      input/       lectura de pad -> acciones (mapeo del prompt seccion 43)
      world/       geometria del distrito (low-poly), suelo, estructuras,
                   niebla de distancia, silueta lejana de The Cathedral
      player/      estado del jugador, camara 3a persona
      physics/     gravedad (direccion variable), caida, colision simple,
                   escalada basica
      entities/    entidades + object pool (jugador, NPCs, recursos, proyectiles)
      combat/      1 arma melee + 1 a distancia (hitbox/raycast, danio)
      items/       recursos con brillo por proximidad (seccion 12)
      hud/         HP / EN / GRV, minimapa, arma equipada (el mockup)
      audio/       stubs primero (viento, campana) - se llena despues

## Reglas transversales

- Memoria: pools por tipo; nada de new/delete en gameplay. Un arena al boot.
- Rendimiento: frustum culling + niebla que oculta lo lejano; LOD e impostors
  para las estructuras de fondo. Medir siempre con el profiler.
- Datos: la geometria del distrito y stats de armas en tablas/const, no
  hardcodeadas por todos lados.

## Orden de construccion (cada paso compila y se prueba antes del siguiente)

1. core + gfx: GU init, limpiar pantalla, camara perspectiva, un cubo/gridfloor
   girando + contador de FPS.  <- primer hito real de motor
2. input + player + camara 3a persona sobre el gridfloor.
3. world: geometria del distrito Campanario con niebla.
4. physics: gravedad y salto/caida; luego un cambio de direccion gravitacional.
5. escalada basica.
6. hud del mockup.
7. items con brillo; combat (melee + rango); un par de NPCs.
8. silueta lejana de The Cathedral + campana.

## Reparto en agentes (una vez que el pipeline compile)

Modulos con interfaces acordadas primero (headers), luego en paralelo:
- Agente A: math + gfx (renderer y camara)
- Agente B: input + player + physics (movilidad y gravedad)
- Agente C: world (geometria + niebla + impostors)
- Agente D: hud + items + combat

El integrador (yo) define los headers/contratos, junta, compila y prueba.
Nunca se da por bueno codigo que no compilo y ejecuto.
