# Fase 9: optimización y validación final

## Cambios

- Los pinceles Paint y Smooth actualizan únicamente la máscara elegida dentro del rectángulo del pincel. Smooth copia ese rectángulo y un borde de una muestra para conservar el resultado del filtro 3×3. Erase reconstruye la base procedural con los ajustes realmente generados.
- Las pinceladas quedan en propiedades transaccionales. La compresión del snapshot se difiere hasta guardar el nivel o exportar datos; Undo/Redo restaura el snapshot y reproduce las operaciones pendientes. `SnapshotStrokeCount=-1` mantiene la lectura de niveles de la fase 7/8, cuyos snapshots ya incluían todas las pinceladas.
- `PostLoad` recupera el heightfield y sus mapas incluso antes de ejecutar Construction Script, también en carga de niveles de juego. Los datos exportados se etiquetan con los ajustes generados, aunque existan valores nuevos todavía sin aplicar.
- Cambiar solo regiones recalcula análisis sobre las alturas existentes. Cambiar material o activar/desactivar colisión conserva la geometría. Rebuild sin cambios retorna sin publicar chunks. Si las alturas resultantes son idénticas, también se conserva la geometría.
- La carga externa construye los chunks una sola vez y publica el resultado validado. Los snapshots nuevos se validan y comprimen antes de reemplazar los datos del actor.
- El análisis de mapas distribuye filas independientes entre trabajadores a partir de 129 filas. No hay reducciones flotantes compartidas; los resultados siguen siendo deterministas.
- El codec escribe desde datos constantes sin duplicar el campo completo; reserva el tamaño de salida cruda conocido. Conserva el formato binario v1. Valida dimensiones por eje, rangos de máscaras/pendiente, estadísticas básicas y suma de pesos regionales.
- El cálculo de agua automática ordena el mismo buffer de alturas de celda. La construcción de mallas reserva arrays de chunks y atributos según su tamaño.
- La preview rechaza máscaras de una superficie diferente, compara los ajustes generados del actor y desactiva pintura durante una actualización. Se añade **Water Level / Submerged Surface** al selector de visualización.
- Se añade `SampleWorldNormal` para C++ y Blueprint. Altura, mapas, regiones y pinceles rechazan coordenadas no finitas y escalas degeneradas; las salidas fallidas se inicializan.
- **Bake Mesh** crea un Static Mesh con todos los chunks actuales, sus UV, normales, material y colisión compleja. Se crea un nombre único en `/Game/ChopIt/World/Generated/SM_Terrain…`, se selecciona en el Content Browser y queda pendiente de guardar como cualquier asset nuevo.

## Uso y contratos

Las consultas proyectan la posición recibida a **XY local del actor**. `SampleWorldHeight` devuelve Z mundial de esa superficie; no calcula una intersección vertical mundial cuando el actor está inclinado. Para colocar objetos sobre la geometría poligonal exacta, usar trazas de colisión. La normal procede de la derivada de la superficie bilineal del heightfield; utiliza rotación e inversa de escala del actor. Es independiente de Polygon Detail y puede diferir de la normal de una cara Low Poly.

Los valores espaciales usan centímetros. Las cuatro máscaras editables están en [0,1], pendiente en grados y curvatura en 1/cm. Los pesos de siete regiones suman uno. Pintar una máscara no cambia automáticamente la clasificación regional ni las otras máscaras. No se generan objetos de follaje.

El Static Mesh conserva coordenadas locales: al colocarlo en un nivel, usar el transform del actor original. Bake no reemplaza al actor ni guarda automáticamente el asset en disco. Produce un solo LOD; no configura Nanite, navegación específica ni UV de lightmap adicionales. Las normales y UV originales se conservan.

La vista de agua colorea en azul las muestras sumergidas y en verde las emergidas. La interpolación de colores depende de la resolución de preview; no es un plano de agua ni representa una simulación.

## Mediciones reproducibles

Prueba: `ChopIt.Terrain.Final.Benchmark`. Mide 257² y 513² muestras, ajustes predeterminados, Polygon Detail 50 y colisión desactivada. Es una medición puntual por resolución, en Win64 Development y en este equipo; no es una mediana ni una garantía de rendimiento. No incluye publicación de componentes/colisión en la columna de construcción de malla. La medición de Paint mide la llamada del actor, sin transacción del editor ni recoloreado de preview.

Datos base: `Saved/TerrainPhase9/Baseline.csv`. Datos actuales: `Saved/TerrainPhase9/Benchmark.csv`. Informes de automatización bajo `Build/Reports/TerrainPhase9*`.

| Operación (ms) | 257² antes | 257² después* | 513² antes | 513² después* |
|---|---:|---:|---:|---:|
| Generación + mapas | 41.233 | 35.758 | 172.861 | 102.920 |
| Encode | 125.449 | 100.198 | 508.564 | 431.698 |
| Decode | 16.393 | 14.889 | 70.591 | 70.305 |
| Construcción de malla | 1.200 | 1.114 | 1.213 | 2.034 |
| Paint | 178.269 | 0.020 | 686.477 | 0.028 |
| Rebuild sin cambios | 1.388 | 0.001 | 1.631 | 0.016 |

\* Ejecución final (`TerrainPhase9Final`). Las cifras pequeñas varían con la carga del sistema; no se afirma mejora de Decode ni de construcción de malla a partir de estas muestras. La compresión mantiene 1,864,598 y 7,387,838 bytes respectivamente. El mayor ahorro del pincel procede de diferir la compresión y eliminar generación/copia completa por pincelada.

Un campo contiene aproximadamente 57 bytes por muestra, sin contar capacidad extra de arrays. Evitar la copia completa al codificar ahorra otro campo temporal: aproximadamente 3.6 MiB a 257², 14.3 MiB a 513² y 228 MiB a 2049². Son tamaños derivados de las estructuras, no mediciones del pico total del proceso.

