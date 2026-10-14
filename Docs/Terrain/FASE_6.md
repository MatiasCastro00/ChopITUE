# Generador de terreno forestal — Fase 6

Estado: completada y verificada. Unreal Engine 5.8.2.

## Uso

En **Tools > ChopIt Terrain Generator**, expandir **Terrain > Regions** para configurar la clasificación. En **Preview > Preview Map**, elegir superficie, alturas, pendientes, curvatura, transitabilidad, claros, humedad potencial, densidad potencial de vegetación, categorías de regiones o transiciones suaves. Debajo del viewport aparece la leyenda correspondiente.

Cambiar la visualización actualiza colores y material sin regenerar la heightmap ni crear colisiones. Los colores muestrean los datos en los vértices de la preview: aumentar **Preview Polygon Detail** permite apreciar características pequeñas. La malla definitiva conserva su material habitual.

## Datos espaciales

`FChopItHeightField::GetMaps()` expone los mapas calculados después del ajuste final de alturas y del nivel de agua. Todos tienen exactamente `SamplesX * SamplesY` elementos, orden `X + Y * SamplesX`, y cubren las mismas dimensiones y coordenadas locales centradas que la heightmap. La altura original se reutiliza como Height Map, sin copiarla.

| Mapa | Significado |
|---|---|
| Height | Elevación local en centímetros. |
| Slope | Ángulo de la pendiente, en grados. |
| Curvature | Laplaciano de la elevación, en 1/cm: positivo cóncavo, negativo convexo. |
| Walkability | Potencial 0–1 según pendiente y altura sobre el agua. No sustituye el sistema de navegación. |
| Clearing | Selección procedural suave de claros, limitada por pendiente y agua. |
| Moisture Potential | Potencial 0–1 por proximidad vertical al agua y concavidad amplia; no simula hidrología. |
| Vegetation Density Potential | Densidad 0–1 condicionada por transitabilidad, humedad y exclusión de claros. |
| Region Map | Categoría dominante por muestra. |
| Region Blends | Siete pesos continuos normalizados que describen las transiciones. |

Las regiones son **Flat Forest**, **Rolling Hills**, **Steep Hills**, **Valleys**, **Lowlands**, **Clearings** y **Transitional**. La clasificación combina la pendiente real, concavidad amplia, proximidad al agua y selección de claros. Los pesos suman uno; la categoría de transición ocupa zonas sin una región claramente dominante. Cambiar controles regionales modifica estos datos y conserva las alturas.

`FChopItTerrainAnalysis::Build` es un análisis puro de CPU reutilizable para una cuadrícula de elevaciones. Las derivadas usan distancias físicas diferentes para cada eje; los bordes emplean diferencias unilaterales. Con solo dos muestras por eje se calcula la pendiente lineal y se considera nula la segunda derivada en ese eje. La detección amplia de valles toma muestras simétricas para no inventar concavidad en los bordes de una rampa.

`SampleLocalMap` y `SampleLocalRegionBlend` interpolan en las mismas coordenadas locales que `SampleLocalHeight`. Las posiciones exteriores fallan y reinician la salida. La consulta mediante coordenadas del mundo y Blueprints corresponde a la fase 7.

## Controles regionales

- **Region Scale**: escala horizontal de los parches de claros y de la detección amplia de valles.
- **Clearing Coverage**: cobertura de selección antes de aplicar restricciones del terreno. Es un control de distribución, no una promesa de porcentaje final exacto. Cero desactiva los claros y uno selecciona todo el suelo adecuado.
- **Transition Width**: anchura de transiciones de selección y umbrales de pendiente.
- **Flat / Steep Slope Degrees**: umbrales de clasificación; el primero debe ser menor que el segundo.
- **Max Walkable Slope Degrees**: centro del umbral suave de transitabilidad.
- **Shore Blend Height**: altura sobre el agua durante la cual aumenta gradualmente la aptitud del suelo seco.
- **Moisture Height Range**: distancia vertical de influencia del agua sobre la humedad potencial.
- **Vegetation Density**: multiplicador final de densidad potencial.

Los ruidos usan el Seed del terreno y un canal determinista independiente. Las funciones de ruido existentes se extrajeron a `ChopItTerrainNoise` para compartirlas sin duplicarlas. `GeneratorVersion` es 3 y `AnalysisVersion` es 1. No se generan objetos de follaje.

## Preview

Las máscaras escalares usan negro = 0 y blanco = 1. Las alturas usan azul, verde azulado y amarillo. Pendiente usa verde a rojo entre 0 y 45 grados; rojo también representa pendientes superiores. Curvatura usa blanco para cero, naranja para convexidad y azul para concavidad, con media intensidad a +/-0.001 1/cm.

La paleta regional usa verde para bosque plano, ocre para colinas onduladas, rojo para empinadas, violeta para valles, azul para zonas bajas, amarillo para claros y gris para transiciones. **Region Map** selecciona la categoría de la muestra más cercana a cada vértice; **Smooth Region Transitions** interpola los pesos regionales. El render interpola los colores sobre los triángulos, por lo que el detalle visual puede ser más grueso que los datos originales.

## Verificación

Compilaciones Win64 Development de `ChopItEditor` y `ChopIt` correctas. La suite final `ChopIt.Terrain` pasó **21 pruebas, 0 fallos**, con reporte en `Build/Reports/TerrainPhase6Final/index.json` y log en `Saved/Logs/TerrainPhase6Final.log`.

Las pruebas de mapas verifican superficies planas, rampas y cuencas analíticas, unidades físicas, bordes, cuadrícula mínima, repetibilidad, alineación, pesos normalizados, clasificación de las siete regiones, efecto de los controles y rechazo de entradas inválidas sin perder el resultado previo. Las pruebas de preview verifican el material de colores, correspondencia espacial de todos los vértices en todos los chunks, conservación de geometría y mapas al cambiar la vista o el detalle, ausencia de colisiones y render real de los nueve modos de mapas.

Se inspeccionaron visualmente las capturas `Saved/Terrain/Phase6_*.png`. La primera pasada detectó poco contraste cromático en el centro de la paleta de alturas; se corrigió la paleta y la suite final pasó completa. No se realizó una sesión manual de mouse en la pestaña del editor; su escena y cliente de viewport se verificaron mediante automatización.

## Archivos de esta fase

Nuevos en `ChopItWorld`: `ChopItTerrainAnalysis.h/.cpp`, `ChopItTerrainMaps.h`, `ChopItTerrainRegionSettings.h/.cpp` y `ChopItTerrainNoise.h/.cpp`. Modificados: `ChopItHeightField.h/.cpp`, `ChopItTerrainSettings.h/.cpp` y `ChopItTerrainGenerator.h/.cpp`.

Editor: nuevo `ChopItTerrainPreviewMode.h`; modificados `ChopItTerrainEditorSettings.h`, `ChopItTerrainPreview.h/.cpp`, `SChopItTerrainPreviewViewport.cpp` y `ChopItTerrainEditorActions.cpp`.

Pruebas: nuevo `ChopItTerrainAnalysisTest.cpp` y ampliado `ChopItTerrainPreviewTest.cpp`.

## Para continuar

Fase 7: persistencia y herramientas de máscaras. Reutilizar los mapas inmutables, sus unidades y su cuadrícula común; conservar datos manuales de forma separada al regenerar. Esperar `siguiente` antes de avanzar.
