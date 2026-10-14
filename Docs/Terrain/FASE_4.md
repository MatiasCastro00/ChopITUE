# Generador de terreno forestal — Fase 4

Estado: completada y verificada. Unreal Engine 5.8.2.

## Uso

Abrir **Tools > ChopIt Terrain Generator**. El panel de parámetros y la preview comparten un separador ajustable. La preview contiene mallas dinámicas reales en una escena de editor independiente.

- Arrastrar con botón izquierdo o derecho: órbita alrededor del objetivo.
- Arrastrar con botón central: pan.
- Rueda: zoom.
- **Reset Camera**: encuadre en perspectiva del terreno completo.
- **Top**: vista superior ortográfica.
- **Isometric**: vista isométrica ortográfica.
- **Wireframe**: alterna entre malla de alambre y material iluminado.
- **Flat Shading**: activado usa normales por cara; desactivado usa Smooth. Cambia también el parámetro de sombreado que aplicará Generate.
- **Preview Polygon Detail**: detalle independiente de la preview, guardado en la configuración del editor. `Polygon Detail` sigue controlando la geometría definitiva del nivel.

Los cambios de parámetros, Random Seed y Reset Defaults actualizan la preview automáticamente. Generate/Regenerate siguen siendo las acciones que publican el terreno en el nivel. La cámara se conserva al cambiar el relieve o sombreado y se reencuadra cuando cambian las dimensiones.

## Construcción y coherencia

`FChopItTerrainPreview` posee un `FPreviewScene` sin escena de física y componentes `UDynamicMeshComponent` transitorios, sin colisión ni navegación. Nunca crea actores de terreno en el nivel. El generador y el constructor de mallas son los mismos que usa el actor definitivo.

El debounce es de 250 ms desde el último cambio. Un solo trabajo de CPU se ejecuta en ThreadPool; captura copias de los parámetros y no accede a UObjects. Los resultados se publican en el hilo del editor. Una revisión descarta resultados antiguos si cambió la configuración durante el cálculo. Mientras calcula, mantiene visible la geometría anterior. Los errores también conservan la preview previa y muestran el diagnóstico.

Se conserva la resolución completa del heightmap para mantener las mismas alturas, media y distribución que el terreno definitivo. Al cambiar solamente detalle o sombreado, se reutiliza el heightmap inmutable en memoria. Por eso, resoluciones extremas del heightmap siguen consumiendo CPU y memoria aunque el detalle visual sea bajo.

`SChopItTerrainPreviewViewport` integra el viewport de Unreal, los controles y la sincronización con `UChopItTerrainEditorSettings`. `FChopItTerrainPreviewViewportClient` controla la cámara y los modos de render. La subida de geometría al render sigue ocurriendo en el hilo del editor; puede producir una pausa breve con detalle máximo. Cerrar la pestaña espera un trabajo activo antes de liberar la escena, para evitar ejecutar código después de descargar el módulo.

## Verificación

Compilaciones Win64 Development de Editor y Game correctas. Se verificaron las doce pruebas del terreno: la suite inicial pasó once y detectó un problema en la preparación del render fuera de pantalla del test. Después de corregir esa preparación, las pruebas de apertura/cierre de pestaña y datos/cámara pasaron de nuevo; la prueba final de render también pasó.

- `Build/Reports/TerrainPhase4/index.json`: suite completa inicial, once éxitos y fallo inicial de render.
- `Build/Reports/TerrainPhase4Preview/index.json`: apertura/cierre de pestaña y datos/cámara correctos; detección inicial incorrecta del color de wireframe.
- `Build/Reports/TerrainPhase4Render/index.json`: render final correcto en los cuatro modos. El test detecta las líneas negras del wireframe por contraste con el fondo.

`ChopIt.Terrain.Preview.DataAndCamera` verifica debounce, prioridad del último cambio, reutilización del heightmap/componentes, igualdad exacta con los datos definitivos, muestreo de los vértices, ausencia de cuerpos físicos, conservación ante parámetros inválidos y controles mediante eventos simulados de mouse. `ChopIt.Terrain.Preview.Render` usa el mismo cliente de viewport de la herramienta y produce capturas reales en `Saved/Terrain/Phase4_Smooth.png`, `Phase4_Flat.png`, `Phase4_Top.png` y `Phase4_Wireframe.png`. Las cuatro capturas se inspeccionaron visualmente.

No se realizó una sesión manual con mouse dentro de la pestaña acoplada. La navegación se verificó mediante eventos de entrada simulados y el renderizado se comprobó con el cliente real del viewport. Los artefactos de captura y reportes permanecen locales e ignorados por Git.

## Para continuar

Fase 5: personalización avanzada del generador, capas independientes y controles de relieve. Reutilizar el flujo de revisión/debounce de la preview para los parámetros nuevos y mantener la generación pura en C++. Esperar `siguiente` antes de avanzar.
