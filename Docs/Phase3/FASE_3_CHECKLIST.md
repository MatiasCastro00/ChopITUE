# Fase 3 — árboles, caída, troncos y recolección

## Objetivo

Convertir el combate contra árboles en un bucle jugable de tala: el árbol cae con física una sola vez, se asienta o agota su timeout, genera madera exactamente una vez y el jugador la recoge sin que los troncos bloqueen movimiento o navegación.

## Clases y assets

- `AChopItTree`: estados `Standing`, `Falling`, `Settled`, `Harvested` y recompensa única.
- `AChopItLogPickup`: unidades de madera, overlap no bloqueante y magnetismo mediante timer.
- `UChopItWoodCargoComponent`: capacidad y transferencias enteras con remanente.
- `UChopItForestRegistrySubsystem`: registro débil de árboles y pickups.
- `BP_Tree_Basic`, `BP_LogPickup` y `L_Test_Harvest` generados por bootstrap.

## Dependencias

- Daño, salud y targeting de Fase 2.
- `CharacterMovement` y colisiones de Fase 1.

## Gate automatizado

- [x] Development Editor y Game compilan.
- [x] Bootstrap Fase 3 es repetible.
- [x] Suites Smoke, Phase1, Phase2 y Phase3 pasan: 11/11, sin warnings.
- [x] Cook Win64 termina sin errores ni warnings.

## Comprobación manual

- [x] El cuarto golpe inicia una única caída visible.
- [x] El árbol colisiona con el suelo sin atrapar al jugador.
- [x] Al asentarse genera una sola recompensa de madera.
- [x] El tronco no bloquea al jugador ni el NavMesh.
- [x] El tronco se magnetiza y aumenta la carga visible.
- [x] Con carga llena, el remanente permanece en el mundo.

## Criterios de aceptación

- [x] Cada árbol cambia de estado de forma válida y recompensa una sola vez.
- [x] No existe `Tick` por árbol o pickup.
- [x] Madera y capacidad usan enteros y nunca se pierden unidades.
- [x] Registros no conservan referencias inválidas.
- [x] Resultado jugable: talar, ver caer y recoger madera.

## Evidencia automatizada

- `Build/CI/VerifyPhase3.ps1`: build Editor/Game, bootstrap, 11/11 pruebas y cook exitosos.
- `Build/Reports/Phase3/index.json`: 11 exitosas, 0 con warnings, 0 fallidas.
- `Build/Logs/Phase3-harvest-runtime.log`: daño `100 → 75 → 50 → 25 → 0`, una transición `Falling` y una recompensa `wood=3 xp=5`.
- La recolección requiere acercarse al tronco; el jugador headless permaneció inmóvil y se valida manualmente.
- Aprobación manual del usuario: 27/08/2026, “funciona genial”.

## Riesgos

- Chaos puede producir caídas distintas según geometría; existe timeout determinista de asentamiento.
- Muchos Actors físicos son costosos; la Fase 3 mide el baseline y deja la futura materialización HISM detrás del registro forestal.
