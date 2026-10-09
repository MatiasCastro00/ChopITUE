# Ítems pasivos

## Arquitectura

El módulo `ChopItCombat` contiene `UChopItItemComponent`, `UChopItItemDataAsset`,
`UChopItItemEffect`, `UChopItItemEventSubsystem`, `UChopItItemLootSubsystem` y los
cuatro efectos de `ChopItPassiveEffects`. No depende de World, AI, presentación ni tienda.
`AChopItCharacter` crea el componente de ítems como subobjeto nativo.

El componente guarda definiciones, stacks e instancias de efectos. Duplica cada
template con el componente como Outer; los Data Assets nunca reciben estado de run.
Agregar antes de BeginPlay es válido: la activación ocurre en BeginPlay. Eliminar
el último stack o terminar el componente desactiva efectos y limpia timers/delegates.
Las mutaciones recursivas desde los callbacks de activación/desactivación se rechazan;
usar `OnInventoryChanged` para reaccionar a operaciones ya completadas.

Los stacks usan una suma geométrica compartida: `Base * (1 - Decay^N)/(1 - Decay)`.
Decay 1 usa `Base * N`; cero stacks produce cero. No hay límite de diseño;
el contador es int32 y rechaza operaciones que lo desborden. Decay por defecto: 0.5.

## Crear contenido

1. Content Browser → Miscellaneous → Data Asset → **ChopItItemDataAsset**.
2. Guardar dentro de `/Game/ChopIt/Items` (subcarpetas permitidas).
3. Asignar un `ItemId` único y permanente, nombre, descripción, rareza, tags e icono.
   El icono es una referencia soft; la UI decide cuándo cargarlo.
4. En `Effects`, agregar instancias inline de los tipos de efecto y configurar
   `BaseValue`. Una definición puede combinar varios efectos.
5. Configurar `StackDecay`, `SpawnWeight`, `RequiredItemIds`, `RequiredTags` y
   `BlockedTags`. Todos los IDs/tags requeridos deben cumplirse; cualquier tag
   bloqueado excluye el ítem. Peso cero lo excluye del loot, pero permite otorgarlo.
6. Ejecutar Validate Assets desde el editor. Evitar IDs duplicados: el AssetManager
   utiliza `ChopItItem:ItemId` como identidad, independiente del nombre del archivo.

Los cuatro ejemplos se generan con `-run=ChopItItemExamples` y se guardan como
assets reales. El commandlet sólo crea assets faltantes, preservando ajustes existentes:

| Asset / ID | Efecto | BaseValue |
|---|---|---|
| DA_Item_Heartwood / Heartwood | Regeneración | 1 HP/s |
| DA_Item_VampireStick / VampireStick | Robo de vida | 0.10 |
| DA_Item_GenerousLog / GenerousLog | Rendimiento de madera | 0.10 |
| DA_Item_Wormwood / Wormwood | Infestación | 0.10 |

Para Reina de la Colonia, configurar `RequiredItemIds = [Wormwood]`. No se agrega
una reina vacía al loot: su efecto queda para una futura implementación.

## Dar y consultar ítems

Blueprint: jugador → **Get Item Component** → **Add Item** (Data Asset, Count).
Usar **Remove Item**, **Get Stack Count**, **Get Items**, **Clear Items** según necesidad.
`AddItem` no evalúa requisitos del loot: también sirve para recompensas explícitas.

```cpp
#include "Items/ChopItItemComponent.h"
#include "Items/ChopItItemLootSubsystem.h"

Player->GetItemComponent()->AddItem(ItemData, 1);
auto* Loot = GetGameInstance()->GetSubsystem<UChopItItemLootSubsystem>();
auto* Reward = Loot->GetRandomItem(Player->GetItemComponent(), {EChopItItemRarity::Rare});
if (Reward) Player->GetItemComponent()->AddItem(Reward);
```

