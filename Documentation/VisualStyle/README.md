# Estilo visual de ChopIt

## Auditoría técnica

- Motor: Unreal Engine 5.8 (validado en 5.8.2).
- Render: Deferred DX12 / SM6; Forward Shading no está habilitado.
- Base conservada: Lumen GI/reflections, Virtual Shadow Maps, luz direccional y skylight móviles, Sky Atmosphere y Exponential Height Fog. Ray tracing, Substrate y static lighting están desactivados.
- Integraciones preservadas: el ciclo día/noche mantiene el control del color e intensidad de la luz direccional; la cámara conserva el sistema de oclusión de follaje y sus rigs. No se reemplazaron mapas ni cámaras. Las nueve instancias de material conocidas del blockout sí se reparentaron deliberadamente al nuevo master después de auditar sus dependencias; sus rutas permanecen intactas.
- Custom Depth + Stencil está habilitado con `r.CustomDepth=3`. El material maestro declara soporte Nanite.

## Assets

- Material maestro: `/Game/ChopIt/Presentation/VisualStyle/Materials/M_LowPoly_Master_Authored`
- Instancias de ejemplo: `MI_LowPoly_Forest`, `MI_LowPoly_Industrial`, `MI_LowPoly_Magic`
- Textura Point/Nearest: `/Game/ChopIt/Presentation/VisualStyle/Textures/T_PixelChecker_8`
- Nueve texturas del usuario, incluida `Log.png`: `/Game/ChopIt/Presentation/VisualStyle/Textures/Authored`; fuentes reproducibles en `SourceArt/VisualStyle/Materials`.
- Post proceso: `/Game/ChopIt/Presentation/VisualStyle/Materials/M_PP_ChopItVisualStyle`
- Outline robusto: `M_OutlineOverlay` y las instancias `MI_Outline_Global`, `MI_Outline_Player`, `MI_Outline_Enemy`, `MI_Outline_Interactable`
- Presets: `DA_VisualPreset_Megabonk`, `DA_VisualPreset_MachineParty`, `DA_VisualPreset_Hybrid`
- Mapa demo: `/Game/ChopIt/World/Maps/L_VisualStyleDemo`
- API runtime/Blueprint: `UChopItVisualStyleComponent` en `Source/ChopIt/Public/Visual`
- Generación determinista: `UnrealEditor-Cmd ChopIt.uproject -run=ChopItBootstrap -VisualStyle`

## Uso del material low-poly

Crear una Material Instance de `M_LowPoly_Master_Authored` y asignarla sólo a los meshes deseados. Para pixel art usar `Filter = Nearest`, evitar mip smoothing indeseado y alinear `TexturePixelResolution` a la grilla original. Los parámetros exponen textura base, vertex color, tint, color steps, contraste, saturación y 2–4 bandas de luz (3 por defecto).

El maestro authored es `Unlit` con bandas de iluminación propias basadas en la normal. Esto evita que la luz direccional cálida destruya la paleta pintada; el postprocesado, la niebla y las sombras de contacto del escenario siguen aportando profundidad.

Asignación activa: `Grass4` al terreno, `Grass` al sendero, `Wood` a la cabaña y elementos de madera, `Log` a troncos, `Leafs` a copas, `Stone` a adoquines/techo e interactuables y `Stone2` a roca. `Grass2` y `Grass3` quedan como instancias alternativas `MI_Ground_Fern` y `MI_Ground_Mossy`.

## Contorno global y contornos selectivos

Toda geometría opaca o masked recibe `MI_Outline_Global`: un inverted hull negro verdoso de 1.25 unidades que dibuja una silueta fina sin tocar HUD, texto, cielo ni transparencias. Está serializado en los mapas generados para verse en el viewport y el controlador lo aplica también a actores creados durante el juego. El control `Global Intensity` puede retirarlo en runtime.

| Stencil | Destino | Color por defecto |
| --- | --- | --- |
| 1 | Jugador/personajes | naranja |
| 2 | Enemigos | rojo |
| 3 | Interactuables | cian |

El jugador, `AChopItEnemyCharacter` y actores con `IChopItInteractable` se registran automáticamente. Para otro Blueprint, llamar `Set Actor Outline` en `UChopItVisualStyleComponent` con stencil 1–3. Esos overlays de color sustituyen el negro en los elementos de gameplay y conservan Custom Stencil para integraciones futuras.

