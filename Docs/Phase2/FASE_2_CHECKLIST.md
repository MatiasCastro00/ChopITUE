# Fase 2 — ataque automático y daño

## Objetivo

El personaje utiliza un hacha básica automática en un arco frontal constante. El golpe daña dummies y árboles dentro del arco sin perseguir objetivos. El daño, la cadencia, el alcance, el ángulo, el límite de objetivos y los críticos nacen de datos y pueden recibir modificadores con handles removibles.

## Archivos, clases y assets

- `FChopItDamageSpec`: fórmula explícita y determinista.
- `UChopItCombatStatsComponent`: modificadores aditivos y multiplicativos por handle.
- `UChopItHealthComponent`: salud, daño acotado y muerte única.
- `UChopItTargetingSubsystem`: registro débil de objetivos por mundo y selección del más cercano.
- `UChopItAutoAttackComponent`: scheduler mediante timer, sin Tick.
- `UChopItWeaponDefinition`: configuración data-driven del arma.
- `AChopItCombatDummy`: objetivo visible de prueba.
- `AChopItTree`: árbol normal dañable; caída y recompensa quedan reservadas para la Fase 3.
- `DA_Weapon_BasicAxe` y `L_Test_Combat` generados por bootstrap.
- Durante esta fase, `L_Test_Combat` es el mapa inicial del editor para que la prueba manual siempre contenga objetivos.

## Gate automatizado

- [x] Development Editor compila.
- [x] Development Game compila.
- [x] `ChopIt.Smoke`, `ChopIt.Phase1` y `ChopIt.Phase2` pasan (8/8).
- [x] Bootstrap Fase 2 genera assets de forma repetible (segunda generación validada 4/4).

## Comprobación manual

- [x] El hacha ataca sin input y sin impedir el movimiento.
- [x] Solo reciben daño los objetivos dentro del arco frontal y del alcance.
- [x] El arco ocurre aunque no haya un objetivo válido.
- [x] Los árboles normales pierden vida al recibir el arco.
- [x] La vida visible baja 25 puntos por golpe y la muerte ocurre una vez.
- [x] Sin objetivos válidos no aparecen errores ni ataques fantasma.
- [x] La cadencia se mantiene estable con tasas de frame alta y baja.

## Criterios de aceptación

- [x] No existe `Tick` por arma ni búsqueda global de actores.
- [x] Daño y muerte coinciden con la fórmula y eventos previstos.
- [x] Altas y bajas de objetivos no dejan referencias inválidas.
- [x] El resultado jugable permite moverse mientras el hacha destruye dummies.

## Riesgos pendientes

- El arco naranja es feedback provisional; animación, VFX y audio definitivos pertenecen a la Fase 13.
- El registro lineal es suficiente para pocos dummies; antes de hordas se medirá y se sustituirá por partición espacial si el profiling lo exige.

## Evidencia

- Verificación completa: `Build/Reports/Phase2/index.json` (8 aprobadas, 0 fallidas).
- Verificación posterior al segundo bootstrap: `Build/Reports/Phase2-Idempotency/index.json` (4 aprobadas, 0 fallidas).
- `VerifyPhase2.ps1` completó build Editor, bootstrap, automatización, build Game y cook Windows el 27 de agosto de 2026.
- Ejecución runtime headless: `Build/Logs/Phase2-runtime-headless.log`; registra `75 → 50 → 25 → 0` y una única muerte del dummy atacado por `BP_ChopItCharacter_C_0`.
- Arco frontal y árboles dañables: `Build/Logs/Phase2-frontal-axe-runtime.log` más `Build/Reports/Phase2/index.json`; el dummy situado delante recibe `75 → 50 → 25 → 0`, la geometría del arco pasa sus casos delante/detrás/lateral/fuera de alcance y el mapa contiene 20 instancias de `AChopItTree`.
- Validación manual aprobada por el usuario el 27 de agosto de 2026; se autorizó avanzar a la Fase 3.