Blueprint: **Get Game Instance Subsystem** → **ChopItItemLootSubsystem**.
`GetAvailableItems`, `GetRandomItem`, `GetRandomItems` comparten `CanItemAppear`.
Rarities vacío acepta todas. Muestreo por peso, sin reemplazo por defecto; los
requisitos se evalúan contra el inventario actual, no contra otros resultados de la tirada.
Un pool vacío retorna nullptr/array vacío. `RefreshCatalog` descubre assets mediante
el AssetManager existente y carga las definiciones sin cargar iconos. Es una carga
sincrónica cacheada: llamar durante una pantalla de carga; la primera consulta la
ejecuta si hace falta. Refrescar después de crear assets durante una sesión de editor.

## Crear efectos

Derivar de `UChopItItemEffect` en C++ (o Blueprint), declarar parámetros estáticos
EditAnywhere y estado runtime Transient. Configurar `Events` únicamente con los
eventos necesarios. Implementar `OnActivated`, `OnStacksChanged`, `HandleEvent`
y `OnDeactivated`; llamar `GetStackedValue()` para el valor efectivo.
No guardar estado en Definition ni usar Tick. Limpiar timers, modificadores y
suscripciones en OnDeactivated. Regeneración usa HP/s multiplicado por intervalo.
Los modificadores de madera reutilizan `UChopItCombatStatsComponent::WoodYield`.

## Eventos y semántica

`FChopItItemEventContext` contiene destinatario, fuente, objetivo, posición, cantidad,
crítico, ejecución, generación, definición e ID de contenido. `Recipient == nullptr`
se reserva para eventos globales, como día/noche. El subsistema es la frontera entre
sistemas; cada componente filtra destinatarios y cada efecto sus eventos.
La UI escucha `OnInventoryChanged` y `OnItemEvent`; ningún efecto conoce widgets.

- Health publica daño real (limitado a la vida restante), recibido y realizado;
  muertes usan la ruta existente una sola vez. TargetKind distingue enemigo/árbol
  sin casts a sus clases. Las notificaciones de muerte pueden preceder DamageDealt.
- TreeDestroyed significa vida agotada, cuando empieza la caída, no eliminación
  física del actor. TreeHit incluye golpes letales y ejecuciones.
- Recogida: `Transferred`/`Remainder` siguen midiendo unidades del pickup;
  `BonusUnits` mide madera adicional. El evento incluye ambas. Fracciones se
  acumulan en cargo; bonus entero que no cabe se descarta. Refunds y suministros
  de prueba no generan bonus ni WoodCollected.
- WoodDelivered se emite al confirmar la cuota; cada tronco en vuelo conserva
  su cargo de origen aunque el jugador se aleje antes de llegar.
- La tienda de armas existente publica ItemPurchased con ContentId y precio en
  Amount. Una futura tienda de pasivos debe cobrar, llamar AddItem y luego publicar
  ItemPurchased con Item/ContentId. AddItem no presupone una compra.
- DayStarted/NightStarted se publican al entrar a la fase; no se reproducen al
  adquirir un ítem a mitad de fase.

Infestación es independiente de HP y pertenece a cada instancia de efecto del
jugador. Observa cambios de vida en objetivos afectados para ejecutar incluso
cuando otro daño cruza el umbral. La ejecución usa Health, preservando recompensas,
y está marcada `bExecution` para evitar procs recursivos de infestación/robo de vida.
`InfestationChanged` sirve para barras. `InfestationExecuted` lleva objetivo, posición
y la infestación acumulada antes de limpiarla; una futura reina puede consultar
objetivos cercanos y llamar `ApplyInfestation` con una fracción calculada.
Quitar el ítem limpia la infestación de esa instancia; reducir stacks conserva
la infestación ya aplicada. Referencias débiles no mantienen objetivos destruidos vivos.

## Archivos

Creados: `ChopItCombat/{Public,Private}/Items/*`,
`ChopItEditor/{Public,Private}/Commandlets/ChopItItemExamplesCommandlet.*`,
`ChopItTests/Private/Items/ChopItItemsTest.cpp`, esta guía y cuatro Data Assets.

