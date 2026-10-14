# Fase 8: presets y mejoras de interfaz

## Perfiles incluidos

La ventana **Tools > ChopIt Terrain Generator** ofrece siete estilos en **Presets > Selected Preset**:

| Preset | Intención |
|---|---|
| Flat Forest | Bosque amplio, bajo relieve y llanuras extensas. |
| Gentle Forest Hills | Colinas suaves con espacios transitables. |
| Rolling Hills | Colinas anchas, ondulaciones y poca planicie. |
| Forest Valleys | Valles marcados y depresiones moderadas. |
| Steep Woodland | Tramos empinados aislados entre zonas boscosas. |
| Wet Lowlands | Terreno bajo con agua automática cercana al 25 % de superficie. |
| Forest Clearings | Claros frecuentes sobre áreas relativamente suaves. |

**Apply Preset** modifica los controles del panel y actualiza la preview. Mantiene el Seed, tamaño, resolución y configuración de malla actuales; pulsa **Generate** para aplicar el resultado al actor. Los presets son configuraciones editables, no campos de altura pregrabados.

## Presets personales

Establece **Presets > Preset File** y usa **Save Preset** o **Load Preset**. El archivo JSON guarda configuración de terreno y malla con una versión. La carga valida los controles antes de aplicarlos. Las máscaras pintadas y los datos de altura se guardan con el terreno mediante las funciones de la fase 7, no dentro del preset.

## Controles de parámetros

Expande **Presets and Parameter Tools**. El buscador filtra el panel de propiedades; también puedes buscar rutas completas mediante **Parameter Tools > Selected Parameter**. Selecciona una categoría para **Randomize Category**, o usa **Randomize Settings** para variar todos los controles numéricos y booleanos compatibles. La aleatorización prueba cada cambio contra la validación del generador y conserva el último estado válido. Las curvas y listas de capas requieren edición manual y no se aleatorizan automáticamente.

**Lock Parameter** conserva el valor seleccionado durante presets, aleatorización y **Reset Defaults**. Puedes bloquear varios parámetros uno por uno; la lista **Locked Parameters** persiste en las preferencias del editor. **Reset Parameter** restaura solo el control seleccionado a su valor original. Si el valor predeterminado haría inválida la combinación actual, la operación se rechaza sin cambiar los ajustes; primero modifica los parámetros relacionados.

Los controles de estas acciones aparecen en un panel plegable para dejar espacio a la preview. El bloque **Terrain Statistics** muestra muestras, dimensiones, elevación, nivel de agua, fracción sumergida, chunks, pinceladas, medias de máscaras y cobertura de regiones para el actor generado seleccionado. Se actualiza después de generar, seleccionar, cargar o editar máscaras.

## Verificación

Los objetivos `ChopItEditor` y `ChopIt` compilaron en Win64 Development. La suite `ChopIt.Terrain` pasó 23 pruebas, 0 fallos, según `Build/Reports/TerrainPhase8Final/index.json`. La prueba nueva genera los siete presets, comprueba que Steep Woodland produce más pendiente media que Flat Forest, prueba el objetivo de agua de Wet Lowlands, valida bloqueos y aleatorización por categoría y global, restaura un parámetro y comprueba el intercambio JSON. La prueba del editor verifica además que los bloqueos persisten al guardar y recargar la configuración y que el panel recibe estadísticas del terreno.
