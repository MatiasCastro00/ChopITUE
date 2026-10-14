# Generador de terreno forestal — Fase 3

Estado: completada y verificada. Unreal Engine 5.8.2.

## Uso

Abrir **Tools > ChopIt Terrain Generator**. La ventana puede acoplarse como otras pestañas del editor. Editar los parámetros de Generation y Geometry en el panel, usando sliders o campos numéricos. Los valores se guardan en la configuración personal del editor al terminar cada cambio.

- **Generate** crea un `AChopItTerrainActor` en el nivel abierto si no hay objetivo. Si la ventana ya tiene un objetivo, actualiza ese mismo actor.
- **Use Selected Terrain** vincula la ventana al actor de terreno seleccionado en el nivel.
- **Regenerate** actualiza únicamente el actor objetivo; muestra un error si falta o pertenece a otro nivel.
- **Random Seed** elige un nuevo Seed y lo guarda en la configuración. **Reset Defaults** restaura los valores iniciales. Ambos requieren pulsar Generate o Regenerate para modificar el nivel.
- Guardar el nivel después de generar terreno. La configuración del panel se guarda por separado de los actores del mapa.

El panel funciona en el editor sin Play Mode. Rechaza la generación durante Play Mode o sin un nivel editable. Los parámetros inválidos muestran un diagnóstico sin reemplazar la geometría existente. Generate/Regenerate son operaciones de transacción del editor para el actor; las propiedades del panel se guardan en `EditorPerProjectUserSettings`.

## Código

- `UChopItTerrainEditorSettings`: wrapper de configuración por usuario que reutiliza `FChopItTerrainSettings` y `FChopItTerrainMeshSettings`.
- `FChopItTerrainEditorActions`: validación, creación diferida de actores, regeneración, Random Seed y Reset Defaults. La creación diferida evita generar primero el terreno con valores predeterminados.
- `FChopItEditorModule`: registra la pestaña Nomad y la entrada del menú Tools. `IDetailsView` proporciona categorías plegables, búsqueda, campos numéricos y sliders basados en metadatos `UIMin`/`UIMax`.

La herramienta no contiene una preview 3D integrada. La Fase 4 añadirá una preview interactiva al panel; el nivel abierto ya muestra la malla real generada.

## Verificación

Las compilaciones Win64 Development de `ChopItEditor` y `ChopIt` pasaron. La suite `ChopIt.Terrain` completó diez pruebas sin fallos (`Build/Reports/TerrainPhase3/index.json`). Tras mover el punto de entrada al archivo convencional del módulo, se recompiló y repitió la prueba `ChopIt.Terrain.Editor.Actions`: pasó y abrió/cerró la pestaña acoplable con `TryInvokeTab` (`Build/Reports/TerrainPhase3Tab/index.json`).

La prueba del editor comprobó creación en un mapa editable, actualización del mismo actor, rechazo de parámetros inválidos sin duplicar actores y persistencia de Seed y Polygon Detail al guardar y recargar la configuración. La prueba restableció los valores previos al finalizar. El panel no se inspeccionó manualmente en una sesión interactiva con mouse, pero la creación de su widget y los flujos de acción se ejecutaron en Unreal Editor con renderizado activo.

## Para continuar

La Fase 4 debe extender la pestaña acoplable y reutilizar los datos de altura y la construcción de malla existentes para una preview 3D interactiva, sin cocinar colisiones para cada cambio. Esperar `siguiente` antes de avanzar.
