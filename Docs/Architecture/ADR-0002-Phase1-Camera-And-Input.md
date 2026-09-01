# ADR-0002: sistema integral de cámara y entrada

- **Estado:** reemplazado por esta revisión
- **Fecha original:** 2026-08-26
- **Revisión:** 2026-08-31
- **Decisores:** equipo de ChopIt

## Contexto

La cámara cenital fija limitaba exploración, combate, diálogo y animación. ChopIt necesita una tercera persona amplia y controlable, además de una única interfaz que permita superponer tomas, postprocesos y shakes sin acoplar gameplay al plugin experimental de Unreal.

## Decisión

Se reemplaza formalmente la cámara fija por el sistema integral de `ChopItPresentation`:

- `UChopItCameraComponent`, host derivado de Gameplay Cameras, resuelve órbita, zoom, colisión y tomas guionadas.
- `UChopItCameraDirectorSubsystem` es la única fachada pública para cues, efectos, shakes, reset y locks de entrada.
- La vista normal tiene FOV 85°, pivot a 120 cm, distancia inicial 850 cm, pitch inicial -32°, límite inferior de -65° y libertad hacia arriba hasta el resguardo técnico de +89°; el zoom cubre 550–1400 cm.
- El yaw es libre y persistente, sin recenter. El movimiento usa el yaw final del `PlayerCameraManager`.
- Los requests base usan prioridad determinista y orden de inserción. Los overrides anidados conservan y restauran exactamente la vista previa.
- Los estados de autoría son `GameplayOrbit`, `Scripted`, `Cinematic` y `Death`. El bootstrap crea rigs, StateTree y Camera Asset reproducibles.
- Los efectos visuales viven en modifier rigs y los impactos usan `UCameraShakeAsset`; no modifican el estado base.
- Los cues poseen locks independientes de cámara, movimiento y acciones, liberados por handle.
- Las preferencias viven en `UChopItCameraUserSettings`; `DeveloperSettings` conserva defaults globales.
- `ChopItCameraSolid` es opt-in: únicamente el suelo y las paredes que limitan el mapa empujan la cámara. Cabaña, palanca, árboles, máquinas, enemigos y demás props conservan su colisión de gameplay pero ignoran ese canal. Después de resolver la posición final, `UChopItCameraComponent` consulta un corredor entre cámara y sujeto: si una pieza de un actor entra en él, todas sus primitivas visuales reciben temporalmente un material masked con dithering temporal y recuperan exactamente sus materiales originales al despejarse.

Gameplay Cameras 5.8 expone partes necesarias como `MinimalAPI`. Para no modificar Engine, ChopIt reimplementa los overrides ABI no exportados y aporta una tarea y condición de StateTree propias dentro de `ChopItPresentation`. El StateTree selecciona realmente los rigs de `GameplayOrbit`, `Scripted`, `Cinematic` y `Death`, mientras la resolución de prioridades y handles permanece detrás de la fachada de ChopIt.

## Consecuencias

- Gameplay, diálogo y VFX no conocen rigs ni el CameraManager.
- Las cámaras futuras pueden autorizarse con assets y anchors animables desde Sequencer.
- El jugador mantiene control espacial consistente a cualquier yaw.
- El plugin experimental puede reemplazarse dentro de `ChopItPresentation` sin modificar reglas de juego.
- `L_Dev_Sandbox` contiene una interacción que demuestra primer plano, postproceso, shake, retirada y restauración.

## Verificación

- `ChopIt.Phase1.GameplayFramework` valida el host, defaults y movimiento.
- `ChopIt.Phase1.InputAssets` valida mouse, teclado y gamepad.
- El bootstrap genera rigs, StateTree, cues, efectos y shakes y es idempotente.
- CI compila Editor/Game, ejecuta automatizaciones y cocina Win64.
