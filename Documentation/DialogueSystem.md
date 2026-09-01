# Sistema de diálogos de ChopIt

## Crear una conversación

1. Crear un `ChopItDialogueSequence` dentro de `/Game/ChopIt/Dialogue`.
2. Asignar un `DialogueId`, `EntryLineId`, tema y líneas con IDs únicos.
3. Para cada línea, asignar hablante, expresión, texto, destino o elecciones.
4. Ejecutar **Validate Assets** antes de guardar. La validación detecta IDs repetidos,
   destinos inexistentes, nodos inalcanzables y markup roto.

El Asset Manager cocina automáticamente las secuencias ubicadas bajo esa carpeta.

## Markup expresivo

Los tags pareados se pueden anidar:

```text
No cortes <shake amp="2"><wave amp="5"><color value="#FF6A00">ese árbol</color></wave></shake>.
<cue id="Warning" event="Dialogue.Event.Warning" face="Angry" camera="Impact" target="Speaker"/>
```

Tags pareados: `shake`, `wave`, `color`, `pulse`, `size`, `speed`, `cue`.

Tags autocerrados: `pause`, `cue`, `face`, `event`, `camera`, `sfx`.

Ejemplos:

```text
<pause seconds="0.25"/>
<face id="Surprised"/>
<event tag="Dialogue.Event.Warning" id="TreeWarning" target="Speaker"/>
<camera id="Impact"/>
<sfx id="WoodCrack"/>
```

Los offsets de `shake` y `wave` se suman; `size`, `speed` y `pulse` se multiplican;
el color anidado más interno prevalece. Un cue se ejecuta una sola vez al alcanzar
su posición. `fireOnFastForward="false"` permite omitir cues puramente decorativos
cuando el jugador completa instantáneamente la línea.

Los símbolos `<` y `>` literales deben escribirse como `&lt;` y `&gt;`.

## Inicio desde C++ o Blueprint

`UChopItDialogueSubsystem` vive en el jugador local. Su entrada principal es:

```cpp
FChopItDialogueHandle Handle = Dialogue->StartDialogue(Sequence, Context,
    EChopItDialogueStartPolicy::Queue);
```

El contexto acepta Gameplay Tags, argumentos de `FText::Format` y bindings de actor
con nombre. Las cámaras y sonidos solo se resuelven por IDs registrados en el tema;
el texto nunca carga rutas arbitrarias.

Políticas disponibles: `Queue`, `Replace`, `RejectIfBusy`. Para control se exponen
`AdvanceOrComplete`, `MoveChoice`, `SelectChoice`, `CancelDialogue` y `StopDialogue`.
Los delegates informan inicio/final, líneas, elecciones y eventos con payload.

## Demo y controles

Abrir `/Game/ChopIt/World/Maps/L_Test_Dialogue` e interactuar con el cilindro de demo.

- `E`, Enter, Espacio o gamepad A: completar/avanzar/confirmar.
- `W/S`, flechas o D-pad: navegar elecciones.
- Escape o gamepad B: cancelar si la secuencia lo permite.

Las opciones de usuario `DialogueTextSpeed`, `bInstantDialogueText`,
`DialogueUIScale` y `bReduceDialogueMotion` controlan accesibilidad sin alterar el HUD.

## Regenerar la demo

Con el Editor cerrado:

```powershell
D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe ChopIt.uproject `
  -run=ChopItBootstrap -Dialogue -unattended -nop4 -NullRHI
```

El commandlet solo vuelve a guardar los assets propios del diálogo y no modifica las
acciones de gameplay, los assets de cámara existentes ni `DA_Day_01`.
