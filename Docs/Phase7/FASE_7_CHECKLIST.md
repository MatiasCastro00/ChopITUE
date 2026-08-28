# Fase 7 — Tienda, armas adicionales y ranuras

## Alcance

- El hacha inicial es exclusiva y no consume ranura.
- La motosierra de mano y la sierra circular son armas compartidas comprables.
- El jugador tiene dos ranuras de armas compartidas durante la run.
- La compra descuenta dinero solo después de equipar el arma; si el cobro falla, el equipamiento se revierte.

## Validación automática

- [x] `Build/CI/VerifyPhase7.ps1` completa editor, bootstrap, automatización, juego y cook.
- [x] Las pruebas `ChopIt.Phase7.*` pasan.
- [x] Existen los Data Assets de las dos armas y `L_Test_Shop`.

## Validación manual

- [ ] En `L_Test_Shop`, vender madera para obtener al menos $20.
- [ ] Acercarse a la terminal junto a la cabaña y presionar `E`.
- [ ] La tienda aparece fija en pantalla; elegir con `1` o `2`, cerrar con `Esc`.
- [ ] La compra descuenta el precio y el arma realiza ataques automáticos adicionales.
- [ ] Reintentar con saldo insuficiente, arma ya equipada y dos ranuras llenas: no se pierde dinero ni se equipa un arma extra.