## Presets y control runtime

`AChopItGameState` posee un `UChopItVisualStyleComponent`; Hybrid es el preset inicial. Nodos Blueprint disponibles:

- `Apply Preset`: Megabonk, Machine Party o Hybrid con transición.
- `Set Global Intensity`: intensidad global de 0 a 1.
- `Trigger Damage Degradation`: pulso temporal de grano, viñeta y distorsión.
- `Set Danger Degradation`: degradación continua por peligro/horno/noche.
- `Restore Base Preset`: vuelve suavemente al preset base.
- `Set Actor Outline`: alta/baja de un actor en el stencil selectivo.

El componente reacciona automáticamente al daño. Noche, élites y muerte elevan la degradación y luego vuelven al preset base. Comandos QA: `ChopIt.Visual.Preset`, `ChopIt.Visual.Intensity` y `ChopIt.Visual.Damage`. En línea de comandos: `-ChopItVisualPreset=Hybrid`, `-ChopItVisualIntensity=1`.

## Post proceso, UI y niebla

El componente reutiliza el volumen unbound de previsualización con exposición automática y bias controlado por preset, bloom contenido, AO ligero, motion blur nulo y aberración nativa nula. En UE 5.8 el color de escena del blendable generado no es fiable en esta ruta, por lo que el material queda como asset experimental pero se elimina de los volúmenes activos para evitar una salida negra. La paleta oscura vive en los materiales authored, la iluminación y el bias; el outline usa geometría. El HUD/UMG queda completamente fuera del efecto.

El muestreo Point se ancla al viewport y a una resolución virtual, por lo que no nada con la cámara. La niebla de profundidad coloreada existe pero está desactivada en los tres presets; Exponential Height Fog sigue siendo la autoridad atmosférica. Hybrid/Megabonk usan ambiente teal y Machine Party aporta la variante beige/marrón para interiores industriales.

## Coste y límites

- Ruta activa: materiales Unlit con bandas low-poly, un volumen nativo ligero y un overlay masked por mesh; no hay lecturas fullscreen adicionales.
- Los contornos añaden una pasada de inverted hull sobre geometría opaca/masked. Los colores selectivos reutilizan esa misma pasada.
- Scanlines, distorsión continua y post-fog están desactivados por defecto.
- Objetivo: PC gama media/baja a 1080p. El coste escala con screen percentage; validar con `ProfileGPU` en hardware objetivo. 4K será sensiblemente más caro.
- La dirección estilizada de luz de cada instancia es control artístico y no sigue automáticamente cada luz local dinámica; las luces y sombras reales continúan superpuestas.
- Geometría muy fina puede requerir 1.5–2 px. Translucencia requiere authoring específico si debe escribir Custom Depth.

## Evidencia y verificación

Capturas 1280×720 obtenidas desde el mismo mapa real `L_Startup` y la misma cámara:

- [Antes de la corrección](Captures/Startup_BeforeFix.png)
- [Hybrid final, texturas + post + outline selectivo](Captures/Startup_Hybrid_OutlineVerified.png)
- [Megabonk](Captures/Demo_Megabonk.png)
- [Machine Party](Captures/Demo_MachineParty.png)
- [Hybrid — Editor](Captures/Demo_Hybrid_Final.png)
- [Hybrid — build cocinado](Captures/Packaged_Hybrid_Final.png)
- [Hybrid final — `L_Startup` empaquetado con outline](Captures/Startup_Hybrid_PackagedVerified.png)
- [Hybrid — materiales authored integrados](Captures/Startup_AuthoredTextures_Final.png)
- [Hybrid — materiales authored en build cocinado](Captures/Startup_AuthoredTextures_Packaged.png)
- [Hybrid oscuro final — outline global y niebla teal](Captures/Startup_DarkGlobalOutline_Final.png)

La prueba `ChopIt.Presentation.VisualStyle.Assets` valida presencia de assets, dominios de material, outline masked, reparentado de las instancias reales, filtrado Point, uso Nanite, orden de presets y efectos opcionales desactivados. El `BuildCookRun` Win64 final terminó con éxito y el ejecutable staged volvió a cargar `L_Startup` y producir la captura verificable anterior.
