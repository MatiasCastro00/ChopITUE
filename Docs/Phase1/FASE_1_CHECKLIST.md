# Fase 1 — movimiento, cámara y mapa de prueba

## Resultado esperado

Al abrir cualquiera de los mapas de proyecto y pulsar Play se ve un claro forestal con cabaña. El personaje naranja puede recorrerlo con `WASD` o el stick izquierdo. La cámara permanece cenital y estable. `E` o el botón inferior del gamepad reserva la interacción para fases posteriores.

## Implementación

- [x] Gameplay Framework propio: GameMode, GameState, PlayerController, PlayerState y Character.
- [x] Personaje sin Actor Tick, con `CharacterMovement` y orientación hacia el movimiento.
- [x] Cámara de perspectiva casi ortográfica fija.
- [x] Entrada normalizada mediante Enhanced Input.
- [x] Teclado y gamepad desde el mismo mapping context.
- [x] `UChopItInteractionComponent` como seam sin reglas prematuras.
- [x] `BP_ChopItCharacter` generado como composición Blueprint mínima.
- [x] `L_Dev_Sandbox` con suelo, cabaña, claro, árboles, límites, iluminación, PlayerStart y NavMesh bounds.
- [x] `L_Startup` visible y jugable para evitar el bootstrap negro.
- [x] Sin lógica en Level Blueprint.

## Gate automatizado

- [x] Development Editor compila.
- [x] Bootstrap Fase 1 termina correctamente y es repetible.
- [x] `ChopIt.Smoke` y `ChopIt.Phase1` pasan (4/4).
- [x] Development Game compila.
- [x] Cook Win64 termina sin errores.

Verificado el 27 de agosto de 2026: `VerifyPhase1.ps1` finalizó con 0 errores y 0 advertencias.

## Comprobación manual

- [x] `WASD` mueve en los cuatro ejes relativos a cámara (`W` avanza hacia donde mira la cámara).
- [ ] El stick izquierdo mueve y respeta dead zone.
- [ ] Una diagonal no supera la velocidad cardinal.
- [ ] El personaje no atraviesa la cabaña, troncos ni límites.
- [ ] La cámara no rota ni cambia bruscamente ante obstáculos.
- [ ] La cabaña y el borde del claro permanecen legibles a 1080p.
- [x] Play en `L_Startup` y `L_Dev_Sandbox` no muestra pantalla negra (render offscreen verificado).
- [ ] El viewport mantiene 60 FPS en el blockout del hardware de desarrollo.

Movimiento relativo a cámara validado manualmente el 27 de agosto de 2026.

## Ejecución

Con Unreal Editor cerrado:

```powershell
powershell -ExecutionPolicy Bypass -File .\Build\CI\VerifyPhase1.ps1
```
