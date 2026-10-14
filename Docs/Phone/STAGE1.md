# Teléfono misterioso — Etapa 1

La escena que usa `AChopItAllFeaturesGameMode` genera el terreno, alinea los objetos colocados y luego intenta generar teléfonos. No hace falta generación de árboles. El resultado se registra en Output Log como `Phone spawn: seed=... count=...`.

## Configuración

- `Content/ChopIt/Phone/DA_PhoneSpawn` controla probabilidad, máximo de teléfonos, distancias respecto del primer `PlayerStart`, límites de agua/pendiente/transitabilidad, volumen libre, ruta y número de intentos. La seed no se guarda en el asset; se deriva de la seed de la partida.
- `Content/ChopIt/Phone/BP_MysteryPhone` es el Blueprint visual. Hereda del actor C++ y contiene dos cubos de reemplazo para que el resultado sea visible. Se puede cambiar el modelo desde el Blueprint.
- `AChopItAllFeaturesGameMode::PhoneSpawnConfig` referencia el Data Asset. Si falta o es inválido, no aparece ningún teléfono y el log explica el fallo.
- `AChopItMysteryPhone::InteractionRange` debe ser igual o menor que el rango del componente de interacción del jugador. El actor valida el rango otra vez antes de aceptar la respuesta.

## Límites de esta etapa

El actor empieza en `Available`. Al interactuar pasa a `Answered` una sola vez y emite `OnAnswered`. Las transiciones posteriores ya están definidas para conectar llamadas, transformación y diálogo en las próximas etapas. Todavía no hay timbre, viñetas, variantes, recompensas ni presentación 3D.

La comprobación de acceso muestrea terreno y obstáculos en la ruta recta desde el inicio; aún no calcula caminos alternativos con navegación. Un cambio futuro en la generación de árboles debe ocurrir antes de esta búsqueda de ubicación.

## Verificación

Las pruebas `ChopIt.Phone.Stage1` cubren seed reproducible, agua, separación entre eventos, interacción única, transiciones y generación desde el GameMode con el Data Asset y Blueprint reales.
