# Fase 6 — experiencia, niveles y mejoras

## Objetivo

Agregar progresión de run exclusivamente por XP: los árboles acreditan XP, cada umbral encola una elección y tres mejoras data-driven aparecen de forma determinista. La selección pausa el mundo, se realiza con `1/2/3` y aplica modificadores reversibles y acumulables.

## Clases y assets

- `UChopItExperienceComponent`: ledger entero de XP, nivel y cola de elecciones.
- `UChopItUpgradeOfferComponent`: catálogo, pesos, seed, stacks, selección única y handles.
- `UChopItUpgradeDefinition`: nombre, descripción, rareza, peso, límite y modificadores.
- `Curve_XP_Levels`: requisitos por nivel.
- Ocho upgrades de prueba: daño, cadencia, alcance, crítico, movimiento y tres combinaciones.
- `L_Test_Progression`: mapa funcional de la fase.
- `AChopItHUD`: HUD screen-space persistente y tarjetas de mejora sobre una capa oscura.

## Gate automatizado

- [x] Development Editor y Game compilan.
- [x] Bootstrap Fase 6 es repetible.
- [x] Suites Smoke y Phase1–Phase6 pasan (24/24).
- [x] Cook Win64 termina con 0 errores y 0 warnings.
- [x] XP simultánea no pierde niveles pendientes.
- [x] Ofertas con el mismo seed conservan orden y no se repiten.
- [x] Stacks respetan caps y al remover handles recuperan el valor base.

## Comprobación manual

- [ ] HUD muestra nivel y XP actual/requerida.
- [ ] El HUD permanece anclado a pantalla y no sobre el personaje.
- [ ] Dos árboles (5 XP cada uno) producen el primer level-up.
- [ ] El mundo se pausa y aparecen tres opciones legibles.
- [ ] `1`, `2` o `3` aplica exactamente una opción y reanuda el juego.
- [ ] Una mejora de daño/cadencia/alcance cambia el hacha de forma perceptible.
- [ ] Una mejora de botas aumenta la velocidad de movimiento.

## Criterios de aceptación

- [x] Las mejoras no se compran con dinero.
- [x] Toda XP acreditada se conserva y los level-ups múltiples se serializan.
- [x] Las ofertas son data-driven, ponderadas y deterministas por seed.
- [x] Una selección aplica una sola vez y respeta `MaxStacks`.
- [x] Los modificadores pueden retirarse sin alterar otras fuentes.
- [ ] Resultado jugable: subir de nivel talando y transformar el hacha/personaje.

## Evidencia automática (2026-08-27)

- `Build/CI/VerifyPhase6.ps1` finalizó correctamente.
- `Build/Reports/Phase6/index.json`: 24 exitosas, 0 fallidas, 0 warnings.
- Curva, ocho upgrades y `L_Test_Progression` fueron generados y cargados por automatización.
- Cook Windows: `Success - 0 error(s), 0 warning(s)`.

## Riesgos

- La UI de texto es provisional hasta la Fase 13, pero ya separa presentación de reglas.
- Los enemigos nocturnos acreditarán XP mediante el mismo componente en la Fase 8.
- Los modificadores de comportamientos (rebote, proyectiles, órbita) se incorporan con armas adicionales en la Fase 7.
