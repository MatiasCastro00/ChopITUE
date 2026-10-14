# Fase 7: datos, máscaras y exportación

## Guardado

Cada `AChopItTerrainActor` conserva en el nivel una instantánea comprimida del campo de alturas, los seis mapas de análisis, los ID de región y los siete pesos de región por muestra. El formato binario tiene versión, dimensiones y CRC. Las configuraciones y el historial de pinceladas son propiedades del actor. Guarda el nivel después de generar o editar.

La pestaña **Tools > ChopIt Terrain Generator** ofrece **Save Data** y **Load Data**. Indica antes la ruta **Data > Terrain Data File** en los ajustes. El archivo `.cterrain` es JSON con configuración, pinceladas y la instantánea binaria en Base64. La carga valida la versión, dimensiones y geometría antes de sustituir el actor. `SaveTerrainData` y `LoadTerrainData` también están disponibles para C++ y Blueprints.

## Edición de máscaras

Selecciona un `ChopItTerrainActor`, pulsa **Use Selected Terrain** y elige **Mask Editor > Edit Mask**: Walkability, Clearing, Moisture o VegetationDensity. Ajusta radio, fuerza y valor objetivo. Hay dos modos:

- Introduce **Brush World Position** y pulsa **Paint Mask**, **Erase Mask** o **Smooth Mask**. La posición inicial se toma del actor seleccionado.
- Activa **Paint on Preview**, elige **Preview Brush** y haz un clic sobre el terreno de la vista previa. El clic transforma las coordenadas locales a mundo y aplica la operación al actor; el arrastre derecho sigue orbitando la cámara. Usa **Preview > Preview Map** para ver la máscara editada.

Cada clic genera una pincelada con centro y radio normalizados. **Erase** restaura los valores procedimentales y **Smooth** promedia las muestras vecinas. Las pinceladas se reaplican en orden si cambias la semilla, tamaño o resolución y regeneras. El historial se limita a 4096 pinceladas por actor. Los mapas derivados de regiones permanecen procedimentales; las cuatro máscaras editables son capas independientes.

## Consultas

`SampleWorldHeight` devuelve Z de mundo. `SampleWorldMap` interpola las siete capas por coordenadas locales obtenidas desde una posición mundial. `SampleWorldRegion` devuelve la región dominante más cercana y `SampleWorldRegionWeights` los siete pesos interpolados en orden: FlatForest, RollingHills, SteepHills, Valleys, Lowlands, Clearings, Transitional. Fuera del terreno devuelven `false`. Estas funciones están expuestas a Blueprints.

## Exportación

Indica **Data > Export Directory** y pulsa **Export Maps**, o llama a `ExportTerrainMaps`. Produce `Height.r16` en orden de filas y little endian, seis mapas continuos, `RegionIDs` y siete máscaras `Region_*`, cada uno en `.pgm` P5 y `.png` de 8 bits. `TerrainExport.txt` contiene dimensiones y rangos físicos para reconstruir los valores. La altura se normaliza entre el mínimo y máximo real; los potenciales y pesos ya están en [0,1]. Los píxeles de `RegionIDs` contienen directamente los valores 0–6. Esto permite a futuros generadores de vegetación leer máscaras y mezclas sin invocar la generación de terreno.
