# Fase 5 — ciclo día/noche y regreso a la cabaña

## Objetivo

Introducir una máquina de estados temporal única y observable. El día termina en atardecer, la cuota pagada habilita la noche y la palanca solo puede cerrar una noche válida; una cuota impaga al límite duro produce derrota provisional.

## Autoridad y clases

- `UChopItRunStateComponent`: día, tiempo total y resultado provisional.
- `UChopItCycleStateMachineComponent`: única autoridad de fase, timers cancelables y generación contra callbacks tardíos.
- `EChopItCyclePhase`: `Bootstrap`, `Day`, `Dusk`, `Night`, `Elite`, `Resolution`, `Death`.
- `UChopItWorldPresentationComponent`: luz/feedback provisional observado desde la FSM.
- `AChopItQuotaMachine`: palanca interactiva que solicita, pero no fuerza, una transición.
- `UChopItDayDefinition`: duraciones suave/dura, noche mínima y resolución.
- `L_Test_Cycle` y assets de Fase 5 generados por bootstrap.

## Gate automatizado

- [x] Development Editor y Game compilan.
- [x] Bootstrap Fase 5 es repetible.
- [x] Suites Smoke y Phase1–Phase5 pasan (20/20, sin warnings).
- [x] Cook Win64 termina con 0 errores y 0 warnings.
- [x] Veinte ciclos acelerados no dejan callbacks tardíos.

## Comprobación manual

- [x] HUD muestra fase y tiempo desde la FSM.
- [x] Atardecer guía visualmente hacia la cabaña.
- [x] Cuota pagada permite entrar en noche; impaga termina en derrota al límite duro.
- [x] La palanca es rechazada antes de la noche mínima.
- [x] La palanca válida entra en élite provisional y luego resolución.
- [x] Reiniciar el mapa no conserva timers ni callbacks del ciclo anterior.

## Criterios de aceptación

- [x] Solo la FSM cambia de fase.
- [x] Todo timer de fase se cancela o invalida al salir.
- [x] Cuota impaga al límite duro produce `Death`.
- [x] La palanca solo cierra una noche válida y pagada.
- [x] Resultado jugable: completar cuota, sobrevivir la transición y cerrar el ciclo.

## Evidencia automática (2026-08-27)

- Gate integral: `Build/CI/VerifyPhase5.ps1` finalizó correctamente.
- Reporte: `Build/Reports/Phase5/index.json`, 20 exitosas, 0 fallidas, 0 warnings.
- Stress: `ChopIt.Phase5.GenerationStress` validó 20 ciclos y 100 generaciones de fase.
- Runtime real: `Build/Logs/Phase5-cycle-runtime.log` registró `Day -> Dusk -> Death` (`1 -> 2 -> 6`) con cuota impaga.
- Cook: finalizó con `Success - 0 error(s), 0 warning(s)`.

## Correcciones posteriores a prueba manual

- La luz direccional interpola intensidad y color durante 2.5 segundos en vez de cambiar instantáneamente.
- Atardecer y noche activan una luz/marcador elevado sobre la cabaña y el HUD muestra distancia más dirección relativa a cámara.
- `Resolution` dura 2 segundos y reinicia automáticamente el ciclo: incrementa el día, limpia el progreso de cuota y entra en `Day`.
- Gate integral repetido tras las correcciones: 20/20 pruebas y cook con 0 errores/0 warnings.
- Aprobación manual del usuario recibida al solicitar avanzar a la Fase 6.

## Riesgos

- Timers tardíos: cada callback captura/verifica una generación de fase.
- Dependencia circular cuota/ciclo: la FSM observa cuota; la máquina solo envía comandos.
- Fase élite aún provisional: se resuelve por timer hasta implementar enemigos reales en fases posteriores.
