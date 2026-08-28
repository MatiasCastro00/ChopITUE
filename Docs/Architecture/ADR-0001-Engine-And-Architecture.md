# ADR-0001: motor y base arquitectónica

- **Estado:** aceptado; decisión de cámara cerrada en ADR-0002
- **Fecha:** 2026-08-26
- **Decisores:** equipo de ChopIt

## Contexto

ChopIt parte de un proyecto C++ vacío generado con el template Open World. El juego objetivo es single-player para PC, con cámara cenital, mapa acotado, hordas y estética low-poly. Se requiere que la base pueda crecer por contenido sin concentrar reglas en Blueprints o en una clase manager.

## Decisiones

1. Se fija **Unreal Engine 5.8.2**, changelist `56702186`, compatible changelist `55116800`, rama `++UE5+Release-5.8`.
2. Se usa C++ para contratos/reglas y Blueprints para composición, contenido y presentación.
3. Se adopta el grafo de módulos:

   ```text
   ChopItCore
      <- ChopItCombat <- ChopItWorld
      <- ChopItCombat <- ChopItAI
      <- ChopItMeta
      <- ChopIt (composition root)
      <- ChopItPresentation (presentation leaf)
      <- ChopItEditor / ChopItTests
   ```

4. Se usa un sistema propio de estadísticas/efectos y Gameplay Tags. GAS no se habilita en el alcance single-player actual.
5. El MVP usa mapas regulares, no World Partition. `L_Startup` y `L_Dev_Sandbox` son mapas propios y no contienen lógica de Level Blueprint.
6. Se deshabilitan hardware ray tracing y Substrate. Se mantiene Lumen por software como baseline evaluable; cualquier cambio exige captura comparativa.
7. Enhanced Input, Niagara, Audio Modulation y MetaSound son plugins oficiales fijados. Modeling Tools es solo de editor. Android File Server queda deshabilitado explícitamente para evitar configuración móvil accidental.
8. Git + Git LFS + locks es el control de versiones inicial. Perforce se reconsidera si aumenta el equipo de contenido.
9. La cámara comienza como perspectiva casi ortográfica; la comparación contra ortográfica real se cerró en Fase 1 mediante ADR-0002.
10. Pooling, HISM, Mass, PCG y StateTree no forman parte del baseline. Se introducen solo con necesidad o evidencia de profiling.

## Consecuencias

- Las dependencias tienen una dirección explícita y presentation no puede convertirse en fuente de reglas.
- El coste inicial es mantener varios módulos pequeños; evita mover UClasses y cambiar rutas más adelante.
- La versión del motor no se actualiza durante una feature. Una actualización usa rama dedicada, build, smoke, cook y rollback etiquetado.
- Cooperativo o red sería un cambio arquitectónico explícito; no se paga esa complejidad por adelantado.

## Verificación

- `Build/CI/VerifyPhase0.ps1` compila Editor/Game, crea assets bootstrap, ejecuta `ChopIt.Smoke` y cocina Win64.
- `ChopIt.Smoke.ProjectConfiguration` verifica descubrimiento de mapas por Asset Manager, settings, tags, módulos cargados y canales/presets de colisión.
- Los mapas y binarios de contenido están cubiertos por LFS y locking.