Modificados: configuración de AssetManager, Build.cs de Combat, DamageTypes,
HealthComponent, CombatStatsComponent, Character, EnemyCharacter, Tree,
WoodCargoComponent, DeliveryZone, CycleStateMachineComponent y ShopComponent.

Pruebas: `ChopIt.Items` (stacks/requisitos y runtime), más regresión de los sistemas
de combate, cosecha, economía, ciclo y tienda afectados.

## Validación realizada

Unreal Engine 5.8.2, ChopItEditor Win64 Development: compilación y enlace completos
correctos. Se ejecutaron **22 pruebas, 22 exitosas**, usando el filtro
`ChopIt.Items+ChopIt.Phase2+ChopIt.Phase3+ChopIt.Phase4+ChopIt.Phase5+ChopIt.Phase7`.
Reporte: `Saved/ItemTestReport/index.json`; log: `Saved/ItemTests.log`.
El commandlet generador finalizó con cero errores y cero advertencias.

Como el editor abierto bloqueaba las DLL, la validación se hizo en
`Saved/ItemValidation`, con una copia de Source/Config y un enlace al Content real.
El 5 de octubre de 2026, con Unreal cerrado, se completó también la compilación y
el enlace del proyecto original (ChopItEditor Win64 Development). Las DLL originales
ya incluyen las clases nuevas. No se realizó una prueba visual en PIE; las pruebas
de efectos utilizan un mundo runtime de automatización.

## Cofre del boss

La muerte del guardián de cada ciclo y la del boss final publican `OnEliteDefeated`
desde el encuentro existente. `AChopItGameState` crea un
`AChopItBossRewardChest` en el suelo bajo el enemigo. No se añade un segundo sistema
de bosses ni se altera la transición de fase.

El cofre aparece con escala animada, luz y `NS_BossChest_Appear`. Mientras espera
flota suavemente, respira con un pulso de escala y luz y emite destellos periódicos
con `NS_BossChest_Idle`; el movimiento afecta sólo a los visuales, de modo que el
área de interacción permanece fija. Acercarse a menos
de 155 cm lo abre automáticamente: la tapa sube y `NS_BossChest_Open` lanza
partículas. El HUD pausa el mundo y dibuja un cofre en pantalla con haces de luz
que avanzan de gris a verde, azul y dorado hasta la rareza ya sorteada. Después
el ítem sale del cofre, crece y muestra su ilustración, nombre, descripción y stacks.
La pantalla permanece pausada hasta pulsar Enter o Esc después de la revelación.
El cofre otorga el ítem una sola vez antes de la animación, porque los timers del
mundo no avanzan durante la pausa. Si el inventario rechaza la operación, la UI
se cancela y el cofre vuelve a estar disponible.

La suerte del jugador es `BaseLuckPercent` en `AChopItCharacter` (0 por defecto),
editable en los valores del personaje y modificable mediante
`EChopItCombatStat::Luck`. El cofre multiplica el `SpawnWeight` por
`(1 + LuckPercent / 100)^Tier`, con Tier 0/1/2/3 para común/poco común/raro/legendario.
La suerte se limita a 1000% para mantener pesos finitos. Sólo los ítems con
`SpawnWeight > 0` y requisitos cumplidos participan; la animación refleja el
resultado sorteado, nunca decide el premio después de mostrar las luces.

La UI muestra un cofre 3D hueco mediante `AChopItChestRevealScene`, una cámara
SceneCapture2D dedicada y un render target de 1024 × 1024. La tapa gira sobre una
bisagra trasera y abre progresivamente. Los haces y partículas emisivos nacen en
su interior: al pasar de gris a verde, azul y dorado aumentan en cantidad y tamaño,
con espirales en las rarezas superiores y un estallido Niagara legendario. El
cofre flota y vibra, la luz interior pulsa y la cámara se acerca al revelar el premio.
El HUD actualiza la escena con tiempo real, incluida la simulación manual Niagara,
para mantener la animación durante la pausa. La escena se destruye al cerrar la UI.
`Scripts/BuildChestRevealMaterial.py` genera el material emisivo editable
`M_ChestReveal_Light`; una referencia del actor asegura su inclusión en el cocinado.

