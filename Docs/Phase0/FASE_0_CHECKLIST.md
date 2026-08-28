# Fase 0 — configuración y verificación

## Objetivo

Obtener una base UE 5.8.2 reproducible, cocinable y sin decisiones heredadas accidentalmente del template. Esta fase no implementa gameplay.

## Archivos y assets creados

- Módulos runtime: `ChopItCore`, `ChopItCombat`, `ChopItWorld`, `ChopItAI`, `ChopItMeta`, `ChopIt`, `ChopItPresentation`.
- Módulos editor: `ChopItEditor`, `ChopItTests`.
- Core: Asset Manager, Developer Settings, logs, tags nativos y nombres/canales de colisión.
- Editor: commandlet `ChopItBootstrap`.
- Tests: `ChopIt.Smoke.ProjectConfiguration`.
- Assets: `/Game/ChopIt/World/Maps/L_Startup` y `L_Dev_Sandbox`, generados por commandlet como mapas no particionados.
- Repo: `.gitignore`, `.gitattributes`, `.editorconfig` y Git LFS local.

## Configuración dentro del editor

Después de abrir el proyecto:

1. En **Project Settings > Maps & Modes**, verificar `L_Dev_Sandbox` como Editor Startup Map y `L_Startup` como Game Default Map.
2. En **Project Settings > Asset Manager**, verificar que `Map` escanea `/Game/ChopIt/World/Maps`.
3. En **Project Settings > Gameplay Tags**, verificar importación desde config y tags `State.*` nativos.
4. En **Project Settings > Collision**, verificar canales `ChopItEnemy`, `ChopItHarvestable`, `ChopItProjectile`, `ChopItPickup`, `ChopItDeliveryZone`, `ChopItInteraction` y los seis presets.
5. En **Plugins**, verificar Enhanced Input, Niagara, Audio Modulation y MetaSound. Modeling Tools queda limitado al editor y Android File Server queda deshabilitado.
6. En **Rendering**, verificar ray tracing y Substrate desactivados. Lumen permanece como baseline de Fase 1.

## Pruebas concretas

Ejecutar desde PowerShell:

```powershell
./Build/CI/VerifyPhase0.ps1 -EngineRoot D:/UE_5.8
```

El script debe:

1. Compilar `ChopItEditor Win64 Development`.
2. Ejecutar el commandlet idempotente de mapas.
3. Ejecutar todos los tests `ChopIt.Smoke` con `NullRHI`.
4. Compilar `ChopIt Win64 Development`.
5. Cocinar contenido para Windows.

El verificador usa una caché DDC local bajo `Saved/LocalDDC` y desactiva Zen Store para que el resultado no dependa de la configuración global del equipo.

## Criterios de aceptación

- UE 5.8.2 y toolchain quedan registrados.
- Editor y Game compilan desde una invocación limpia.
- Existen los dos mapas propios y el proyecto no arranca en un mapa de Engine.
- Smoke tests pasan sin error.
- Cook de Windows termina con código 0.
- El grafo de módulos no contiene ciclos y cada módulo carga.
- Git LFS reconoce `*.uasset` y `*.umap` como `filter=lfs` y `lockable`.
- No hay lógica de movimiento, combate, economía o ciclo implementada.

## Riesgos pendientes

- La referencia exacta de hardware y la decisión final de cámara se cierran en Fase 1.
- Lumen se conserva provisionalmente; su coste se medirá en el blockout.
- El pipeline es compatible con CI, pero el proveedor remoto y runner licenciado de Unreal se seleccionan cuando exista remoto del repositorio.
