# Cadena V2

`UChopItRopeComponent` es la autoridad de posiciones, longitud desplegada, contactos y tensión. `UChopItTetherPathComponent` conserva las consultas y la depuración como adaptador; no calcula otro recorrido.

## Simulación

- Reloj de 120 Hz, captura acotada de cuadros lentos y subdivisión del movimiento de los extremos.
- Restricciones de tensión con masa por unidad de longitud, compliance XPBD y amortiguación. La gravedad no depende de la masa.
- Barridos de partículas, cápsulas de segmentos completos y cobertura temporal de segmentos que rotan. Las correcciones vuelven a pasar por colisión.
- Los pasos inválidos se revierten, incluyendo topología y longitud. No se reconstruye el recorrido cuando aparece visibilidad directa.
- Máquina fija; fuerzas y torques agregados y limitados por cuerpo de Chaos. Los canales Pawn, Enemy, Projectile, Pickup, DeliveryZone y Chain no producen contactos.
- Cadena irrompible, sin autocolisión ni nudos.

Referencia de las restricciones y la estimación de fuerza: [XPBD, Macklin et al.](https://mmacklin.com/xpbd.pdf).

## Carrete y presentación

La longitud cambia solamente por la salida de la máquina. El primer segmento puede tener longitud parcial. Las vueltas conservan sus partículas y consumen longitud disponible. La recogida acepta únicamente estados que siguen respetando contactos y longitud.

El receptor del jugador ejecuta la simulación en PostPhysics, después de CharacterMovement y de actualizarse los cuerpos de Chaos. Las fuerzas resultantes se acumulan para el siguiente paso de cuerpos rígidos. La máquina tiene al receptor como prerrequisito y presenta después los eslabones; la cámara se actualiza en PostUpdateWork. Cada eslabón sigue un único segmento físico, evitando cuerdas visuales que corten las esquinas.

La longitud almacenada es `MaxChainLength - GetRopeLength()`. `ChainLinkCount` ya no define la longitud ni la discretización: se conserva para cargar presets antiguos. Los parámetros principales son `PhysicalSegmentLength`, `LinearMassDensity`, `StretchCompliance`, `CableParticleDiameter`, `CableSolverIterations`, velocidad/aceleración del carrete, holgura e histéresis.

## Migración y verificación

Desde PowerShell en la raíz del proyecto:

```powershell
./Build/CI/VerifyChainV2.ps1 -MigrateAssets
```

Compila Editor y juego, ejecuta la migración específica y `ChopIt.Chain`. El script comprueba el JSON de resultados porque Unreal puede devolver código cero aunque una prueba falle. `-SkipBuild` permite reutilizar los binarios.

La migración `-run=ChopItBootstrap -ChainV2` actualiza exclusivamente el preset `DA_Chain_Default`, la malla `SM_ChainLink_V2` y `L_Test_ChainLab`. Conserva el máximo configurado, las referencias de Blueprints y las mallas personalizadas. Añade dos cajas de igual forma con masas de 10 y 200 kg y una pared de 2 cm. No regenera otros mapas.

Resultados de automatización: `Saved/ChainV2Reports/index.json`. Registro: `Saved/Logs/ChainV2-Tests.log`.

En una ejecución del Editor con -game, el comando de consola `ChopIt.Chain.Capture` espera a que se estabilice la simulación y guarda una vista de la cadena en `Saved/ChainV2Visuals`. La variante `ChopIt.Chain.Capture exit` cierra esa ejecución después de capturar, para QA desatendida. Este comando no se incluye en el juego compilado.

## Validación realizada

Nueve pruebas `ChopIt.Chain.V2` cubren: material y reloj, dos vueltas en ambos sentidos y desenrollado, segmentos completos/pared delgada/enemigos, máximo y adaptador, suelo y obstáculo desplazado, fuerzas y masas en Chaos, dos esquinas con pendiente, CharacterMovement con salto y cuadro lento, y límite máximo con una vuelta conservada, movimiento lateral y regreso.

En la ejecución del 22/09/2026: sin cruces en los recorridos comprobados; la consulta independiente verifica todos los segmentos con una tolerancia de penetración de 0,5 cm. Extensión máxima por segmento en las vueltas: 0,20 cm. Coincidencia del anclaje del personaje: 0,000 cm. El objeto de 10 kg alcanzó 20 veces la velocidad del de 200 kg con la misma fuerza.

Coste por cuadro medido en el recorrido de dos vueltas (máquina local, Development Editor): mediana/P95 de 3,00/11,96 ms a 30 FPS, 1,53/6,08 ms a 60 FPS y 0,68/2,92 ms a 120 FPS. No es un presupuesto garantizado para cualquier mapa; el coste depende de longitud desplegada, geometría y contactos. Si el presupuesto de resolución se agota, el movimiento se reduce al último estado válido.

Los escenarios aislados de CharacterMovement/Chaos utilizan `GamePreview` con simulación de física habilitada: conserva movimiento y física de juego sin instanciar subsistemas de renderizado de Water innecesarios en el mundo temporal.

## Regresión al regresar sobre el mismo camino

Al retirar partículas por el carrete, `SetNumZeroed` conservaba valores existentes de masa inversa. El nuevo índice del extremo podía heredar la masa de una partícula interior: el solver desplazaba el anclaje y la integración del personaje eliminaba velocidad al corregirlo. `RefreshMasses` ahora reinicializa todas las masas inversas y asigna masa únicamente a las partículas interiores.

El carrete mide el avance del extremo respecto de los contactos físicos actuales, en su orden a lo largo de la cadena, excluyendo el suelo como soporte de cadena floja. La dirección del último tramo doblado ya no convierte un regreso en una orden de entregar más material. Esta consulta no modifica la forma ni elimina contactos; recoger sigue sujeto a las comprobaciones de colisión del solver.

En un recorrido libre se reutiliza la cadena floja antes de entregar más material. `ChopIt.Chain.V2.CharacterRoundTrip` comprueba tres idas y vueltas con CharacterMovement real, gravedad y suelo, recogida de material sin acumulación entre recorridos, continuidad del movimiento y colisión de todos los segmentos.

Validación de esta corrección: Editor y juego compilados; las once pruebas pasan. En la reproducción previa al arreglo de masas, el regreso terminaba en X=870,93 cm; después termina en X=324,85 cm, manteniendo Z=90,15 cm y sin rechazos ni cuadros limitados por presupuesto en los tres recorridos. Es una reproducción automatizada; queda la comprobación interactiva del mapa del usuario.

## Presupuesto de simulación

### Regreso a velocidad de juego (650 cm/s)

La prueba anterior usaba 300 cm/s y un preset transitorio con aceleración de carrete de 8000 cm/s². No cubría el atasco a velocidad de juego con `DA_Chain_Default` (650 cm/s y 3200 cm/s²). `ProductionReturn` carga ese asset sin modificarlo y comprueba la vuelta a 30, 60 y 120 FPS.

La reproducción mostró extensiones locales de centésimas de centímetro que, al sumarse en más de 270 tramos, excedían el límite global. Los pasos convergían localmente pero se rechazaban por longitud total. `SolveTotalLength` resuelve ahora también esa restricción dentro de cada iteración, con anclajes fijos, correcciones barridas y validación completa; no se aumentó la tolerancia.

La restricción global interviene cuando el residuo acumulado excede el límite que ya existía, después de resolver las curvas locales. Se mantiene la integración conjunta del carrete y los contactos. Las correcciones del personaje eliminan solo la parte de velocidad rechazada, en la dirección del desplazamiento bloqueado; al alcanzar el máximo de material se conserva la proyección radial para permitir movimiento lateral.

`ChopIt.Chain.ReplayReturn exit` permite repetir un recorrido de ida y vuelta por X en una ejecución independiente del mapa guardado con su pawn y Blueprint reales. No modifica assets. Registra velocidad, estado físico, colisión y resultado del recorrido; `exit` cierra esa ejecución al terminar.

Validación del 22/09/2026, posterior a la reproducción del atasco: las doce pruebas pasan, incluido `ProductionReturn` a 30/60/120 FPS con el preset guardado. También se ejecutaron los mapas con renderizado y `BP_ChopItCharacter_C`: recorrido X en `L_Test_ChainLab` y recorrido Y (`ReplayReturn y exit`) en `L_Test_Progression`. Ambos regresos registraron 650 cm/s, 0/54 muestras lentas y todos los segmentos libres en la consulta independiente de 0,5 cm. Logs: `Saved/Logs/ChainLab-Replay.log` y `Saved/Logs/ChainMain-ReplayY.log`; capturas en `Saved/ChainV2Visuals`.

El recorrido X inicial en Progression no era un corredor libre: encontraba la cabaña y árboles; no se usa como evidencia de un regreso libre. Su log se conserva en `ChainMain-Replay.log`, incluido un fallo de despeje al final de esa interacción. Esa interacción no está cubierta por los dos recorridos libres aprobados.

La simulación tiene ahora un presupuesto de CPU por cadena y cuadro, configurable con `ChopIt.Chain.FrameBudgetMs` (32 ms por defecto; 0 únicamente para mediciones sin límite). Es una protección contra picos, no un objetivo de coste habitual ni una garantía de FPS. Las consultas de colisión y la validación final son operaciones indivisibles: puede haber un pequeño exceso sobre el presupuesto.

Al agotarlo, se cancelan subdivisiones y reintentos, se valida el progreso alcanzado y se restaura el último estado válido si no cumple las restricciones. Se descarta la deuda de tiempo para evitar que un cuadro lento provoque una espiral de recuperación. La velocidad válida del carrete se conserva; el jugador queda limitado por el extremo aceptado. En sobrecarga puede avanzar menos durante ese cuadro.

Los movimientos menores que el margen del barrido final ya están cubiertos por ese volumen: se omiten sus barridos temporales redundantes sin omitir la comprobación del segmento completo. Se conservan la convergencia, las tolerancias y el sistema de contactos originales.

Con el extremo del jugador quieto, el carrete reduce a la mitad su incremento de recogida después de un intento rechazado y recupera gradualmente ese factor al resolver incrementos completos. En movimiento, limita lo que recoge por cuadro a la mitad del avance directo del jugador hacia la máquina; los giros laterales no autorizan recoger cadena. Así la cadena no se tensa alrededor del jugador solo porque se acortó la distancia en línea recta. La entrega mantiene su velocidad configurada. Si el presupuesto se agota sin aceptar ningún paso, se frena la predicción inercial incompatible, conservando la capacidad del motor de entregar cadena.

`ChopIt.Chain.V2.FrameBudget` añade una décima prueba: corte forzado, conservación de la forma, recuperación y 90 cuadros de una cadena de 21 m sobre el suelo con cambios bruscos de dirección y cuadros de 250 ms. Este caso también comprueba un presupuesto más estricto de 12 ms y valida todos los segmentos mediante consultas independientes.

Validación posterior a la corrección: las diez pruebas pasan con el presupuesto predeterminado activo. En el recorrido de vueltas, mediana/P95 de 2,28/7,59 ms a 30 FPS, 1,14/4,54 ms a 60 FPS y 0,63/2,44 ms a 120 FPS. El regreso desde el máximo enrollado terminó con error de extremo de 0,000 cm. Estas mediciones corresponden a los escenarios automatizados locales; no sustituyen una medición interactiva del mapa del usuario.

## Atasco al tocar obstáculos con cadena disponible

La reproducción inicial de `ProductionObstacleWalk` se detenía durante la segunda vuelta al poste con más de 21 m almacenados, 340 cuadros lentos y P95 de 32,03 ms a 60 FPS. El motor volvía a pedir longitud directa al acercarse el jugador a la máquina, aunque las vueltas aún consumían material. El reintento repetido agotaba el presupuesto de simulación. Log anterior: `Saved/Logs/ChainObstacle-Baseline.log`.

El destino del carrete ahora integra el movimiento firmado del extremo contra el mismo conjunto de contactos y conserva como mínimo la longitud guiada más la holgura. La histéresis regula la velocidad del motor, sin borrar pequeños cambios acumulados del destino. La velocidad interna de recogida se actualiza al aplicar su límite por movimiento del jugador: no se calcula una frenada de una velocidad de recogida que nunca se ejecutó. La recogida continúa pasando por el solver físico; no se eliminan vueltas ni se reconstruye la cadena.

Los contactos se indexan por segmento para evitar comparar cada nuevo contacto con todos los de la cadena. Los límites de la región barrida incluyen ambos estados completos del segmento. Se reutiliza la convergencia únicamente cuando los errores local y total ya cumplen los límites existentes y dejan de mejorar durante tres iteraciones. No se modificaron el radio, la tolerancia de penetración, el límite de extensión ni los 32 ms de protección por cuadro.

Para componentes estáticos simples con un único collider convexo, el descarte previo utiliza planos que contienen los vértices cocinados de Chaos, transformados a mundo y ampliados 0,01 cm por redondeo. Solo descarta un segmento o su triángulo barrido si todos sus extremos están fuera del mismo plano por más del radio de consulta. Los casos restantes siguen usando las consultas originales de Unreal. Geometrías complejas, instancias, compuestos, elementos transformados y escalas no admitidas conservan el descarte por caja. La validación final independiente no usa este filtro. Así se evita buscar repetidamente colisiones imposibles dentro de la caja del poste.

Con material disponible, el barrido completo del extremo permite mantener los pasos de 120 Hz a 650 cm/s; antes se subdividían sistemáticamente a 240 Hz. Los desplazamientos mayores siguen subdividiéndose y el límite máximo conserva la resolución fina anterior para permitir movimiento lateral. Se mantienen las consultas mundiales de Unreal y la validación independiente de todos los segmentos.

`ProductionObstacleWalk` usa CharacterMovement, suelo, gravedad y el preset guardado, con dos tipos de poste: cápsula y el cilindro convexo real del mapa. Ejecuta dos vueltas completas y dos vueltas inversas siguiendo el mismo camino, a 30/60/120 FPS. Comprueba movimiento, enrollado antes de invertir, desenrollado, todos los segmentos y los límites físicos de longitud. La comprobación de coste usa el mismo presupuesto por tiempo simulado (16 ms por cuadro a 60 FPS, escalado por el número de pasos fijos), además del límite de CPU de producción.

Validación final de esta revisión: Editor y juego compilados; las catorce pruebas pasan, incluida `TreeHitClearance` y el orden definitivo en PostPhysics. El recorrido ampliado completa 16/16 puntos en los seis escenarios. P95 de simulación de 17,99 / 9,63 / 5,65 ms para la cápsula y 23,01 / 9,09 / 5,32 ms para el cilindro a 30 / 60 / 120 FPS. Ambos registran cero cuadros lentos en las tres tasas. Son costes de la cadena por cuadro, no FPS totales del juego. Resultado completo: `Saved/ChainV2Reports/index.json` y `Saved/Logs/ChainV2-Tests.log`.

`ChopIt.Chain.ReplayReturn post exit` reproduce dos vueltas en el mapa guardado `L_Test_ChainLab` con su pawn real, registra velocidad y P95, y verifica los segmentos independientemente. No guarda cambios en el mapa.

La reproducción renderizada final del laboratorio completó 9/9 puntos, sin penetraciones en la comprobación de 0,5 cm y con ambas vueltas conservadas (ángulo acumulado 10,970 rad), 0/86 muestras lentas y P95 de 19,93 ms, frente a unos 33 ms y frenadas persistentes antes del descarte convexo. Pueden seguir existiendo picos de coste: el presupuesto no garantiza los FPS totales de todos los mapas. Evidencia: `Saved/Logs/ChainLab-PostReplay.log`.

El filtro espacial conserva también la región de la consulta inicial. Si una corrección sale de esa región, vuelve a consultar el mundo completo: la lista local de obstáculos nunca justifica descartar geometría exterior que no se recopiló.

## Contactos con árboles golpeados y en caída

La repetición X en Progression identificó dos penetraciones adicionales. El pulso visual del golpe escalaba `PhysicsRoot`, agrandando también la cápsula del árbol un 10 % (24 % en críticos). El efecto ahora escala solamente `TrunkMesh`. `TreeHitClearance` reproduce golpes normales y críticos junto a una cadena y comprueba que la cápsula permanezca igual, todos los segmentos sigan libres y el pulso visual se restaure.

Al caer el árbol, Chaos movía su cápsula después de resolver la cadena. La simulación del receptor pasa a PostPhysics y la presentación de la máquina depende explícitamente de ella. Así los contactos usan la posición del árbol del cuadro actual, antes de dibujar y actualizar la cámara.

La repetición renderizada del mapa principal después de ambas correcciones termina con `clear=1`, incluidos todos los cuadros de golpes y caída, frente al `clear=0` anterior. Registra 5/54 muestras lentas en un recorrido que también encuentra obstáculos físicos del personaje; no se presenta como un corredor libre. Evidencia: `Saved/Logs/ChainMain-ContactReview.log`.
