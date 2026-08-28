# Fase 8 — Enemigos, spawns y director de dificultad

## Alcance

- Dos familias nocturnas data-driven: árbol animado y árbol veloz.
- El director gasta presupuesto por oleada, aumenta gradualmente y limita la densidad a 12 enemigos.
- Los enemigos persiguen al jugador, hacen daño de contacto y dan más XP que un árbol normal.
- La vida del jugador se muestra en la UI y llegar a cero transiciona la run a derrota.

## Validación automática

- [x] Compilación de editor y juego, bootstrap, automatización y cook de Windows completan correctamente.
- [x] Las pruebas `ChopIt.Phase8.*` pasan.
- [x] Existen ambos Data Assets de enemigo, el director y `L_Test_Enemies`.

## Validación manual

- [ ] En `L_Test_Enemies`, pagar la cuota y esperar la noche.
- [ ] Aparecen oleadas progresivas alrededor del jugador, sin superar una densidad legible.
- [ ] Los árboles animados persiguen, dañan y se pueden derrotar con las armas automáticas.
- [ ] Cada muerte entrega XP; el árbol veloz entrega más XP que el básico.
- [ ] Al accionar la palanca tras el mínimo nocturno, el director limpia los enemigos al salir de noche.
