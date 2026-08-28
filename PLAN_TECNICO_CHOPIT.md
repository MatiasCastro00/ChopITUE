# Plan técnico de ChopIt

**Estado:** propuesta para aprobación, sin implementación de gameplay  
**Fecha:** 26 de agosto de 2026  
**Plataforma inicial:** PC, single-player  
**Motor recomendado:** Unreal Engine 5.8, fijado a un hotfix concreto

---

## 1. Resumen ejecutivo de la arquitectura

ChopIt se construirá como un juego **data-driven, orientado a componentes y dividido por dominios**, con C++ como propietario de reglas y contratos, y Blueprints como capa de composición, presentación y contenido. La arquitectura tendrá cinco límites fundamentales:

1. **Configuración estática:** `Primary Data Assets`, tablas y curvas describen personajes, armas, mejoras, pactos, enemigos, días, cuotas y spawns.
2. **Estado persistente:** un `GameInstanceSubsystem` y un `SaveGame` versionado conservan desbloqueos y moneda meta mediante identificadores estables.
3. **Estado de la run:** `GameState` y `PlayerState` poseen día, fase, cuota, dinero, XP, pactos y carga que deben sobrevivir a una reposición del Pawn.
4. **Combate momentáneo:** Actor Components del jugador y enemigos poseen salud, estadísticas calculadas, armas, efectos y capacidades temporales.
5. **Presentación:** UI, audio, iluminación, Niagara, cámara y feedback observan eventos; no deciden reglas.

El ciclo se gobernará mediante una máquina de estados explícita (`Day`, `Dusk`, `Night`, `Elite`, `CycleResolution`, `VictoryChoice`, `InfiniteEntry`, `Death`, `Finished`). El combate automático compondrá estrategia de selección de blanco, patrón de ataque, movimiento del proyectil e impacto, evitando una subclase por cada arma. Las modificaciones de estadísticas se recalcularán desde fuentes inmutables y removibles, nunca alterando el valor base.

No habrá `GameManager` global, lógica central en Level Blueprint, búsquedas mundiales frecuentes ni dependencias desde gameplay hacia UI/VFX. El diseño comienza simple —Actors sin Tick, timers, overlaps y consultas espaciales limitadas— y reserva pooling, instancing avanzado o Mass para cuando Unreal Insights demuestre su necesidad.

## 2. Suposiciones y preguntas realmente bloqueantes

### Suposiciones de trabajo

- El repositorio actual es una base C++ casi vacía, asociada localmente a UE 5.8; no se considera que el mapa Open World, ray tracing o Substrate del template sean decisiones finales.
- Una run normal dura 20–30 minutos; cada día dura aproximadamente 2–3 minutos incluyendo noche y élite. Los valores finales serán curvas de diseño.
- Cámara de perspectiva con FOV bajo y brazo largo como primera opción para conservar sombras, profundidad y compatibilidad; en Fase 1 se compara contra ortográfica real.
- Movimiento con teclado/mouse y gamepad desde Fase 1. El ataque principal es automático; interacción explícita para palanca, tienda, elecciones y confirmaciones.
- Una unidad de tronco, dinero y moneda meta es entera. Daño y estadísticas internas usan precisión suficiente y solo redondean al presentar o convertir economía.
- El hub es seguro respecto a spawns normales, pero no una invulnerabilidad explotable durante élites salvo que diseño lo confirme.
- La run normal usa un único mapa persistente acotado; no necesita World Partition para el MVP.
- El ajuste dinámico no aumenta la dificultad por detectar que el jugador va fuerte. El poder estimado se usa para telemetría, composición segura y protección contra picos imposibles.

### Bloqueantes

No hay una decisión bloqueante para producir ni comenzar la Fase 0. Antes de implementar se pedirá únicamente aprobación del plan. Si no se indica lo contrario, se fijarán UE 5.8, Win64, Git LFS, cámara de perspectiva casi ortográfica y soporte gamepad desde el inicio.

## 3. Versión recomendada de Unreal Engine y plugins

### Motor

Recomiendo **Unreal Engine 5.8 con el último hotfix disponible al iniciar la Fase 0**. Epic publicó 5.8 el 23 de junio de 2026 y la presenta como la última versión mayor de UE5 planificada, con soporte posterior para correcciones y regresiones. El proyecto ya fue generado contra una instalación `D:/UE_5.8`, por lo que comenzar ahí evita una migración inmediata. Se fijará una versión exacta para todo el equipo y las actualizaciones posteriores pasarán por rama de prueba, build, cook y smoke test.

No se basará producción en funciones 5.8 marcadas Beta o Experimental. Mesh Terrain, Toon Shader experimental, MCP experimental y otras novedades no son necesarias para este juego.

