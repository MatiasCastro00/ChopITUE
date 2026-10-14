# Generador de terreno forestal — Fase 5

Estado: implementación completa. Unreal Engine 5.8.2.

## Controles

Los controles aparecen en las categorías del panel **Tools > ChopIt Terrain Generator**. La configuración reflejada sigue siendo la fuente única para preview y terreno del nivel.

- **Noise**: ruido principal (`Octaves`, `Persistence`, `Lacunarity`) y hasta cuatro `NoiseLayers` aditivas. Cada capa tiene activación, fuerza, frecuencia, octavas, persistencia, lacunaridad y desplazamiento de semilla propios. La lista vacía conserva el relieve original.
- **Hills / Valleys**: `HillStrength` y `ValleyStrength` existentes; `HillShape` ensancha o concentra las cimas y `ValleyShape` cambia la anchura del fondo. El valor 2 reproduce el perfil anterior.
- **Plains**: `PlainsAmount` determina cuánto se aplana; `PlainsCoverage` cambia qué extensión ocupa la selección. Cobertura 0 excluye llanuras y 1 incluye todo el mapa.
- **Slopes / Roughness**: `SlopeBias` suaviza o acentúa gradientes locales. `SmallIrregularity` mantiene la amplitud del detalle fino y `RoughnessFrequency` cambia su escala espacial.
- **Warp / Edges**: `DomainWarpStrength` deforma las coordenadas mediante dos ruidos independientes y `DomainWarpFrequency` fija su escala. `EdgeFalloff` desvanece el relieve crudo desde `EdgeFalloffStart` hasta el borde.
- **Height Curve**: `HeightDistributionCurve` es una lista de puntos normalizados `(entrada, salida)` con interpolación lineal. Vacía significa identidad. Debe comenzar en `(0,0)`, terminar en `(1,1)`, avanzar en X y no retroceder en Y; máximo 16 puntos.
- **Depressions**: profundidad y frecuencia de depresiones suaves, independientes del perfil de valles.
- **Terraces / Erosion**: fuerza de terrazas y número de niveles; erosión térmica ligera con fuerza e iteraciones. Ambos efectos se desactivan al poner su fuerza a cero.
- **Water**: `WaterLevel` fija una cota manual. Al activar `bAutoWaterLevel`, `TargetSubmergedFraction` elige una cota para aproximar el porcentaje de área sumergida. La preview muestra la cota efectiva y la cobertura real. `FChopItHeightField` expone `GetWaterLevel()` y `GetSubmergedFraction()` para consumidores C++.

Los parámetros secundarios solo afectan al resultado cuando su fuerza o activación principal está habilitada. No se duplicaron los controles de fuerza de llanuras, colinas, valles ni rugosidad existentes.

## Orden de generación

El generador calcula relieve, capas, deformación, depresiones y atenuación de bordes en una cuadrícula determinista. Después aplica el ajuste local de pendientes, erosión, curva de alturas y terrazas. Finalmente realiza el ajuste afín global que conserva `TargetMeanHeight`, `MinHeight` y `MaxHeight` sin recortar mesetas. `GeneratorVersion` pasa a 2. Los nuevos efectos vienen desactivados o en valores neutros por defecto.

La cobertura de agua se estima en los centros bilineales de las celdas de la heightmap, que representan áreas XY iguales. La selección automática usa un cuantil de esos centros. Una cuadrícula finita tiene resolución de cobertura de una celda; si muchas celdas comparten exactamente la misma altura, ningún nivel de agua puede separar esas celdas y el porcentaje objetivo quizá no sea alcanzable. Se comunica siempre el porcentaje real. El agua es un dato de generación y no crea una malla ni simulación de agua.

La erosión transfiere pequeñas cantidades de altura hacia vecinos más bajos por encima de un umbral de talud. Es determinista, conservativa antes del ajuste global y limitada a ocho iteraciones; no modela sedimentación hidráulica. La curva de distribución es monótona para evitar inversiones de altura.

## Verificación

Las compilaciones Win64 Development de `ChopItEditor` y `ChopIt` terminaron correctamente después de todos los cambios. `ChopIt.Terrain` ejecutó 16 pruebas con 0 fallos en `Build/Reports/TerrainPhase5CellArea/index.json`. Las pruebas nuevas comprueban efectos de cada grupo, controles secundarios cuando están activos, determinismo, media y límites, llanuras completas, energía de pendientes, borde continuo, número de terrazas, ajuste de agua y validación sin alterar un resultado previo. La suite completa incluye la preview, geometría y colisión de fases anteriores.

No se realizó una sesión manual de edición de cada control en la pestaña acoplada. La presentación de propiedades usa el mismo `DetailsView` reflejado de las fases anteriores.

## Para continuar

Fase 6: regiones y máscaras espaciales alineadas con esta heightmap. Esperar `siguiente` antes de avanzar.