Los visuales se encuentran en `/Game/ChopIt/Items/Chest`: tres materiales y tres
sistemas Niagara editables. `Scripts/BuildBossChestAssets.py` sólo genera los assets
que falten y conserva los ajustes hechos en el editor. Para ejecutar el script desde
el editor se habilitó `CascadeToNiagaraConverter` sólo para Editor; el juego usa
Niagara en runtime. Los iconos configurados en los Data Assets aparecen en la revelación;
si falta una referencia, la UI intenta encontrar `T_Stick_<ItemId>`. Las 24 imágenes
originales en `Content/ChopIt/Art/Sticks` tienen nombres de archivo en inglés y
se importan como `T_Stick_*` mediante `Scripts/ImportStickIcons.py`. Los cuatro
ítems actuales tienen ilustración: Heartwood, VampireStick y Wormwood coinciden
con su nombre inglés, y GenerousLog usa la única ilustración de tronco suministrada,
HollowLog. La referencia `Icon` del Data Asset tiene prioridad, así que cada foto
se puede cambiar desde Details. El script guarda también las referencias en cada
Data Asset.

## Prueba de ítems y aparición del boss

Durante una partida, **F6** pausa el juego y abre el catálogo de los 24 Data Assets. Flechas arriba
y abajo seleccionan; Re Pág/Av Pág cambian de página; **Enter** concede un stack
del seleccionado y se puede repetir; **F6** o **Esc** cierran y reanudan la partida. El catálogo incluye
ítems con `SpawnWeight=0`, útiles para probar fotos y futuros efectos. Si `Effects`
está vacío, la ficha lo indica y otorgar el ítem solo cambia sus stacks. A la
derecha del HUD se ve el inventario actual con iconos y cantidades.

El encuentro Elite busca suelo alrededor del jugador en varios puntos cercanos.
Si no encuentra una superficie válida, usa la altura del jugador y deja que el
spawn ajuste la colisión. Si aún falta el jugador o la definición, reintenta cada
segundo mientras la fase Elite siga activa. El log avisa de esos retrasos y confirma
el spawn del guardián.

Validación actual: los cinco tests `ChopIt.Items` aprobaron, incluidos el catálogo
de 24 assets, los pesos de suerte por rareza, el cofre y el spawn del guardián sin suelo WorldStatic. La compilación
de `ChopItEditor` en `Saved/ItemValidation` fue correcta. Después de cerrar el editor
se compiló y enlazó también el proyecto principal. La presentación visual en PIE
queda para inspección manual.

La prueba adicional `ChopIt.Items.ChestRevealScene` requiere renderizado (sin
`-NullRHI`), verifica capturas de las cuatro rarezas y guarda los PNG en
`Saved/ChestRevealTier0.png` a `Saved/ChestRevealTier3.png`. Aprobó con
`-RenderOffscreen`; también se compiló el proyecto principal con la escena 3D.

## Palo Mandrágora y vida del boss

`DA_Item_MandrakeStick` es un ítem poco común que puede salir del cofre y se
puede otorgar con F6. Cada muerte causada por el jugador tiene un 10% de
probabilidad de invocar una mandrágora en el lugar donde cayó el enemigo. Los
stacks aumentan esa probabilidad mediante `StackDecay` (15% con dos stacks
cuando vale 0,5). La mandrágora dura seis segundos; su grito espacializado
produce 8 de daño por segundo en un radio de 350 unidades, con pulsos cada
0,25 s. El daño se atribuye al jugador y pasa por `UChopItHealthComponent`,
por lo que activa robo de vida e infestación y afecta al boss como a otros
enemigos. Se excluyen el jugador, los árboles y los objetivos no enemigos.
`Duration`, `Radius` y `DamagePerSecond` se pueden ajustar en el efecto del DA.
`Scripts/BuildMandrakeAssets.py` crea los materiales y el sonido editables.