Fuentes oficiales: [anuncio de UE 5.8](https://www.unrealengine.com/news/unreal-engine-5-8-is-now-available), [notas de versión 5.8](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-release-notes).

### Plugins y módulos oficiales

| Elemento | Decisión | Momento | Motivo |
|---|---|---:|---|
| Enhanced Input | Requerido | Fase 0–1 | Acciones, contextos, gamepad y futura reasignación. |
| Niagara | Requerido | Fase 2 en adelante | Impactos, madera, críticos, transición y control de presupuesto visual. Viene habilitado por defecto en UE5. |
| Audio Modulation | Requerido | Fase 13, preparar antes | Mezclas dinámicas de día/noche, buses y accesibilidad. Está deshabilitado por defecto. |
| MetaSound | Requerido | Fase 13 | Capas, variaciones de pitch y parámetros de combate. |
| Common UI | Recomendado, no obligatorio para MVP | Fase 7/13 | Navegación coherente con teclado/gamepad y pantallas apilables. Si UMG simple cubre el slice, se difiere. |
| Functional Testing | Solo desarrollo | Fase 0 | Mapas y pruebas funcionales automatizadas. No forma parte de la lógica Shipping. |
| Audio Insights | Solo editor, opcional | Fase 13 | Diagnóstico de mezcla y voces si el audio lo requiere. |
| Modeling Tools Editor Mode | Opcional, editor | Ya está habilitado | Útil para blockout; no es dependencia de runtime. |
| Gameplay Abilities | No habilitar | — | Se recomienda sistema propio; se evita coste y complejidad innecesarios. |
| Mass Entity, PCG, StateTree, Game Features | No requeridos inicialmente | Revisar tras profiling/necesidad | Añadirlos antes de medir o necesitar su flujo sería sobrearquitectura. |

`GameplayTags`, `AIModule`, `NavigationSystem`, `UMG`, `Slate`, `DeveloperSettings`, `AssetRegistry` y `DataValidation` se tratarán como módulos de motor cuando cada dominio los necesite; no todos son plugins. La documentación oficial confirma que Gameplay Tags son etiquetas jerárquicas y requieren el módulo `GameplayTags`, y que Audio Modulation ofrece control dinámico de volumen y pitch: [Gameplay Tags](https://dev.epicgames.com/documentation/unreal-engine/using-gameplay-tags-in-unreal-engine), [Audio Modulation](https://dev.epicgames.com/documentation/en-us/unreal-engine/audio-modulation-overview-in-unreal-engine), [Niagara](https://dev.epicgames.com/documentation/en-us/unreal-engine/getting-started-in-niagara-effects-for-unreal-engine).

## 4. Decisión entre GAS y sistema propio

### Comparación

| Criterio | GAS | Sistema propio propuesto |
|---|---|---|
| Replicación y predicción | Excelente, pero sin valor inmediato en single-player | No se implementa complejidad de red no solicitada |
| Atributos y efectos apilables | Muy completo | Se implementa solo el subconjunto necesario |
| Curva de aprendizaje/depuración | Alta; ASC, AttributeSets, GameplayEffects, Abilities, cues | Menor; componentes y specs alineados al dominio |
| Armas automáticas composables | Posible, con bastante ceremonia | Natural mediante estrategias y definiciones |
| Riesgo de sobrearquitectura | Alto para este alcance | Controlado |
| Futuro cooperativo | Ventaja clara | Requeriría rediseño parcial |

### Recomendación

Usar un **sistema propio**, apoyado en Gameplay Tags pero no en GAS. Incluye:

- `UChopItStatComponent`: bases y agregación determinista de modificadores.
- `UChopItHealthComponent`: salud, daño, curación y muerte.
- `UChopItStatusEffectComponent`: efectos temporales y permanentes de la run.
- `FChopItModifierSpec` + `FChopItModifierHandle`: fuente, operación, magnitud, duración y política de stacking.
- `FChopItDamageSpec`: fuente, tags, daño, crítico, impulso y efectos.

La API estará basada en specs y tags, no en conocimiento de armas concretas; esto conserva una posible migración futura sin imitar GAS internamente. GAS se reconsideraría solo si se aprueba cooperativo/red, decenas de habilidades activas con cancelación compleja o efectos que requieran predicción.

## 5. Módulos principales del proyecto

La separación objetivo es física desde Fase 0 para evitar mover UClasses y cambiar rutas más adelante:

| Módulo | Responsabilidad | Dependencias permitidas |
|---|---|---|
| `ChopItCore` | IDs, tipos, tags nativos, interfaces, settings, utilidades puras | Core de Unreal |
| `ChopItCombat` | Stats, salud, daño, armas, proyectiles, XP y efectos | `ChopItCore` |
| `ChopItWorld` | Árboles, troncos, carga, cabaña, cuota, venta, tienda e interacción mundial | `ChopItCore`, `ChopItCombat` |
| `ChopItAI` | Enemigos, cerebros, anchors, spawns y director de dificultad | `ChopItCore`, `ChopItCombat` |
| `ChopItMeta` | Perfil, migraciones, desbloqueos y servicio de guardado | `ChopItCore` |
| `ChopIt` | Composition root, Gameplay Framework, run y ciclo | Core + módulos de gameplay/meta |
| `ChopItPresentation` | HUD, presentadores, audio, VFX, feedback y accesibilidad | Puede observar todos; nadie depende de él |
| `ChopItEditor` | Validadores de assets y herramientas de autoría | Todos, solo Editor |
| `ChopItTests` | Automation Specs y helpers de prueba | Todos, no Shipping |

Regla: dependencias apuntan hacia contratos estables; presentación es hoja del grafo. No habrá dependencias circulares. Los módulos pueden exponer una API mínima mediante sus macros y mantener implementaciones privadas.

### Control de versiones

- Equipo pequeño: **Git + Git LFS + locks** para `*.uasset`, `*.umap` y binarios fuente grandes. Perforce es alternativa si el equipo de contenido crece o necesita locking central más robusto.
- Versionar `Source`, `Config`, `Content`, `.uproject`, plugins propios, tests y documentación.
- Ignorar `Binaries`, `DerivedDataCache`, `Intermediate`, `Saved`, `.vs` y soluciones generadas.
- Activar “one asset per responsibility”; evitar que múltiples diseñadores editen un único Data Table gigante.
- Branches cortas, PR obligatorio para C++/Config, build automatizado Development Editor + cook Win64 y chequeo de redirects/referencias.
- No actualizar versión de motor en una rama de feature; usar rama de migración y etiqueta de rollback.

## 6. Árbol propuesto de carpetas

```text
Source/
  ChopItCore/{Public,Private}/
    Data/ Interfaces/ Settings/ Tags/ Types/
  ChopItCombat/{Public,Private}/
    Damage/ Effects/ Experience/ Stats/ Targeting/ Weapons/
  ChopItWorld/{Public,Private}/
    Economy/ Harvest/ Hub/ Interaction/ Pickups/
  ChopItAI/{Public,Private}/
    Director/ Enemies/ Spawning/
  ChopItMeta/{Public,Private}/
    Profile/ Save/ Unlocks/
  ChopIt/{Public,Private}/
    Framework/ Player/ Run/ Cycle/
  ChopItPresentation/{Public,Private}/
    Accessibility/ Audio/ Feedback/ UI/ VFX/
  ChopItEditor/
    Validation/
  ChopItTests/
    Automation/ Fixtures/

Content/ChopIt/
  Core/{Curves,DataTables,Tags}/
  Characters/{Definitions,Blueprints,Animations}/
  Weapons/{Definitions,Blueprints,Projectiles,VFX,Audio}/
  Upgrades/{Definitions,Icons}/
  Pacts/{Definitions,Icons,VFX,Audio}/
  Enemies/{Definitions,Blueprints,AI,Animations}/
  World/{Forest,Trees,Pickups,Hub,Maps}/
  Economy/{Days,Quotas,Shop}/
  UI/{Widgets,Icons,Fonts}/
  Audio/{MetaSounds,Music,Mixes,SFX}/
  VFX/{Niagara,Materials}/
  Tests/{Maps,Data}/
  Developer/<UserName>/
```

Nada de lógica en Level Blueprint. Los mapas colocan actores de composición, volúmenes, navegación e iluminación.

## 7. Lista de clases C++ principales

### Framework y ciclo

- `AChopItGameMode`, `AChopItGameState`, `AChopItPlayerController`, `AChopItPlayerState`, `AChopItCharacter`.
- `UChopItRunStateComponent`, `UChopItCycleStateMachineComponent`.
- `UChopItCyclePhase` y fases concretas: `Day`, `Dusk`, `Night`, `Elite`, `Resolution`, `VictoryChoice`, `InfiniteEntry`, `Death`, `Finished`.

### Core y datos

- `UChopItAssetManager`, `UChopItDeveloperSettings`.
- Interfaces `IChopItDamageable`, `IChopItTargetable`, `IChopItInteractable`, `IChopItRunContext`, `IChopItGameplayCueSink`.
- Data Assets `UChopItCharacterDefinition`, `UChopItWeaponDefinition`, `UChopItUpgradeDefinition`, `UChopItPactDefinition`, `UChopItEnemyDefinition`, `UChopItDayDefinition`, `UChopItSpawnProfile`, `UChopItMetaUnlockDefinition`.

### Combate

- `UChopItStatComponent`, `UChopItHealthComponent`, `UChopItStatusEffectComponent`.
- `UChopItWeaponInventoryComponent`, `UChopItAutoCombatComponent`, `UChopItWeaponInstance`.
- `UChopItTargetingStrategy`, `UChopItAttackStrategy`, `UChopItProjectileMotionStrategy`, `UChopItImpactStrategy`.
- `UChopItTargetingSubsystem`, `AChopItProjectile`, `UChopItExperienceComponent`, `UChopItOfferComponent`.

### Mundo y economía

- `UChopItInteractionComponent`, `AChopItTree`, `AChopItLogPickup`, `UChopItWoodCargoComponent`.
- `AChopItCabinHub`, `AChopItQuotaMachine`, `UChopItQuotaComponent`, `AChopItDeliveryZone`, `AChopItSellZone`.
- `UChopItEconomyComponent`, `UChopItShopComponent`, `UChopItForestRegistrySubsystem`.

### Enemigos, dificultad y pactos

- `AChopItEnemyCharacter`, `UChopItEnemyBrainComponent`, `AChopItSpawnAnchor`.
- `UChopItDifficultyDirectorComponent`, `UChopItSpawnSchedulerComponent`.
- `UChopItPactComponent`, `UChopItPactOfferComponent`.

### Persistencia y presentación

- `UChopItSaveSubsystem`, `UChopItMetaProgressionSubsystem`, `UChopItProfileSaveGame`, `UChopItGameUserSettings`.
- `AChopItHUD`, `UChopItHUDPresenter`, `UChopItAudioDirectorSubsystem`, `UChopItFeedbackSubsystem`, `UChopItWorldPresentationComponent`.
- `UChopItAssetValidator` y validadores específicos en `ChopItEditor`.

## 8. Responsabilidad exacta de cada clase

| Clase/grupo | Responsabilidad exacta; lo que no debe hacer |
|---|---|
| `GameMode` | Crea/completa una run y valida transiciones terminales. No almacena datos que UI deba leer. |
| `GameState` | Propietario observable del estado de run/ciclo y sus componentes. No presenta UI ni audio. |
| `PlayerController` | Input, foco de interacción, apertura/cierre de pantallas y pausa local. No calcula daño/economía. |
| `PlayerState` | Dinero de run, XP/nivel, pactos, slots y estadísticas del jugador que sobreviven al Pawn. |
| `Character` | Locomoción, cámara, puntos de montaje y composición de componentes momentáneos. |
| `RunStateComponent` | Día, modo, seed, tiempo total, resultado y resumen de run. |
| `CycleStateMachineComponent` | Única autoridad de fase; valida transiciones, entra/sale de estados y emite eventos. |
| `CyclePhase` | Reglas acotadas de una fase: condiciones de entrada/salida, timers y comandos válidos. |
| `AssetManager` | Descubre/carga Primary Assets y bundles por ID estable. No balancea ni instancia gameplay arbitrariamente. |
| `DeveloperSettings` | Referencias bootstrap: datos globales, mapa, colisiones y defaults seguros. |
| Interfaces Core | Contratos pequeños sin casts: recibir daño, ser objetivo, interactuar, consultar run y recibir cues. |
| Data Assets | Definiciones inmutables de diseño. Nunca guardan estado de una run. |
| `StatComponent` | Bases + modificadores + cache invalidada; devuelve valores finales deterministas. |
| `HealthComponent` | Salud actual/máxima, invulnerabilidad, daño, curación y delegate de muerte. |
| `StatusEffectComponent` | Duración, stacking y remoción de efectos; aplica modificadores por handle. |
| `WeaponInventoryComponent` | Slots, exclusividad, compra/equipamiento y vida de `WeaponInstance`. |
| `AutoCombatComponent` | Agenda ataques de armas habilitadas; no conoce tipos concretos. |
| `WeaponInstance` | Estado runtime por arma: nivel, cooldown, estrategias y handles de mejoras. |
| Estrategias de arma | Selección, emisión, movimiento e impacto intercambiables/configurables. |
| `TargetingSubsystem` | Registro espacial de objetivos y consultas filtradas; reemplaza búsquedas globales. |
| `Projectile` | Ejecuta un spec ya resuelto, colisión y vida útil; no consulta Data Assets cada frame. |
| `ExperienceComponent` | XP acumulada, umbrales y solicitudes de level-up. |
| `OfferComponent` | Genera opciones deterministas y aplica una selección válida. No dibuja la pantalla. |
| `InteractionComponent` | Detecta foco y llama `TryInteract` sobre la interfaz con contexto explícito. |
| `Tree` | Salud/cosecha, transición a caída física y emisión de recompensa una sola vez. |
| `LogPickup` | Representa unidades de madera, magnetismo y transferencia no bloqueante. |
| `WoodCargoComponent` | Capacidad y cantidad transportada; no paga cuota ni vende. |
| `ForestRegistrySubsystem` | Registro/consulta de recursos y futura materialización HISM; sin lógica de economía. |
| `CabinHub` | Referencia espacial y composición de máquina/camioneta/tienda; sin reglas globales. |
| `QuotaComponent` | Objetivo del día, progreso, completado y rechazo/aceptación de madera. |
| `QuotaMachine` | Interacción de palanca y fachada audiovisual; delega reglas a cuota/ciclo. |
| `DeliveryZone` | Extrae madera automáticamente con ritmo controlado y destino inyectado. |
| `SellZone` | Convierte únicamente excedente cuando la cuota está completa. |
| `EconomyComponent` | Saldo entero de run y transacciones auditables; no conoce widgets. |
| `ShopComponent` | Catálogo, precios, validación de compra y entrega al inventario. |
| `EnemyCharacter` | Representación, movimiento y componentes de combate del enemigo. |
| `EnemyBrainComponent` | Estrategia simple por arquetipo: persecución, zona, soporte o guardia. |
| `DifficultyDirectorComponent` | Calcula presupuesto previsto y composición habilitada desde datos. |
| `SpawnSchedulerComponent` | Convierte presupuesto en spawns escalonados respetando densidad/performance. |
| `PactComponent` | Pactos aceptados, Maldición total y handles persistentes durante la run. |
| `PactOfferComponent` | Condiciones, pesos, incompatibilidades, rechazo libre y selección determinista. |
| `SaveSubsystem` | Slots, escritura atómica, versionado, migraciones y fallbacks. |
| `MetaProgressionSubsystem` | Consulta/aplica desbloqueos y retención al finalizar una run. |
| `ProfileSaveGame` | DTO persistente, sin referencias a Actors/Levels. |
| `GameUserSettings` | Accesibilidad, gráficos, audio y controles locales, separado de progresión. |
| Presentadores/subsystems | Transforman eventos en UI, mezcla y cues; nunca alteran reglas. |
| Validadores | Detectan IDs duplicados, assets ausentes, tags inválidos, curvas incompletas e incompatibilidades. |

## 9. Propietario y ciclo de vida de cada estado importante

| Estado | Propietario | Vida | Persistencia |
|---|---|---|---|
| Configuración de diseño | Primary Assets/curvas/tablas cocinadas | Aplicación/build | Se versiona como contenido |
| Ajustes del usuario | `UChopItGameUserSettings` | Entre ejecuciones | Archivo de settings separado |
| Perfil/meta | `UChopItProfileSaveGame` servido por GameInstance subsystems | Entre runs/mapas | Save versionado |
| Run global | `RunStateComponent` en `GameState` | Inicio a resultado final | Snapshot opcional, no necesario para MVP |
| Fase/día/cuota | Componentes de `GameState` y máquina | Un ciclo/día | No se mezcla con perfil |
| Dinero, XP, nivel, armas, pactos | Componentes de `PlayerState` | Una run; sobrevive al Pawn | Resumen al terminar |
| Salud/efectos momentáneos | Componentes del Pawn/enemigo | Vida del Actor | Nunca meta |
| Cooldowns/proyectiles/drops | Instancias/Actors runtime | Segundos | No persistente |
| UI/audio/VFX | Controller, HUD y presentation subsystems | Local/mapa | Solo settings persistentes |

El estado de la run podrá serializarse más adelante en un DTO separado si se aprueba “continuar run”; no se introduce en el SaveGame de perfil sin necesidad.

## 10. Patrones de diseño utilizados y motivo

- **State:** fases del ciclo con entradas/salidas y transiciones válidas. Evita condicionales repartidos y timers huérfanos.
- **Strategy:** targeting, ataque, trayectoria e impacto de armas; cerebros de enemigos. Permite combinar comportamientos sin una jerarquía cartesiana.
- **Observer mediante delegates tipados:** UI/audio/VFX reaccionan a salud, cuota, fase, XP y muerte sin ser dependencias del dominio.
- **Factory localizada:** inventario crea `WeaponInstance`; director crea enemigos desde definiciones. No se añade una “Factory universal”.
- **Dependency Inversion:** World/AI consumen interfaces de Core para contexto, daño e interacción, no clases concretas del mapa.
- **Component:** capacidades reutilizables con propietario claro: salud, stats, carga, XP, armas, cuota.
- **Repository/Service acotado:** `SaveSubsystem` encapsula slots y migración; no se convierte en service locator.
- **Command:** no se usa inicialmente. Interacciones son inmediatas y validables; se introduciría solo para replay, input buffer o acciones diferidas reales.
- **Object Pool:** no es arquitectura base. Se aplica por tipo únicamente si una captura demuestra que spawn/destrucción o GC supera el presupuesto.

No se usará un event bus global opaco. Los observers se conectan a propietarios conocidos; los eventos de dominio llevan payload tipado y fuente.

## 11. Modelo de datos

Todos los contenidos extensibles usan `FPrimaryAssetId` como identidad estable, Gameplay Tags para categorías/consultas y `TSoftObjectPtr`/`TSoftClassPtr` para presentación y clases opcionales. El Asset Manager carga bundles (`Gameplay`, `UI`, `Audio`) según contexto.

### Definiciones

| Data Asset | Campos principales |
|---|---|
| `CharacterDefinition` | ID, tags, Pawn class, stats base, arma exclusiva inicial, slots inicial/máximo, reglas allowed/blocked, presentación. |
| `WeaponDefinition` | ID/tags, exclusividad, precio, stats base, estrategia de targeting/ataque, lista de comportamientos de proyectil/impacto, cue set. |
| `UpgradeDefinition` | rareza, peso, tag query, máximo de stacks, selectores de objetivo, modifier specs, texto/ícono. |
| `PactDefinition` | condiciones, peso, incompatibilidades, ventaja, desventaja, Maldición, modificadores del director, texto/VFX/audio. No hay cap global de pactos; solo incompatibilidades y políticas declaradas por contenido. |
| `EnemyDefinition` | clase/estrategia, tags, stats, coste de spawn, XP, madera opcional, límites, presentación. |
| `DayDefinition` | duración suave/dura, cuota, timings, spawn profile, élite, condiciones especiales, audio/luz. |
| `SpawnProfile` | entradas por fase/día/tag, pesos, ventanas, coste, caps y curvas de presupuesto. |
| `MetaUnlockDefinition` | ID, tipo, coste/requisito, prerequisitos y contenido concedido. |

### Tablas y curvas

- Data Tables: rarezas, precios base homogéneos, tabla de retención, texto/localización exportable y matrices de drop simples.
- Curve Tables/Float Curves: XP por nivel, cuota por día, presupuesto nocturno, escalado infinito, multiplicadores de Maldición, duración y caps.
- Primary Assets: contenido complejo o polimórfico; no se fuerza dentro de filas gigantes.

### Cálculo de estadísticas

Para cada stat:

```text
Final = Clamp((Base + ΣFlatAdd) × (1 + ΣPercentAdd) × ΠMultipliers, Min, Max)
```

- Orden fijo por operación y prioridad; `double` durante agregación, `float` al exponer gameplay.
- Economía, madera y moneda meta usan enteros; conversiones usan reglas explícitas de floor/round/ceil.
- Cada modificador tiene handle, `SourceId`, stat tag, operación, magnitud, stacking policy, prioridad y duración.
- Aplicar/quitar invalida cache y recalcula desde base; nunca se “deshace” restando un valor mutado.
- Políticas: `UniqueBySource`, `RefreshDuration`, `StackCount`, `HighestWins`, `Independent`.
- Caps se aplican al final; caps blandos, si se desean, son curvas explícitas.
- Upgrade/pacto selecciona stats por tags y slots, no por casts a armas concretas.

### Integridad

Validadores comprueban ID único, referencias blandas resolubles, tag queries posibles, incompatibilidades simétricas, valores finitos/no negativos, curvas con días 1–7, estrategia compatible y ausencia de ciclos de desbloqueo.

## 12. Flujo de eventos del ciclo completo

### Clasificación de acciones

| Tipo | Acciones |
|---|---|
| Tiempo real | Movimiento, auto-target, ataques, daño, tala, caída, drops, magnetismo, recogida, XP, spawns, iluminación y mezcla. |
| Interacción explícita | Palanca, abrir tienda, comprar, elegir mejora, aceptar/rechazar pacto, retirarse/entrar a infinito. |
| Zona automática | Máquina consume carga hasta cuota; camioneta consume excedente y acredita dinero; pickups entran por overlap/magnetismo. |

### Árbol a cuota/venta

1. AutoCombat solicita un blanco a Targeting y ejecuta ataque.
2. `Tree` recibe `DamageSpec`; Health llega a cero una sola vez.
3. Tree se desregistra como objetivo, inicia caída con física y emite `Harvested`.
4. Tras asentarse/timeout, genera `LogPickup` con N unidades y XP de árbol; el tronco ignora bloqueo de Pawn/Nav.
5. Pickup entra en radio, se magnetiza y transfiere unidades a `WoodCargoComponent`; si no caben, conserva remanente.
6. Al entrar en DeliveryZone de la máquina, la zona pide una transferencia por lotes.
7. Si cuota incompleta, `QuotaComponent` acepta hasta el remanente y emite progreso/completado. La venta permanece cerrada.
8. Cuando cuota completa, la máquina rechaza nuevas unidades y `SellZone` queda habilitada.
9. La camioneta extrae excedente, calcula valor entero desde regla del día/pactos y `EconomyComponent` registra la transacción.
10. Delegates actualizan HUD, sonido y animación; ninguna presentación modifica saldos.

### Día a día

1. `Day` fija cuota, limpia estado diario y habilita recolección.
2. Al timer suave: `Dusk`; guía a cabaña, transición visual/musical. Si falta cuota, existe gracia visible.
3. Al límite duro con cuota impaga: `Death(DevouredByMachine)`.
4. Con cuota pagada: `Night`; spawn budget crece. Tras una duración mínima, palanca habilitada.
5. El jugador puede prolongar la noche. Tirar de palanca es interacción y transiciona a `Elite`.
6. Muerte del élite: `CycleResolution`; se evalúan condiciones de Parca, resumen y siguiente día.
7. Tras día 7: `VictoryChoice`; retirarse liquida con retención alta o `InfiniteEntry` reutiliza el ciclo con escalado.
8. Muerte en cualquier fase activa cancela spawns, congela transacciones y liquida con retención de derrota.

## 13. Máquina de estados de día y noche

```text
Bootstrap -> Day -> Dusk -> Night -> Elite -> CycleResolution
              |      |       |        |              |
              |      |       |        |              +-> Day (día < 7)
              |      |       |        +-----------------> Death
              |      |       +--------------------------> Death
              |      +-- cuota impaga al hard deadline --> Death
              +------------------------------------------> Death

CycleResolution (día 7) -> VictoryChoice -> Finished/Retired
                                        \-> InfiniteEntry -> Day (día 8+, modo infinito)
Any active state -> Death -> Finished
```

Cada fase implementa `CanEnter`, `Enter`, `CanExit`, `Exit` y acepta comandos concretos; solo `CycleStateMachineComponent` cambia el estado. Un token/generación invalida callbacks tardíos de timers. `GameState` publica snapshot de fase, tiempo restante/transcurrido y flags; presentation observa.

Reglas clave:

- `Day -> Dusk`: timer suave.
- `Dusk -> Night`: cuota completa y final de transición mínima.
- `Dusk -> Death`: límite duro sin cuota.
- `Night -> Elite`: cuota completa, noche mínima cumplida y palanca válida.
- `Elite -> Resolution`: élite registrado muere; no por timeout silencioso.
- `Resolution -> VictoryChoice`: día 7 completado.
- Infinito es modo de run, no una copia de sistemas: `InfiniteEntry` cambia el modo y vuelve a estados normales con curvas extendidas.

## 14. Arquitectura del combate automático

1. Cada `WeaponInstance` posee definición, nivel, cooldown y estrategias runtime creadas una vez.
2. `AutoCombatComponent` agenda el próximo disparo con timers/scheduler. No hay Tick por arma.
3. Targeting consulta `TargetingSubsystem`, que mantiene registro espacial por celdas y tags; durante MVP puede respaldarse con overlaps espaciados, nunca `GetAllActorsOfClass` recurrente.
4. `TargetingStrategy` ordena candidatos: cercano, mayor vida, cono, aleatorio determinista, árbol/enemigo prioritario.
5. `AttackStrategy` produce uno o más `AttackEmissionSpec`: arco melee, burst de proyectiles, órbita o daño sostenido.
6. Proyectiles reciben un spec resuelto con velocidad, pierce, rebotes, ricochet, retorno, órbita, radio y payload. No releen Data Assets por frame.
7. `ImpactStrategy` filtra impactos repetidos por target, genera `DamageSpec`, estado y cues.
8. Crítico se resuelve una vez con stream aleatorio de run/arma y queda registrado en el spec.

Composición inicial:

- Hacha: target cercano + arco melee + daño instantáneo.
- Sierra circular: emisión de proyectil + movimiento lineal/orbital + pierce/rebote.
- Búmeran: emisión + outbound/return + registro de impactos por tramo.
- Motosierra: target de cono + volumen sostenido con intervalos de daño, no hit por frame.
- Hacha maldita: ataque normal + modifier/status de contrapartida definido en datos.

La matriz de capacidades del editor rechaza combinaciones imposibles. Una nueva arma normalmente agrega un Data Asset y Blueprint visual; solo un comportamiento realmente nuevo requiere una estrategia C++ pequeña.

## 15. Arquitectura de tala, árboles, troncos y recolección

- `Tree` implementa Damageable/Targetable, no Tick. Se activa por daño y pasa `Standing -> Falling -> Spent`.
- La caída usa Chaos con colisión visual/ambiental controlada; al terminar, se deshabilita simulación. La recompensa se emite exactamente una vez, incluso con daño simultáneo.
- Troncos usan perfil `Pickup`: overlap con recolección/entrega, ignore Pawn movement y NavMesh; no bloquean rutas.
- Carga es un número de unidades, no un inventario de referencias a Actors. Un pickup puede representar varias unidades.
- Magnetismo se activa solo cerca del jugador y se actualiza por lote; el radio proviene de stats.
- Árboles normales otorgan XP al ser talados; XP es independiente de madera.
- MVP: Actors event-driven para cientos de árboles, medidos en mapa de estrés.
- Escalado previsto: `ForestRegistrySubsystem` puede representar árboles lejanos con HISM y “materializar” un Actor al entrar en rango/recibir interés. Se adopta solo si el coste de Actors excede el presupuesto.
- Respawn/regeneración se define por día o celdas, nunca desde cada árbol con timer propio.

## 16. Arquitectura de cuota, máquina, camioneta y economía

- `WoodCargoComponent`: cantidad/capacidad y transferencias atómicas.
- `QuotaComponent`: objetivo/progreso del día; acepta `min(carga, restante)` y devuelve remanente.
- `DeliveryZone`: bombea unidades a ritmo configurable para comunicar visualmente progreso sin perder atomicidad.
- `SellZone`: consulta `IChopItRunContext::IsQuotaComplete`; si es falso, no consume. Si es verdadero, convierte y acredita.
- `EconomyComponent`: saldo `int64`, `TransactionId`, razón/tag y delta; evita dobles pagos y facilita pruebas.
- `ShopComponent`: valida fase, dinero, slots, restricciones del personaje y ownership antes de una transacción compra+entrega atómica.
- `QuotaMachine`: palanca y presentación macabra; el devorado es resultado `DeathReason.QuotaFailed`, no lógica escondida en animación.
- `CabinHub`: expone referencia/ubicación para guía; no actúa como manager.

La cuota siempre tiene prioridad. No existe una ruta válida que venda madera antes de completarla. Si ambas zonas se solapan por error, el destino de cuota tiene prioridad explícita y el validador marca el layout.

## 17. Director de dificultad

### Dificultad prevista

`DifficultyDirectorComponent` calcula por intervalos (por ejemplo 0,5–1 s), no por frame:

```text
Budget = DayCurve × NightTimeCurve × InfiniteCurve
       × PactDirectorModifiers × CurseCurve
```

Luego selecciona entradas habilitadas por tags/día/ventana respetando coste, caps por familia, densidad total, distancia, línea de visión y cooldown de spawn. El scheduler reparte el resultado en frames y anchors válidos.

Al entrar en Night, el director puede reservar árboles registrados y convertirlos en enemigos mediante una transacción `TreeId -> EnemyEncounterId`: el árbol deja de ser cosechable antes del spawn, no entrega recompensa doble y su posición alimenta un anchor validado. La proporción, familias elegibles y ritmo de transformación son datos del día; druidas, enredaderas y guardianes adicionales aparecen por el scheduler normal o por transformaciones compatibles.

### Poder aproximado y ajuste dinámico

El `PlayerPowerScore` resume DPS teórico, supervivencia, movilidad y control. Se registra y se usa para:

- detectar perfiles imposibles o contenido fuera de rango;
- evitar una composición accidentalmente infranqueable;
- telemetría y balance offline;
- opcionalmente aliviar un pico, nunca elevar presupuesto porque el jugador construyó bien.

El baseline de día/tiempo/pactos siempre manda. No hay rubber-banding oculto. Una opción de dificultad asistida futura puede reducir presupuesto con consentimiento, separada de la curva prevista.

### Rendimiento y seguridad

- `MaxAliveCost`, cap duro de Actors, cap por arquetipo y spawn escalonado.
- El gobernador de rendimiento retrasa spawns si se supera presupuesto de frame y lo registra; no compensa en secreto con daño injusto.
- Enemigos simples usan steering directo y consultas Nav acotadas; druidas/guardianes pueden usar Behavior Tree o StateTree solo si su comportamiento lo justifica.
- Pooling por clase después de medir; reset contract obligatorio y prueba de estado residual.

## 18. Meta-progresión y SaveGame

### Esquema

`UChopItProfileSaveGame` contiene:

- `SchemaVersion`, `BuildVersion`, `ProfileId` y timestamps.
- Moneda meta entera.
- sets de `FPrimaryAssetId` desbloqueados para personajes/armas/contenido.
- upgrades meta por ID + nivel, incluyendo slots desbloqueados.
- estadísticas acumuladas y tutorial flags por IDs/tags.
- checksum básico/backup; no referencias a Actors, mapas ni punteros de assets.

### Flujo seguro

1. Cargar slot principal; validar cabecera y versión.
2. Si falla, cargar backup; si también falla, defaults seguros sin sobrescribir automáticamente el archivo roto.
3. Aplicar migraciones secuenciales `Vn -> Vn+1`, idempotentes y probadas con fixtures.
4. Resolver IDs mediante Asset Manager. IDs retirados quedan como desconocidos ignorables y se registran; no rompen carga.
5. Escribir a slot temporal, verificar, rotar backup y reemplazar principal.
6. Guardar en hitos: compra meta, fin de run y settings; no en cada pickup.

Retención: `RunSummary` calcula dinero bruto y porcentaje según derrota/victoria; el meta subsystem aplica una sola transacción con ID de run para evitar duplicación. Estado temporal de run nunca se copia entero al perfil.

## 19. UI, audio, VFX, juicy feedback y accesibilidad

### UI

- HUD: vida, XP/nivel, madera/capacidad, cuota, dinero, slots, fase/tiempo y Maldición.
- Indicador diegético + HUD hacia cabaña durante Dusk/Night, con distancia opcional.
- Level-up y pactos: pausa real en single-player, navegación teclado/gamepad, descripciones generadas desde los mismos modifier specs.
- Tienda: catálogo, precio, compatibilidad, slots y comparación; nunca recalcula reglas localmente.
- Presenters UObjects se suscriben a delegates y alimentan UserWidgets pasivos.

### Audio

- MetaSounds para variación de impactos, capas y parámetros.
- Quartz para transiciones musicales cuantizadas; Audio Modulation para buses `Music`, `SFX`, `UI`, `Ambience` y mezcla día/metal.
- `AudioDirectorSubsystem` observa fase e intensidad y cruza capas; no decide cuándo cambia la fase.
- Límites de concurrencia y prioridades evitan cientos de sonidos de impacto.

### VFX y feedback

- Cues de datos: hit flash, partículas de madera, trails, arcos, crítico, XP, level-up, cuota y élite.
- `FeedbackSubsystem` aplica presupuesto por categoría/distancia: combina impactos cercanos, limita números y prioriza crítico/élite/jugador.
- Hit stop global solo para eventos excepcionales; ataques comunes usan pausa/local slowdown visual para no romper hordas/timers.
- Camera shake con amplitud limitada, falloff y cooldown.
- Árboles caen con exageración controlada; pickups y XP muestran magnetismo legible.

### Accesibilidad

- Intensidad 0–100% de shake; flashes reducidos/desactivados; contraste de pickups/amenazas; densidad de partículas/números; tamaño de UI; modo daltónico basado en forma+color; volumen por bus.
- El modo “claridad” reduce trails, decals y partículas decorativas antes que telegraphs.
- Ninguna señal crítica depende solo de texto, color, audio o vibración.

## 20. Estrategia de rendimiento

### Objetivo inicial y referencia asumida

- **60 FPS / 1080p**, frame de 16,67 ms, configuración media/alta.
- Referencia provisional: CPU de 6 núcleos equivalente a Ryzen 5 3600/i5-10400, GTX 1660 Super/RX 5600 XT, 16 GB RAM y SSD.
- Presupuesto inicial orientativo, sujeto a captura: Game Thread ≤ 6 ms, Render Thread ≤ 6 ms, GPU ≤ 15 ms; margen para picos de 1% low.

### Presupuestos de contenido provisionales

- 150 enemigos simultáneos en vertical slice, stress a 300.
- 500 proyectiles lógicos/visibles, stress a 1.000.
- 250 pickups; agregación automática al superar cap.
- Máximo 10 spawns de Actor por frame y límite de Niagara/concurrencia por distancia.

No son promesas de diseño: cada cifra se ajusta a hardware y legibilidad.

### Decisiones

- Tick desactivado por defecto. Timers para armas, delegates para estado, scheduler por lotes para targeting/magnetismo/spawn.
- Registro espacial por celdas y consultas por tags; intervalos escalonados para IA, nunca scans globales frecuentes.
- Colisiones simples, canales dedicados (`Player`, `Enemy`, `Harvestable`, `Projectile`, `Pickup`, `Delivery`), overlaps mínimos y filtrados.
- Proyectiles rápidos con sweep; no física rígida completa salvo donde aporte game feel.
- NavMesh acotado; enemigos masivos con steering ligero y avoidance simplificado. Obstáculos dinámicos no regeneran nav cada caída.
- Niagara con fixed bounds, scalability tiers, culling, LOD y preferencia GPU solo cuando convenga; no collision events costosos para decoración.
- Árboles, logs y XP sin Tick individual; considerar HISM/materialización y pickups agregados tras medir.
- Spawning escalonado y precarga de Primary Assets en transición.
- Pooling: medir `SpawnActor`, destrucción, GC y memoria en Insights. Si se adopta, interfaz de reset y tests de reutilización.
- Lumen/ray tracing/Substrate se auditan en Fase 0. Para low-poly y hardware medio se prioriza una ruta sin hardware ray tracing salvo prueba visual justificada.

### Profiling

- Unreal Insights: CPU, timers, asset loading, memory y custom trace events por director/targeting.
- `stat unit`, `stat game`, `stat gpu`, `stat niagara`, Collision Analyzer y ProfileGPU durante desarrollo.
- Capturas repetibles en `L_Perf_Horde`, mismo seed y rutas de cámara.
- Gates de performance en MVP, vertical slice y antes de build; no esperar a Fase 14 para medir.

## 21. Estrategia de testing

### Automatización/unidad

- Stats: orden, caps, stacking, handles, remoción, precisión y NaN/inf.
- Damage: crítico determinista, resistencias, invulnerabilidad y muerte única.
- Cuota/venta: transferencias parciales, prioridad, enteros y transacciones idempotentes.
- Ofertas: seed, rareza, incompatibilidad, máximos y fallback.
- Pactos: aparición, stacking ilimitado técnico, Maldición y director modifiers.
- FSM: tabla completa de transiciones válidas/inválidas y timers tardíos.
- Save: round-trip, defaults, corrupción, backup, cada migración y IDs retirados.

### Functional Tests en mapas

- `L_Test_Combat`: dummy, armas y efectos.
- `L_Test_Harvest`: caída, spawn único, pickup no bloqueante.
- `L_Test_Economy`: cuota, zonas solapadas, venta y palanca.
- `L_Test_Cycle`: día 1–7 acelerado, muerte, victoria e infinito.
- `L_Test_AI`: spawns, caps, anchors y élites.
- `L_Perf_Horde`: seeds y cargas escalables.

### Integración

- Árbol -> XP/madera -> cuota -> venta -> compra -> arma.
- Noche -> prolongación -> élite -> resolución -> Parca -> siguiente día.
- Muerte/victoria -> resumen -> retención -> save -> recarga.

### Calidad de datos

- Validadores de Editor y commandlet en CI.
- Asset Registry: soft refs resolubles, Primary Asset IDs únicos, redirects pendientes y mapas cocinables.

### Manual

- Checklist de game feel: legibilidad a saturación, respuesta de impacto, navegación de UI, mezcla, guía a cabaña y accesibilidad.
- Sesiones de 20–30 minutos con telemetría local; registrar causa de daño/muerte y curva de recursos.

## 22. Riesgos técnicos y mitigaciones

| Riesgo | Mitigación / señal de activación |
|---|---|
| UE 5.8 reciente | Fijar hotfix, evitar Experimental, rama de upgrade y smoke/cook. |
| Hordas saturan Game Thread | Actors sin Tick, scheduler, spatial registry, caps; pooling/Mass solo tras captura. |
| Árboles físicos causan caos/coste | Ventana corta de simulación, collision profile limitado, sleep/settle y cap de caídas simultáneas. |
| Proyectiles combinables se vuelven sistema genérico inmanejable | Cuatro puntos de estrategia, matriz de compatibilidad y specs resueltos; no DSL universal. |
| Modificadores producen drift | Bases inmutables, handles, orden fijo y recálculo; tests exhaustivos. |
| Dependencias entre módulos | Interfaces en Core, presentation hoja, revisión automática de Build.cs. |
| Data Assets rotos al renombrar/quitar contenido | Primary IDs estables, soft refs, redirects, validadores y tolerancia en saves. |
| Director castiga builds buenas | Curva prevista independiente; poder nunca aumenta presupuesto; telemetría auditable. |
| Noche prolongable rompe duración | Crecimiento exponencial/caps y recompensa por ventana; pruebas de economía/run time. |
| Hub explotable | Reglas explícitas de spawn/daño y layout; pruebas con élites. |
| Ruido visual/audio | Feedback budget, prioridades, scalability, concurrencia y presets accesibles. |
| Save corrupto o duplicación de recompensa | Escritura temporal+backup, `RunId` idempotente, migraciones con fixtures. |
| Git con binarios | LFS, locks, assets pequeños, ownership y Perforce si el equipo crece. |
| Scope de contenido | Gates MVP/slice/full y métricas por fase; no crear variantes antes del bucle validado. |

## 23. Roadmap por fases

### Fase 0: definición técnica y configuración del proyecto

- **Objetivo:** convertir el template vacío en una base reproducible y fijada.
- **Sistemas:** módulos, configuración, Asset Manager, tags, colisiones, logs, tests, source control/CI.
- **Clases/assets:** módulos de §5; `UChopItAssetManager`, `UChopItDeveloperSettings`, tags nativos, mapas `L_Startup`/`L_Dev_Sandbox`, ADRs, `.gitignore`/LFS.
- **Dependencias:** ninguna.
- **Pasos:** fijar UE 5.8 hotfix; auditar ray tracing/Substrate/World Partition/Android config; crear grafo de módulos; configurar Primary Assets, canales, logging y builds; documentar convenciones.
- **Pruebas:** compile Development Editor y Development Win64; abrir/cerrar mapa; cook vacío; prueba de asset discovery y validator; clon limpio/LFS.
- **Aceptación:** proyecto compila/cocina sin errores, abre mapa propio, motor y plugins están fijados, módulos no tienen ciclos y CI smoke es verde.
- **Riesgos:** exceso de módulos o settings heredados; mitigar manteniendo APIs mínimas y ADR por decisión.
- **Resultado jugable:** sandbox vacío ejecutable y reproducible.

### Fase 1: movimiento, cámara y mapa de prueba

- **Objetivo:** locomoción cenital legible con retorno espacial al hub.
- **Sistemas:** Gameplay Framework, Enhanced Input, cámara, límites y blockout.
- **Clases/assets:** GameMode/State/Controller/State/Character, `InteractionComponent`; `IA_Move`, `IA_Interact`, `IMC_Gameplay`, `BP_ChopItCharacter`, `L_Dev_Sandbox`.
- **Dependencias:** F0.
- **Pasos:** composición del Pawn; movimiento desacoplado de cámara; spike perspectiva vs ortográfica; cámara sin rotación accidental; hub/forest blockout y nav.
- **Pruebas:** teclado+gamepad, diagonales normalizadas, bordes, distintas tasas de frame, obstrucciones y guía placeholder.
- **Aceptación:** control consistente, cámara muestra área objetivo sin clipping, 60 FPS en blockout y no hay lógica en Level Blueprint.
- **Riesgos:** ortográfica limita VFX/sombras; decidir mediante captura comparativa.
- **Resultado jugable:** recorrer un claro alrededor de la cabaña.

### Fase 2: ataque automático y daño

- **Objetivo:** primera hacha automática contra dummies con daño verificable.
- **Sistemas:** stats, salud, targeting, weapon instance, scheduler, cues básicos.
- **Clases/assets:** componentes de stats/health/inventory/autocombat, estrategias básicas, TargetingSubsystem, `DA_Weapon_BasicAxe`, dummy y test map.
- **Dependencias:** F1.
- **Pasos:** specs y fórmula; registro de targets; timer de ataque; arco melee; damage/death delegates; debug overlay.
- **Pruebas:** cadencia, rango, crítico off/on, varios blancos, baja/alta FPS, alta/baja de targets y 1.000 aplicaciones de modifiers.
- **Aceptación:** hacha selecciona y daña sin input, no usa scan global/Tick por arma, daño coincide con fórmula y muerte ocurre una vez.
- **Riesgos:** abstracción prematura; limitar estrategias a necesidades demostradas.
- **Resultado jugable:** moverse mientras el hacha destruye dummies.

### Fase 3: árboles, caída, troncos y recolección

- **Objetivo:** convertir combate en tala y madera transportable.
- **Sistemas:** harvest, física, pickups, carga, XP placeholder y registro forestal.
- **Clases/assets:** Tree, LogPickup, WoodCargo, ForestRegistry; `BP_Tree_Basic`, `BP_LogPickup`, collision profiles y `L_Test_Harvest`.
- **Dependencias:** F2.
- **Pasos:** tree state; caída y settle; recompensa única; pickup agregado; magnetismo/capacidad; respawn por lote.
- **Pruebas:** daño simultáneo, caída contra Pawn/hub, tronco no bloquea, carga llena/remanente, 500 árboles y 250 pickups.
- **Aceptación:** cada árbol produce XP/madera una sola vez, logs nunca bloquean movimiento/nav y el stress target respeta presupuesto provisional.
- **Riesgos:** Chaos y cantidad de Actors; limitar simulaciones y medir HISM/materialización.
- **Resultado jugable:** talar, ver caer y recoger madera.

### Fase 4: cabaña, máquina, cuota y camioneta

- **Objetivo:** cerrar el bucle económico de madera con prioridad obligatoria.
- **Sistemas:** hub, quota, delivery zones, economía y feedback provisional.
- **Clases/assets:** CabinHub, QuotaMachine/Component, DeliveryZone, SellZone, EconomyComponent; Blueprints de máquina/camioneta y `DA_Day_01`.
- **Dependencias:** F3.
- **Pasos:** transferencias atómicas; gating; cuota; venta excedente; ledger; HUD debug; palanca aún sin ciclo completo.
- **Pruebas:** cuota exacta, exceso en la misma transferencia, zonas solapadas, reentrada, carga cero, doble evento y valores extremos.
- **Aceptación:** nada se vende antes de cuota; no se pierde excedente; saldos y progreso son enteros/idempotentes; feedback refleja fuente de verdad.
- **Riesgos:** condiciones de carrera por overlaps; serializar transferencias en autoridad y usar transaction IDs.
- **Resultado jugable:** talar, pagar cuota y vender excedente.

### Fase 5: ciclo día-noche y regreso a la cabaña

- **Objetivo:** un ciclo temporal completo con presión, transición y cierre explícito.
- **Sistemas:** FSM, RunState, timers, guía, iluminación/audio placeholder, palanca.
- **Clases/assets:** CycleStateMachine y fases Day/Dusk/Night/Elite-placeholder/Resolution/Death; WorldPresentationComponent, curvas de tiempo.
- **Dependencias:** F4.
- **Pasos:** tabla de transiciones; timers cancelables; gracia de cuota; guía; noche mínima/prolongación; palanca; resolución provisional.
- **Pruebas:** cada transición válida/inválida, pausa, callback tardío, cuota impaga, palanca prematura, 20 ciclos acelerados.
- **Aceptación:** solo FSM cambia fase; cuota impaga mata al hard deadline; palanca solo cierra noche válida; ningún timer de fase anterior actúa después.
- **Riesgos:** estados audiovisuales desincronizados; presentation consume snapshot y eventos con generación.
- **Resultado jugable:** día, dusk, noche prolongable y retorno para cerrar ciclo.

### Fase 6: experiencia, niveles y mejoras

- **Objetivo:** progresión data-driven exclusiva de level-up.
- **Sistemas:** XP, nivel, ofertas, rareza, modificadores y UI de elección.
- **Clases/assets:** ExperienceComponent, OfferComponent, UpgradeDefinition, widgets; curva XP y 8–12 mejoras de prueba.
- **Dependencias:** F2–5.
- **Pasos:** fuentes de XP; cola de niveles; seed; filtros/rareza; pausa/elección; aplicar handles; descripción derivada de datos.
- **Pruebas:** múltiples levels simultáneos, pool agotado, stacks/caps, remoción temporal, determinismo y ninguna compra en tienda.
- **Aceptación:** XP de árboles/enemigos dispara opciones válidas; una selección aplica exactamente una vez; stats vuelven al valor correcto al remover fuentes.
- **Riesgos:** combinaciones inválidas; validadores, tag queries y fallback explícito.
- **Resultado jugable:** subir niveles y transformar el hacha/personaje.

### Fase 7: tienda, armas adicionales y ranuras

- **Objetivo:** gastar excedente y construir loadout respetando exclusividad.
- **Sistemas:** shop, inventario de armas, slots meta/runtime y precios.
- **Clases/assets:** ShopComponent, UI, 2–3 armas compartidas, estrategias de projectile/return/orbit/sustain necesarias.
- **Dependencias:** F4 y F6.
- **Pasos:** catálogo por seed/día; transacción atómica; slot rules; arma inicial exclusiva fuera/de dentro del conteo según ADR; upgrades por selector.
- **Pruebas:** saldo insuficiente, slots llenos, arma exclusiva ajena, compra doble, rollback si falla entrega, arma adicional ataca.
- **Aceptación:** no hay saldo negativo ni arma incompatible; slots inicial/máximo son datos; nueva arma compuesta no modifica clases centrales.
- **Riesgos:** explosión de comportamientos; mantener cuatro ejes de estrategia y matriz de capacidades.
- **Resultado jugable:** vender, comprar y usar varias armas automáticas.

### Fase 8: enemigos, spawns y director de dificultad

- **Objetivo:** noche bullet-heaven escalable y predecible.
- **Sistemas:** enemigos, brain, anchors, budget director, spawn scheduler, XP nocturna.
- **Clases/assets:** EnemyCharacter/Brain, DifficultyDirector, SpawnScheduler, SpawnAnchor; 3 familias iniciales y SpawnProfile.
- **Dependencias:** F2, F5–7.
- **Pasos:** arquetipos básico/rápido/enredadera; presupuesto previsto; ventanas; caps; spawn fuera de vista; XP ~2× configurado; power telemetry no punitiva.
- **Pruebas:** mismo seed, caps, anchors inválidos, prolongación, pact modifiers fake, 150/300 enemigos y ausencia de target.
- **Aceptación:** dificultad sigue curvas/datos, no fuerza escala por build fuerte, caps nunca se violan y noche permanece dentro del budget en hardware objetivo.
- **Riesgos:** navegación/colisión de hordas; steering ligero, intervalos escalonados y perfiles simples.
- **Resultado jugable:** sobrevivir una noche creciente con tres amenazas.

### Fase 9: élites y séptimo día

- **Objetivo:** cierre obligatorio de cada ciclo y final de run normal.
- **Sistemas:** élites, encounter tracking, días 1–7, entidad final y VictoryChoice.
- **Clases/assets:** definiciones/Blueprints de élite y jefe final; Elite phase real; DayDefinitions 1–7; arena/telegraphs.
- **Dependencias:** F5 y F8.
- **Pasos:** palanca bloquea spawns normales según diseño; spawn elite; tracking robusto; recompensa; escalado diario; jefe día 7; elección post-victoria placeholder.
- **Pruebas:** élite muere/desaparece, player muere simultáneo, doble delegate, día 7, reinicio y enemigo fuera de bounds.
- **Aceptación:** ningún ciclo avanza sin élite derrotado; día 7 llega a VictoryChoice; daño peligroso siempre tiene telegraph legible.
- **Riesgos:** élite perdido/soft lock; watchdog recuperable y tracking por encounter ID, no búsqueda global.
- **Resultado jugable:** run estructural de siete días con final.

### Fase 10: Parca, pactos y Maldición del Bosque

- **Objetivo:** riesgo/recompensa acumulable sin tocar sistemas centrales por pacto.
- **Sistemas:** condiciones, ofertas, pact component, tags, director modifiers y presentación.
- **Clases/assets:** PactComponent/OfferComponent, PactDefinition, 6–10 pactos iniciales, BP Parca y UI.
- **Dependencias:** F6, F8–9.
- **Pasos:** registrar métricas diarias; aparición aleatoria/forzada; seed/pesos; incompatibilidad; aceptar/rechazar; aplicar beneficio/coste/curse; hooks por specs.
- **Pruebas:** no-hit, madera vendida, élite rápida, noche larga, rechazo sin penalidad, stacks altos, incompatibles y recarga de fase.
- **Aceptación:** agregar pacto basado en modificadores existentes requiere solo datos; rechazo es neutro; Maldición y consecuencias del director son auditables.
- **Riesgos:** un pacto necesita lógica única; encapsular consecuencia nueva como estrategia/tagged effect pequeño, no `switch` central.
- **Resultado jugable:** la Parca ofrece decisiones que transforman la run.

### Fase 11: muerte, victoria, meta-progresión y guardado

- **Objetivo:** cerrar runs de forma segura y convertir resultados en progreso persistente.
- **Sistemas:** resultados, retención, perfil, unlocks, save/migrations y frontend mínimo.
- **Clases/assets:** SaveSubsystem, MetaProgressionSubsystem, ProfileSaveGame, MetaUnlockDefinition, pantallas de resumen/selección.
- **Dependencias:** F7, F9–10.
- **Pasos:** RunSummary; transaction ID; porcentajes; schema V1; backup/atomic write; defaults; unlocks de personaje/arma/slot; fixtures migratorios.
- **Pruebas:** derrota antes de día 7, victoria, doble final, save corrupto, ID eliminado, V0->V1, sin permisos y perfil vacío.
- **Aceptación:** retención correcta una vez; reiniciar conserva meta y no run; corrupción recupera backup/default sin borrar evidencia; IDs rotos no impiden carga.
- **Riesgos:** pérdida/duplicación de progreso; escritura atómica e idempotencia por RunId.
- **Resultado jugable:** runs con consecuencias y desbloqueos persistentes.

### Fase 12: modo infinito

- **Objetivo:** continuar voluntariamente tras victoria reutilizando el mismo ciclo.
- **Sistemas:** InfiniteEntry, curvas extendidas, spawn/quotas/rewards, récords y salida.
- **Clases/assets:** curvas infinito, datos de milestones y UI de elección/récord.
- **Dependencias:** F9–11.
- **Pasos:** flag de modo; día 8+; cuotas/presupuesto/caps; recompensas decrecientes o hitos; salida voluntaria; resumen.
- **Pruebas:** elección retirar/continuar, días grandes, overflow, seed, muerte en infinito y 100 ciclos acelerados.
- **Aceptación:** no duplica FSM; valores no desbordan; muerte/retiro liquidan según datos; modo normal no cambia.
- **Riesgos:** crecimiento numérico y densidad; curvas con clamps/caps y tipos enteros amplios.
- **Resultado jugable:** supervivencia endless estable tras día 7.

### Fase 13: UI, audio, VFX, game feel y accesibilidad

- **Objetivo:** convertir la arquitectura funcional en experiencia comercial legible y satisfactoria.
- **Sistemas:** HUD/presenters, CommonUI si procede, MetaSound/Quartz/Modulation, Niagara, feedback budget y settings.
- **Clases/assets:** HUD/Presenter, AudioDirector, FeedbackSubsystem, GameUserSettings; widgets finales, mixes, MetaSounds, Niagara y presets.
- **Dependencias:** F1–12.
- **Pasos:** skin UI; navegación; capas día/metal; cues; hit feedback; transición; saturación; opciones de flash/shake/contraste/densidad; localización-ready.
- **Pruebas:** cada preset, teclado/gamepad, pausa, 150 enemigos, mezcla de voces, señal sin audio/color/texto, fotosensibilidad checklist.
- **Aceptación:** señales críticas multimodales; opciones se guardan/aplican; feedback respeta caps; UI no contiene reglas duplicadas.
- **Riesgos:** polish degrada FPS/legibilidad; budgets y revisión en escenas de peor caso.
- **Resultado jugable:** vertical slice con identidad acogedora/metal absurda y accesible.

### Fase 14: optimización, testing y preparación de build

- **Objetivo:** alcanzar estabilidad, rendimiento y reproducibilidad de release.
- **Sistemas:** profiling, pooling medido, LOD/culling, CI, cook/package, crash/logging y QA.
- **Clases/assets:** `L_Perf_Horde`, suites finales, scalability configs, Device Profiles, build scripts y checklist release.
- **Dependencias:** todas.
- **Pasos:** capturas baseline; atacar top bottlenecks; introducir pooling/HISM solo con evidencia; memory/leak soak; automatización; Shipping package; validación de licencias/assets.
- **Pruebas:** 30/60 min soak, 1% lows, carga/guardado repetido, clean machine install, alt-tab/resoluciones/input, 100 runs aceleradas y matriz gráfica.
- **Aceptación:** 60 FPS objetivo en escenario acordado, sin crash/soft lock conocido crítico, cook Shipping limpio, saves migran y todas las suites/gates pasan.
- **Riesgos:** optimización tardía cambia feel; profiling intermedio previo y golden captures.
- **Resultado jugable:** candidato de release Win64 mantenible.

## 24. Criterios de aceptación de cada fase

Una fase solo cierra con cuatro evidencias: build/cook pertinente, tests automatizados verdes, prueba funcional grabable/repetible y checklist manual. Gate resumido:

| Fase | Evidencia mínima de salida |
|---:|---|
| 0 | Clean build/cook + motor/plugins/repo fijados |
| 1 | Movimiento y cámara válidos en teclado/gamepad |
| 2 | Autoataque y fórmula cubiertos por tests |
| 3 | Árbol->tronco->carga sin bloqueo ni duplicación |
| 4 | Cuota prioritaria y ledger exacto |
| 5 | FSM completa de un ciclo sin timers huérfanos |
| 6 | Level-up determinista y modifiers reversibles |
| 7 | Compra atómica, slots y exclusividad configurables |
| 8 | Noche con caps y presupuesto medido |
| 9 | Siete días, élites y victoria sin soft lock |
| 10 | Pactos data-only para comportamientos existentes |
| 11 | Retención/save/migración idempotentes |
| 12 | Infinito reutiliza ciclo y soporta stress numérico |
| 13 | Presentación accesible dentro de budgets |
| 14 | Shipping reproducible, performance y QA aprobados |

Si falla un criterio, se documenta el riesgo y la fase continúa abierta; no se avanza por calendario.

## 25. Orden recomendado de implementación

El orden es exactamente F0 → F14 porque cada fase produce un slice verificable y reduce el mayor riesgo siguiente. Dentro de una fase:

1. Contrato y datos mínimos.
2. Implementación C++ del dominio.
3. Composición Blueprint mínima.
4. Tests automatizados.
5. Prueba funcional en mapa.
6. Profiling proporcional.
7. Ajuste de presentación.
8. Validación de criterios y ADR/resumen.

Contenido artístico puede avanzar en paralelo con proxies y contratos fijados, pero no habilita cerrar una fase si falla su gameplay. Balance fino comienza cuando el bucle F8 existe; antes solo se usan valores instrumentales.

## 26. Alcance del MVP, vertical slice y versión completa

### MVP de prototipo — F0 a F8

- Un día representativo repetible, un personaje, hacha + dos armas compartidas.
- Árbol básico, cuota, camioneta, tienda, 8 mejoras, tres enemigos y élite placeholder/real simple.
- Ciclo día/noche completo, sin meta final ni polish comercial.
- Objetivo: validar que tala + riesgo nocturno + economía + build crafting es divertido.

### Vertical slice — F0 a F11 + pase dirigido de F13

- Run estructural de siete días con contenido reducido pero calidad objetivo.
- Un personaje, 4–5 armas, 12–18 mejoras, 4–5 familias, 2 élites rotables, jefe final, 6 pactos.
- Save/meta mínimo, música día/noche, UI y accesibilidad esenciales.
- Objetivo: demostrar experiencia comercial y pipeline de contenido, no volumen final.

### Versión completa inicial — F0 a F14

- Siete días balanceados, infinito, 3+ personajes, 10+ armas, 30+ mejoras, 12+ pactos, 6+ enemigos, pool de élites y jefe final.
- Meta-progresión, opciones, polish, stress/soak, build Win64 y contenido original completo.
- Las cantidades son baseline de planificación; se recortan antes que degradar calidad, legibilidad o mantenibilidad.

## 27. Decisiones que pueden posponerse

- Ortográfica real vs perspectiva casi ortográfica: decidir al final de F1.
- HISM/materialización de árboles: tras stress de F3.
- Pooling por tipo: tras Insights de F3/F8/F13.
- Common UI frente a UMG puro: antes de cerrar F7.
- Behavior Tree/StateTree para druidas/guardianes: cuando exista su diseño concreto.
- DDA asistida opcional: después de tener telemetría; baseline sin rubber-banding.
- World Partition/PCG: solo si el mapa deja de ser acotado y diseñado a mano.
- Reanudación de una run interrumpida: después del Save de perfil V1.
- Cloud saves, achievements, Steam/EOS y consola: fuera del alcance PC inicial.
- Localización concreta, número final de personajes/armas/pactos y monetización: producción de contenido.
- Migración a UE6: proyecto separado de evaluación, nunca automática.
- Cooperativo: cambio material; requeriría revisar autoridad, GAS/red y no se reserva complejidad ahora.

## 28. Primera tarea concreta para comenzar el proyecto

**Tarea F0.1 — Baseline reproducible y ADR-0001**

Sin gameplay todavía:

1. Registrar versión exacta/hotfix de UE 5.8 y toolchain Win64.
2. Crear `ADR-0001-Engine-And-Architecture.md` con decisiones: motor, sistema propio frente a GAS, módulos, cámara a validar, mapa acotado y source control.
3. Auditar `ChopIt.uproject`, `DefaultEngine.ini`, `DefaultGame.ini` y Build.cs heredados.
4. Decidir y documentar ray tracing, Substrate, Lumen, World Partition y plugins; deshabilitar lo no justificado.
5. Configurar Git LFS/ignores/locks y una comprobación de clean checkout.
6. Crear los módulos vacíos mínimos, mapa propio de startup, logs/tags/settings y smoke automation.
7. Compilar Development Editor, ejecutar smoke, hacer cook Win64 vacío y adjuntar resultados al cierre de F0.1.

**Criterio de cierre:** otra máquina puede clonar, obtener binarios LFS, compilar, abrir `L_Startup` y cocinar Win64 con la misma versión fijada y sin depender del mapa/template de Engine.

---

## Aprobación necesaria antes de implementar

No hay preguntas técnicas bloqueantes. Para comenzar la Fase 0 solo necesito tu aprobación explícita de este plan. Si apruebas sin cambios, aplicaré las suposiciones de §2 y ejecutaré únicamente la Fase 0, empezando por F0.1, con explicación previa de archivos/assets, pruebas y criterios.
