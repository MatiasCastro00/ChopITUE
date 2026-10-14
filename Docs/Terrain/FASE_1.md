# Generador de terreno forestal — Fase 1

Estado: Fase 1 completada y verificada el 10 de octubre de 2026. Motor del proyecto: UE 5.8.2.

## Alcance y uso

El generador vive en `ChopItWorld` y no necesita World, Actor, editor, Blueprint ni estado aleatorio global. Usa ruido de gradiente 2D con hash entero por Seed, fractal normalizado, perfiles suaves de colinas/valles y atenuación espacial de llanuras. No crea geometría ni contenido.

```cpp
#include "Terrain/ChopItTerrainGenerator.h"

FChopItTerrainSettings Settings;
Settings.Seed = 1337;
FChopItHeightField Field;
FString Error;
if (FChopItTerrainGenerator::Generate(Settings, Field, Error))
{
    float Height;
    Field.SampleLocalHeight(FVector2D::ZeroVector, Height);
}
```

`FChopItTerrainSettings` es un USTRUCT reflejado, apto para propiedades de futuros objetos de configuración. `FChopItHeightField` es un resultado C++ con muestras de solo lectura, dimensiones, mínimo/máximo real y media real. `FChopItTerrainGenerator::GeneratorVersion` identifica la versión 1 del algoritmo. No se añadieron dependencias de módulos.

## Contrato de datos

- Centímetros; XY local centrado en el terreno. X va de `-Width/2` a `+Width/2`; Y, de `-Length/2` a `+Length/2`. Z contiene altura local.
- Orden de muestras: `X + Y * SamplesX`. Se incluyen ambos bordes; espaciamiento `Width/(SamplesX-1)` y `Length/(SamplesY-1)`.
- Resolución de datos: de 2 a 2049 muestras por eje. Es independiente del futuro slider de polígonos. El máximo almacena aproximadamente 16 MiB de alturas, además del resultado previo que conserve el llamador.
- `SampleLocalHeight` interpola bilinealmente, devuelve false fuera del rectángulo o ante coordenadas no finitas y deja altura de salida en cero. La consulta en coordenadas del mundo corresponde a la Fase 7.
- `MinHeight` y `MaxHeight` son límites permitidos, no extremos obligatorios. `TargetMeanHeight` representa la media aritmética de las muestras, no una integral ponderada por área.
- Se resta la media del ruido y se aplica una única escala global: primero `HeightVariation`, reducida si hace falta para respetar los límites. Luego se suma la media objetivo. Así se conserva el perfil sin mesetas por saturación. Un Clamp final protege el redondeo numérico.
- Variación cero, límites coincidentes, media en un extremo permitido o todos los aportes de relieve desactivados producen un terreno plano. Aumentar la variación más allá del límite disponible deja de aumentar el relieve.
- Misma Seed, configuración, versión del generador y entorno de compilación producen las mismas muestras. No se promete identidad binaria entre plataformas/compiladores distintos. Las alturas se almacenan como float y sus estadísticas se acumulan en double.
- La generación es síncrona y usa únicamente datos locales. Un futuro trabajo de preview puede ejecutarla fuera del game thread con su propia copia de configuración y resultado. Publicar UObjects/geometría seguirá siendo responsabilidad del editor.
- Una configuración inválida devuelve false, explica el error y conserva el resultado anterior. Los límites se validan en C++, además de los metadatos del editor.

## Parámetros de relieve

| Parámetro | Efecto |
|---|---|
| Width / Length | Extensión física, manteniendo la escala del ruido en centímetros. |
| ReliefScale | Tamaño horizontal de los relieves principales. |
| HeightVariation | Escala vertical antes del ajuste a límites. |
| PlainsAmount | Atenuación suave de colinas y valles en áreas seleccionadas por ruido; no indica porcentaje de cobertura. |
| HillStrength / ValleyStrength | Intensidad relativa de las partes positivas y negativas del perfil. La media final desplaza el conjunto. |
| SmallIrregularity | Variación pequeña, de frecuencia mayor, sumada al perfil. |
| Octaves / Persistence / Lacunarity | Número de octavas, caída de amplitud y multiplicador de frecuencia del ruido fractal. |

Los valores iniciales producen un bosque de relieve moderado. Esta fase no impone una pendiente máxima: esa restricción y los controles avanzados pertenecen a la Fase 5. Con una cuadrícula muy gruesa pueden perderse detalles de alta frecuencia. Cambiar la resolución del heightmap cambia ligeramente las estadísticas de normalización; cambiar el futuro detalle poligonal debe reutilizar exactamente el mismo heightmap.

## Verificación

La suite `ChopIt.Terrain.Core` está en `ChopItTests`. Comprueba repetición exacta de semillas (incluidos negativos y extremos int32), independencia de generaciones anteriores, límites/media, parámetros, casos planos, interpolación y entradas inválidas.

Resultados verificados:

- `ChopItEditor Win64 Development`: Succeeded, 17 acciones; compilación de los tres archivos del núcleo, reflexión UHT y suite de pruebas.
- `ChopIt Win64 Development`: Succeeded, 28 acciones; el núcleo compila y enlaza en el target de juego.
- `ChopIt.Terrain.Core`: 5 pruebas ejecutadas, 5 Success, 0 fallos. Informe en `Build/Reports/TerrainPhase1/index.json`; registro en `Saved/Logs/TerrainPhase1.log` (artefactos locales ignorados por Git).
- No se verificó presentación visual ni colisión; aún no hay geometría en esta fase. Tampoco se ejecutó cook ni pruebas de otras features.

Compilar `ChopItEditor Win64 Development` con `D:/UE_5.8/Engine/Build/BatchFiles/Build.bat`. Ejecutar `UnrealEditor-Cmd.exe` con el proyecto, `-NullRHI -unattended -nop4 -NoSound`, `-ExecCmds="Automation RunTests ChopIt.Terrain.Core"`, `-TestExit="Automation Test Queue Empty"` y `-ReportExportPath` para conservar el informe. El script antiguo `Build/CI/VerifyPhase1.ps1` corresponde a gameplay y genera mapas bootstrap: no se usa para esta feature.

## Continuidad entre fases

La próxima fase autorizable es la 2, geometría procedural. Debe consumir el resultado existente, construir `UDynamicMeshComponent`, usar normales de vértice/esquina para smooth/flat shading y mantener resolución de malla separada de la cuadrícula de datos. Si se usan chunks, se divide un único heightmap ya normalizado y se comparten sus muestras de borde; no se normaliza cada chunk por separado.

Se mantienen las decisiones de la Fase 0: interfaz y preview en `ChopItEditor`; persistencia posterior mediante asset de datos separado del actor; pruebas en `ChopItTests`. No se integra el registro de árboles ni se reemplaza el suelo actual. Ventana, preview, geometría, máscaras, exportación, presets y persistencia quedan para sus fases. Solo avanzar cuando el usuario escriba `siguiente`.