La versión con FBX usa `SM_Mandrake` y `T_Mandrake_Albedo` en
`/Game/ChopIt/Items/Mandrake`. Los slots `Palmera` y `Lagrimas` tienen materiales
separados: cuerpo con atlas original y agua translúcida con UV animadas,
franjas descendentes, normales animadas y refracción suave. `NS_Mandrake_SonicWaves`
emite ondas expansivas; `NS_Mandrake_TearSplash` salpica en ambas bases;
`NS_Mandrake_Emerge` añade motas al aparecer. Los enemigos alcanzados reciben
`NS_Mandrake_Confusion` y una reducción de velocidad del 35%. Los pulsos renuevan
el estado sin multiplicar la penalización; expira un segundo después del último
impacto y se limpia al morir. `SlowMultiplier` y `SlowDuration` son editables
en el efecto del DA. La confusión es visual: no cambia las decisiones de la IA.

Los originales están en `SourceArt/Mandrake`. El FBX binario proporcionado fue
rechazado por el importador de Unreal; `Scripts/InspectMandrakeFbx.py` recupera
sus nodos y arrays a `Mandrake_Recovered.fbx` ASCII, conservando geometría, UV y
los dos materiales. `Scripts/ImportMandrakeModel.py` importa la versión recuperada
y normaliza su escala de construcción a centímetros, con normales recalculadas.
`Scripts/BuildMandrakeVFX.py` crea los materiales/Niagara faltantes y actualiza
la descripción del DA; ejecutar ambos scripts con `-ExecutePythonScript` en
editor completo, no con el commandlet `-run=pythonscript` (requieren subsistemas
del editor y Niagara requiere Slate). Los assets
generados quedan editables y no se reconstruyen si ya existen.
`ChopIt.Items.MandrakeVisual` comprueba la compilación de Niagara y guarda
`Saved/MandrakePreview.png` usando un RHI de renderizado.
La captura precarga los shaders de estos materiales y verifica que no se esté
usando el material de respaldo; también comprueba partículas vivas en los cuatro
sistemas Niagara. El halo de estrellas sobre la mandrágora en esta captura es
una muestra del efecto que se coloca sobre los enemigos durante la partida.

`BP_MandrakeFXPreview` es un actor de muestra inofensivo: buscá “Mandrake FX
Preview” en Place Actors, arrastralo al nivel y ejecutá Play para ver el modelo,
el grito, las ondas, las lágrimas, las salpicaduras y las motas. No inflige daño
ni aplica ralentización. `Scripts/CreateMandrakePreview.py` crea el Blueprint a
partir de la clase nativa `AChopItMandrakePreview`.

Durante el encuentro Elite, el HUD muestra abajo, centrados, el nombre y la
vida actual/máxima del boss. Lee el componente de salud del Elite activo y
oculta la barra cuando este muere o sale del encuentro.

La prueba `ChopIt.Items.MandrakeScream` mata un enemigo con una invocación
garantizada, verifica la aparición en el lugar de muerte y comprueba el daño,
la exclusión de árboles/objetivos lejanos y la acumulación de infestación en
un objetivo con tipo Enemy. También comprueba que el Data Asset real conserva
el icono, tiene el efecto asignado y participa en el loot. Pasó en el proyecto
principal, junto con las otras cinco pruebas `ChopIt.Items` ejecutables con
`-NullRHI`; `ChopItEditor` compiló y enlazó correctamente.

Validación del FBX/Niagara: `ChopItEditor Win64 Development` compiló correctamente
y las ocho pruebas `ChopIt.Items` pasaron con `-RenderOffscreen`, incluidas
`MandrakeScream` (ralentización, renovación, expiración y limpieza al morir) y
`MandrakeVisual` (shaders reales y partículas vivas). Log:
`Saved/Logs/MandrakeFinalItems.log`.
