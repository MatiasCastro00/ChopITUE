# HUD PSX: edición y pruebas

## Assets

- `/Game/ChopIt/UI/WBP_PSX_HUD`: HUD existente, ahora basado en `ChopItJuiceWidget`.
- `/Game/ChopIt/UI/DA_UIJuice`: perfil de movimiento, interpolación, colores, límites y audio.
- `/Game/ChopIt/UI/Audio/UI_*`: doce sonidos sintéticos provisionales reemplazables.
- `Saved/WBP_PSX_HUD_before_juice.uasset`: copia anterior a esta implementación.
- `Saved/HUD_before_juice.t3d` y `Saved/HUD_inspect.t3d`: exportaciones para comparar el diseño.

El árbol original del WBP se conservó íntegro. Las nuevas animaciones son `JuicePickup` (CargoIcon), `JuiceMission` (MissionFrame) y `JuiceStamp` (QuotaTextContainer). Se editan desde Animations del diseñador. Sus curvas tienen precedencia sobre el rebote nativo de esos widgets; el perfil controla las demás reacciones. El evento Blueprint `OnJuiceEvent` permite extender la presentación.

La capa `JuiceEffects` se crea al ejecutar y no recibe entrada. Cantidades variables, destellos, astillas, humo, sello y partículas se construyen en ella. No se serializan como cambios de diseño. Las barras conservan el ancho completo y el UV que tenían en el WBP al entrar en Play, recortando ambos de forma proporcional. No se reconstruye el HUD ni se sustituyen los ajustes del usuario.

## Perfil

### Icono del ciclo

`SunIcon` usa el sol del WBP de día y las regiones de atardecer/luna del mismo atlas para Dusk/Night. Conserva posición, tamaño y transformación base. `ClockIconSwingDegrees` (20°), `ClockIconSwingPeriod` (2,4 s), `ClockIconFadeDuration` (0,45 s) y `ClockIconUrgentPulse` (20 %) son editables en la sección Clock del perfil. El cambio hace fade-out y fade-in; el sprite se sustituye cuando queda invisible. La primera lectura del estado no hace fundido. El pulso se activa bajo los umbrales de urgencia de día/atardecer, nunca por el temporizador mínimo de noche. Movimiento reducido desactiva balanceo y pulso, pero conserva el fundido. La demo incluye `Sunset` y `DayReturn`, además de `ResetPhase` para la luna.

Seleccionar `DA_UIJuice`. `bReducedMotion` elimina partículas viajeras y latidos, reduce los golpes y deja cantidades con desplazamiento mínimo. `Volume` regula el audio UI; la variación de tono está limitada al 5 %. `PickupSoundInterval` nunca baja de 0,08 s. Los límites efectivos son 48 partículas y ocho textos. Se agrupan madera, cuota, dinero ganado y XP durante `GainGroupWindow`.

`FillDuration`, `MoneyDuration`, `DamageTrailDelay`, `DamageTrailDuration`, `TravelDuration` y `FloatingTextDuration` controlan las interpolaciones. `Envelope` acepta una curva flotante normalizada de 0 a 1 para los golpes nativos. Los sonidos se reemplazan en el mapa `Sounds` sin tocar C++.

## Ejecución

El HUD se integra en `L_PSX_test` a través del HUD existente. Se suscribe a cargo, salud, cuota, economía, XP y ciclo. `RevealMissionTracker` revela la misión existente de cuota. No se añaden quests ni recompensas. La restauración de salud ahora notifica el cambio; los plazos de día/atardecer emiten el cero antes de cambiar de fase. La noche mínima no activa urgencia.

La animación de entrega se dispara desde `CompleteFlight` después de que la cuota acepte realmente la unidad. Cambiar el contador de cuota por sí solo no dispara troncos viajeros.

## Demo solo de desarrollo

Desde PowerShell, con el editor cerrado:

```powershell
& 'D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' 'C:\Users\Elmr\Documents\Unreal Projects\ChopIt\ChopIt.uproject' /Game/ChopIt/World/Maps/L_PSX_test -game -UIJuiceDemo -ResX=1280 -ResY=720 -ForceRes -windowed
```

La secuencia dura unos 48 segundos y cierra esa ejecución al terminar. Solo simula valores visuales: no modifica los valores reales de la partida ni guarda el mapa. Prueba recogida, rechazo, entrega, daño, curación, dinero, gasto, nivel, vida crítica, reloj, cuota completa, vencimiento, cambio de fase, barras a 0/25/50/100, tres niveles, ráfaga, atardecer y retorno al sol. Para movimiento reducido añadir `-UIJuiceReduced`; para 1080p cambiar ResX/ResY. Las capturas quedan en `Saved/UIJuice/<resolución>_<modo>/`.

También se puede llamar `PreviewJuiceEvent` desde Blueprint durante desarrollo. Los nombres de eventos están en `ChopItJuiceWidget.cpp`. La función no ejecuta efectos en Shipping.

## Verificación reproducible

En Session Frontend → Automation ejecutar `ChopIt.UI`. `Juice` comprueba UV, dimensiones, inicialización, notificaciones reales, reloj, XP y desconexión. `JuicePIE` abre `L_PSX_test`, prueba eventos reales y ráfagas, retorno de transformaciones, pausa/reconexión y entrega mediante interacción con la máquina. No guarda la partida ni el mapa de prueba.

Las capturas sintéticas sirven para inspeccionar los efectos aislados; `PIE_events.png` y `PIE_delivery.png` corresponden a Play con componentes reales. Los logs están en `Saved/UIJuicePIE.log` y `Saved/UIJuice*.log`. No se ha realizado una prueba de empaquetado Shipping ni una evaluación auditiva humana de los clips provisionales.

Validación realizada el 12/09/2026: compilación C++ correcta, compilación WBP correcta, `ChopIt.UI.Juice` y `ChopIt.UI.JuicePIE` aprobados. Comparación textual del árbol original: idéntico, 29.460 caracteres antes/después. Capturas inspeccionadas en 1280×720 y 1920×1080, más movimiento reducido. La entrega de prueba aceptó cinco unidades y produjo cinco confirmaciones; la prueba también verifica retorno de transformaciones, pausa y reconexión sin recogidas falsas.