## Verificación

La batería final pasó **26 pruebas, 0 fallos, 0 sin ejecutar**, según `Build/Reports/TerrainPhase9Final/index.json`. La prueba de estrés detectó durante el desarrollo un mundo auxiliar cargado sin cerrar en el test de reapertura; se corrigió con `DestroyWorld` y se repitió la batería completa. El modo Water Level se renderizó offscreen y se inspeccionó visualmente en `Saved/Terrain/Phase6_WaterLevel.png`.

Los objetivos **ChopItEditor** y **ChopIt** compilaron correctamente en Win64 Development con UE 5.8.2. Después de la batería final, otro proceso de Unreal ejecutó `ChopIt.Terrain.Final.ReopenSavedLevel`: **1 prueba, 0 fallos** (`Build/Reports/TerrainPhase9ReopenFinal/index.json`). Se verificaron consulta de máscaras y normales, dos pinceladas persistentes y geometría cargada antes de regenerar. Para repetir esa prueba aislada, ejecutar primero `ChopIt.Terrain.Final.Lifecycle`, que escribe `Saved/TerrainPhase9/Roundtrip.umap`; la batería completa las ejecuta en ese orden.

La batería cubre determinismo, controles del generador y Polygon Detail, límites, continuidad de posiciones/normales/UV entre chunks, Flat/Smooth, trazas y barridos de colisión, ausencia de actores duplicados, equivalencia de preview y datos definitivos, resultados asíncronos obsoletos, render offscreen, presets y bloqueos, codec y corrupción, pinceles y exportación. Las pruebas nuevas cubren además:

- Reutilización de geometría ante cambios exclusivos de mapas o colisión.
- Undo/Redo real de máscaras mediante transacciones del editor.
- Normales perpendiculares a tangentes con rotación y escala no uniforme.
- Rechazo de NaN y escala cero.
- Exportación coherente cuando hay ajustes todavía sin generar.
- Guardado de nivel, historial y máscaras; carga en un proceso nuevo.
- Bake de Static Mesh, igualdad de número de triángulos y guardado del asset.
- Suavizado en bordes equivalente al vecindario 3×3 con clamp.
- Regeneración/repartición repetida con recolección de basura y trazas a las nuevas alturas.
- Render del modo Water Level en la preview.

## Límites conocidos

- La generación de alturas, compresión del guardado y publicación/cocinado de colisión definitivos siguen siendo síncronos. La preview calcula generación/malla en segundo plano, pero su cierre espera al trabajo activo; no tiene cancelación cooperativa.
- Un cambio global de relieve vuelve a generar el campo y los chunks afectados globalmente. No hay streaming de heightmaps, LOD dinámico ni edición de alturas por pincel. La regeneración parcial cubre máscaras, regiones, material y colisión.
- Erase reconstruye la base procedural; repetir borrado es más costoso que Paint/Smooth. El historial tiene un límite de 4096 operaciones. Su reproducción después de cambiar resolución puede acumular coste.
- Los snapshots comprimidos también forman parte de las transacciones del actor. Una transacción y la copia/recolor de mapas en la preview añaden coste a las cifras de Paint del benchmark.
- 2049² es el límite validado de datos, no una recomendación de edición interactiva. Un campo ocupa unos 228 MiB; generación, snapshot y preview pueden mantener varias copias. La medición de rendimiento se realizó hasta 513².
- El radio de pincel se almacena como fracción del lado menor; una escala horizontal no uniforme del actor puede hacer el pincel elíptico en el mundo. Las consultas de normales sí corrigen la escala no uniforme.
- Las normales bilineales pueden cambiar de derivada al cruzar una celda; para apoyar objetos en triángulos exactos se debe usar la normal del impacto de colisión.
- El agua automática usa centros de celda y alturas discretas; alturas repetidas pueden impedir alcanzar exactamente una fracción sumergida.
- El bake puede tardar con mallas grandes. No incluye los datos de máscaras ni la API del actor; conservar el actor o su archivo de datos para generar follaje después.
- Verificación automatizada en UE 5.8.2 / Windows. No se hizo una sesión manual exhaustiva con ratón, una prueba de juego empaquetado, ni validación en otras plataformas.

## Archivos de esta fase

- Runtime: `ChopItTerrainActor.h/.cpp`, `ChopItTerrainDataCodec.h/.cpp`, `ChopItTerrainAnalysis.cpp`, `ChopItTerrainGenerator.cpp`, `ChopItTerrainMeshBuilder.cpp`.
- Editor: `ChopItEditor.Build.cs`, `ChopItEditor.cpp`, `ChopItTerrainEditorActions.h/.cpp`, `ChopItTerrainPreviewMode.h`, `ChopItTerrainPreview.cpp`, `SChopItTerrainPreviewViewport.cpp`.
- Pruebas: nuevo `ChopItTerrainPhase9Test.cpp`; ampliados `ChopItTerrainMeshTest.cpp` y `ChopItTerrainPreviewTest.cpp`.
- Documentación: este archivo. Las carpetas Terrain de las fases anteriores siguen sin commit; no se creó ningún commit en esta fase.

## Contexto de continuidad

Se completan las fases previstas 0–9. El generador sigue en versión 3, análisis en versión 1, snapshots binarios y JSON en versión 1. `SnapshotStrokeCount` conecta el snapshot comprimido con el historial pendiente de pinceles. `PostLoad`, `PreSave` y `PostEditUndo` reconstruyen los caches derivados. Las nuevas extensiones deben conservar independencia entre heightfield y resolución poligonal, y validar cualquier cambio de formato con datos anteriores. Cualquier trabajo adicional requiere una nueva indicación del usuario.
