# Teléfono misterioso — Etapa 2: llamadas

`UChopItPhoneCallComponent` programa cada llamada con timers. Usa dos `UAudioComponent` unidos al teléfono para que las colas de los dos golpes de cada par puedan superponerse sin cortarse. Ambos usan audio mono espacial 3D con atenuación lineal. Cuando termina la duración, el estado vuelve a `Available` y se programa otra llamada. Al atender, limpia los timers y detiene ambos canales. `OnRingStarted`, `OnRingBeat` y `OnRingStopped` sincronizan la viñeta y el sacudón del teléfono.

## Configuración

- `Content/ChopIt/Phone/DA_PhoneCall` contiene el sonido, intervalo mínimo y máximo, duración, volumen, radio a volumen completo y distancia audible. La referencia opcional `AttenuationAsset` permite usar un Sound Attenuation Asset con una curva personalizada. Se puede editar en Unreal Editor.
- El intervalo se mide entre los comienzos de dos timbres. En la configuración actual, el primer timbre empieza casi al iniciar Play; después se repite cada 10 segundos y dura 3 segundos. Dentro de cada timbre hay tres pares: empiezan en los segundos 0, 1 y 2; el segundo golpe de cada par ocurre 0,32 segundos después del primero. La viñeta salta en cada golpe. Al atender el teléfono, todo se detiene.
- `Content/ChopIt/Phone/S_PhoneRingStrike` es el golpe proporcionado para el teléfono. Su fuente importada es `SourceArt/Phone/RingStrike.wav`, convertida del MP3 en la misma carpeta. `Scripts/TuneMysteryPhoneBeatPresentation.py` lo asigna a `DA_PhoneCall`.
- `BP_MysteryPhone` hereda el nuevo componente. Su propiedad `CallConfig` permite usar un Data Asset distinto en otras variantes.
- El asset de llamada se crea con `Scripts/BuildMysteryPhoneStage2.py`; el script conserva cualquier configuración ya existente.

Sin asset de atenuación, la distancia audible y el radio definen la caída lineal. Al asignar un asset de atenuación, sus ajustes y curva reemplazan esa caída.

## Verificación

`ChopIt.Phone.Stage2.CallCycle` valida el Data Asset, ambos canales espaciales, los seis golpes, la exclusión de llamadas simultáneas, el final por duración y la cancelación al atender.
