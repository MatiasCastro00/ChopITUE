# Generador de terreno forestal — Fase 2

Estado: completada y verificada. UE 5.8.2.

## Uso dentro de Unreal

Colocar un `ChopItTerrainActor` desde las clases C++ del proyecto. En Details, `Terrain Settings` controla el heightmap de la Fase 1 y `Mesh Settings` controla la geometría. El actor genera terreno al construirse; el botón `Regenerate Terrain` reconstruye sus componentes existentes. Puede asignarse un material en `Terrain Material`; por defecto reutiliza `MI_Ground` del blockout.

El actor no tiene Tick. Moverlo sin cambiar los parámetros reutiliza la geometría ya construida. Cambiar solamente detalle, sombreado, partición o colisión conserva el mismo heightmap en memoria. Un error de validación conserva la geometría anterior y devuelve un diagnóstico desde `RebuildTerrain`.

## Diseño

- `FChopItTerrainMeshSettings`: detalle 0–100, sombreado Smooth/Flat, tamaño de chunk y colisión.
- `FChopItTerrainMeshBuilder`: constructor C++ de datos `FDynamicMesh3`, sin acceso a World ni UObjects. Consume un `FChopItHeightField` constante.
- `FChopItTerrainMeshChunk`: malla, primera celda global y cantidad de celdas del chunk. Mantiene la malla en una dirección estable mediante `TUniquePtr`, porque sus atributos guardan referencias a ella.
- `AChopItTerrainActor`: publica las mallas en componentes `UDynamicMeshComponent`, todos unidos a una raíz estática. Actualiza los componentes existentes y destruye únicamente los sobrantes si disminuye el detalle.
- `ChopItWorld.Build.cs`: añade `GeometryCore` y `GeometryFramework`. No se habilitaron plugins ni se usa Geometry Script.

## Resolución y continuidad

El lado más largo usa `round(8 * 32^(PolygonDetail/100))` celdas. El otro lado se adapta a la relación ancho/largo, con al menos una celda. En un terreno cuadrado, detalle 0 produce 128 triángulos; detalle 100, 131072. El máximo de 256 celdas por lado limita el coste de esta primera implementación. La resolución del heightmap sigue siendo independiente.

Cada vértice muestrea el mismo heightmap mediante interpolación bilineal en coordenadas locales. El detalle cambia la aproximación geométrica; en detalle muy bajo se pierden las irregularidades menores. No cambia la Seed, distribución de ruido, normalización de alturas ni datos fuente.

Los chunks tienen hasta 64 celdas por lado por defecto, configurable entre 8 y 128. Sus bordes reutilizan posiciones calculadas en una cuadrícula global. Las normales Smooth se acumulan por área sobre la cuadrícula completa antes de dividirla: los bordes reciben exactamente la misma normal. Flat usa normales por cara en el overlay, sin cambiar posiciones, triángulos ni colisión. Las UV son globales, con una repetición cada 1000 cm.

No hay niveles de LOD diferentes entre chunks; todos comparten la resolución seleccionada. No se generan faldones ni paredes laterales: el terreno es una superficie abierta.

## Colisión y regeneración

La colisión usa los triángulos del terreno estático con `ComplexAsSimple`, tipo `WorldStatic`, perfil `BlockAll` y `QueryAndPhysics`. El terreno no simula física, pero admite consultas y contacto con cuerpos móviles. Respeta el canal y tag `ChopItCameraSolid` ya utilizados por el proyecto y puede afectar navegación. No se genera un NavMesh en esta fase.

Las actualizaciones de colisión se difieren hasta terminar cada chunk y se cocinan síncronamente para que la colisión esté lista al regresar de `RebuildTerrain`. Generaciones grandes pueden bloquear brevemente el editor; preview sin colisión y optimización asíncrona corresponden a fases posteriores.

Los componentes y su geometría pueden guardarse con el nivel. El heightmap en memoria es una caché no persistente; al reconstruirlo se obtiene de los parámetros guardados. El asset persistente de datos, máscaras y consultas de mundo sigue reservado para la Fase 7.

## Verificación

Pruebas añadidas en `ChopIt.Terrain.Geometry`:

- `MeshData`: conteos, topología, muestreo del heightmap, normales y UV de los bordes, sombreado plano, parámetros inválidos y proporciones extremas.
- `SurfacePreservation`: mide el error vertical en centros de triángulo frente al heightmap con tres semillas y los extremos del slider; comprueba que Flat/Smooth mantienen exactamente las mismas posiciones y triángulos.
- `ActorCollision`: colocación en un nivel, trazas y barrido de esfera, actualización y desactivación de colisión, reutilización del heightmap/componentes y ausencia de actores duplicados.
- `LevelRender`: requiere RHI real. Renderiza el terreno dentro de un nivel de editor, captura Smooth y Flat en `Saved/Terrain/Phase2_Smooth.png` y `Phase2_Flat.png` y guarda un mapa aislado en `/Game/ChopIt/Developer/Terrain/L_Terrain_Phase2`.

El mapa de inspección y las capturas son artefactos locales ignorados por Git. Los mapas de gameplay existentes no se modifican. Ejecutar la suite con `UnrealEditor-Cmd.exe`, `/Engine/Maps/Entry`, `-RenderOffscreen -unattended -nop4 -nosplash -NoSound`, `-ExecCmds="Automation RunTests ChopIt.Terrain"` y `-TestExit="Automation Test Queue Empty"`. No usar `-NullRHI` para `LevelRender`.

Las compilaciones Win64 Development de Editor y Game pasan. Las pruebas de datos, conservación de superficie y colisión pasan junto con las cinco pruebas del núcleo. Para las semillas 1337, -57 y 12, el error RMS vertical en centros de triángulo fue 27,702 / 37,819 / 43,115 cm en detalle 0 y 0,186 / 0,186 / 0,193 cm en detalle 100, sobre rangos de altura de 432 a 692 cm. Estas mediciones corresponden a los parámetros probados; no garantizan conservar detalles menores que una celda con cualquier configuración de ruido.

Resultado final: nueve pruebas verificadas. El reporte `Build/Reports/TerrainPhase2/index.json` contiene ocho éxitos y el fallo inicial de captura; `Build/Reports/TerrainPhase2Visual/index.json` contiene la repetición exitosa de `LevelRender` tras preparar explícitamente los shaders antes de la captura síncrona. Se inspeccionaron ambas imágenes: terreno visible con material verde, diferencias Flat/Smooth y bordes de chunks sin huecos visibles. No se verificó una sesión de juego interactiva ni contacto de un cuerpo en simulación; sí trazas y barrido de esfera contra la colisión real de Chaos.

Se corrigieron durante la validación la orientación de los triángulos (GeometryCore utiliza winding horario y producto vectorial invertido), la estabilidad de dirección de las mallas y la preparación de shaders del test visual. La compilación explícita de shaders pertenece únicamente al test, no a la regeneración normal del actor.

## Para continuar

La Fase 3 creará la ventana acoplable y organizará los controles del editor. Debe reutilizar `TerrainSettings`, `MeshSettings` y `RebuildTerrain`, conservando un actor objetivo. La preview interactiva es Fase 4. Esperar `siguiente` antes de avanzar.
