# Fase 4 — cabaña, máquina, cuota y camioneta

## Objetivo

Cerrar el bucle económico de madera: la máquina recibe primero la cuota diaria y la camioneta vende únicamente el excedente. Todas las transferencias son enteras, auditables e idempotentes.

## Autoridad y clases

- `UChopItQuotaComponent`, montado en `AChopItGameState`: objetivo, progreso y transacciones diarias.
- `UChopItEconomyComponent`, montado en `AChopItPlayerState`: saldo `int64` y ledger idempotente.
- `AChopItCabinHub`: referencia espacial y composición visual, sin reglas económicas.
- `AChopItQuotaMachine`: fachada visual de la cuota y futura palanca.
- `AChopItDeliveryZone`: bombeo temporizado de carga hacia la cuota.
- `AChopItSellZone`: venta temporizada, bloqueada mientras la cuota esté incompleta.
- `UChopItDayDefinition`: configuración inmutable de cuota y valor de madera.
- `BP_CabinHub`, `BP_QuotaMachine`, `BP_DeliveryZone`, `BP_SellZone`, `DA_Day_01` y `L_Test_Economy`.

## Gate automatizado

- [x] Development Editor y Game compilan.
- [x] Bootstrap Fase 4 es repetible.
- [x] Suites Smoke y Phase1–Phase4 pasan: 15/15, sin warnings.
- [x] Cook Win64 termina sin errores ni warnings.
- [x] Runtime headless inicia con cuota 3, tala/recompensa única y ninguna venta anticipada.

## Comprobación manual

- [x] La máquina consume madera y muestra progreso de cuota.
- [x] La camioneta no consume antes de completar la cuota.
- [x] El excedente se vende sin perder unidades.
- [x] HUD debug refleja madera, cuota y dinero desde la fuente de verdad.
- [x] Reentrar en una zona no duplica pagos.

## Criterios de aceptación

- [x] Nada se vende antes de pagar la cuota.
- [x] Cuota, carga y excedente se conservan exactamente como enteros.
- [x] Saldo y ledger son enteros e idempotentes por `TransactionId`.
- [x] Las zonas no usan Tick y serializan transferencias mediante timers.
- [x] Resultado jugable: talar, pagar cuota y vender excedente.

## Riesgos

- Overlaps simultáneos: la venta consulta la cuota en cada lote y la cuota tiene prioridad explícita.
- Eventos duplicados: cuota y economía conservan IDs procesados durante el ciclo de vida de la run.
- Valores extremos: aritmética de dinero usa `int64` y rechaza overflow o saldo negativo.

## Evidencia automatizada

- `Build/CI/VerifyPhase4.ps1`: build Editor/Game, bootstrap Fases 1–4, 15/15 pruebas y cook exitosos.
- `Build/Reports/Phase4/index.json`: 15 exitosas, 0 con warnings, 0 fallidas.
- `Build/Logs/Phase4-economy-runtime.log`: cuota inicial 3, tres árboles talados y tres recompensas únicas; sin venta mientras la cuota sigue impaga.
- La secuencia con movimiento `recoger → cuota → camioneta` se reserva a comprobación manual visible.
- Aprobación manual del usuario: 27/08/2026, “funciona perfecto”.
