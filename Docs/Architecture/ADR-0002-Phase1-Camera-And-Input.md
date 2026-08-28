# ADR-0002: cámara y entrada de la Fase 1

- **Estado:** aceptado
- **Fecha:** 2026-08-26
- **Decisores:** equipo de ChopIt

## Contexto

La locomoción debe mantener legibles al jugador, el hub y la futura densidad de enemigos. La cámara no puede rotar accidentalmente ni convertir la dirección del movimiento al cambiar el control. La configuración debe funcionar con teclado y gamepad desde el primer slice.

## Decisión

Se adopta una cámara de perspectiva casi ortográfica como baseline:

- FOV de 35 grados, brazo de 2.300 cm e inclinación fija de -58 grados.
- Rotación absoluta del brazo, sin control de cámara por mouse o stick.
- Movimiento relativo a la cámara fija: `W` siempre avanza hacia la dirección horizontal que muestra la cámara, con los vectores forward/right proyectados sobre el plano XY.
- Entrada Axis2D normalizada antes de alimentar `CharacterMovement`.
- `WASD` y stick izquierdo para movimiento; `E` y botón inferior del gamepad para interacción.
- Cámara con lag suave y sin prueba de colisión para evitar zoom brusco bajo futuras copas de árboles.

La alternativa ortográfica real se descarta para el baseline porque la perspectiva baja conserva profundidad, sombras, escala visual y compatibilidad con VFX con una lectura cenital equivalente. Se puede reabrir la decisión si capturas del vertical slice demuestran pérdida de legibilidad.

## Consecuencias

- La composición visual puede asumir un encuadre estable.
- El diseño de obstáculos no puede depender de rotar la cámara.
- Las señales críticas deberán leerse desde el ángulo fijado.
- Si la cámara fija cambia de orientación durante el desarrollo, los controles continúan alineados con lo que ve el jugador.
- El jugador no gana velocidad al combinar dos ejes.

## Verificación

- `ChopIt.Phase1.GameplayFramework` verifica clases, cámara y normalización diagonal.
- `ChopIt.Phase1.InputAssets` verifica acciones y mapeos de teclado/gamepad.
- `ChopIt.Phase1.SandboxMap` verifica GameMode, PlayerStart y geometría de blockout.
- `Build/CI/VerifyPhase1.ps1` ejecuta build Editor/Game, generación idempotente, tests y cook Win64.
