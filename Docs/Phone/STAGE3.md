# Teléfono misterioso — Etapa 3: viñeta RING RING

El HUD crea una conexión por teléfono y escucha `OnRingStarted` y `OnRingStopped`. Cada conexión reutiliza un solo widget y un `UChopItPhoneIndicatorViewModel` del plugin ModelViewViewModel. El widget observa Field Notifications; no consulta al actor. Los cálculos de distancia, cámara y proyección viven en la conexión de presentación.

La viñeta se muestra durante el timbre si el jugador está dentro de la distancia configurada. Proyecta el teléfono sobre la pantalla, sigue el movimiento de la cámara y se coloca en el borde cuando queda fuera de vista. Para objetivos detrás de la cámara usa los ejes de la cámara. El texto aparece progresivamente, la escala hace un rebote al entrar y la salida se desvanece. El timer de actualización solo funciona mientras suena o se desvanece la viñeta; no hay Tick permanente.

## Assets editables

- `Content/ChopIt/Phone/DA_PhoneIndicator`: texto, widget, distancia máxima, margen, offset, escalas, curva opcional, intervalo de actualización y tiempos de animación.
- `Content/ChopIt/Phone/WBP_PhoneRingIndicator`: Blueprint derivado del widget UMG nativo. Se puede sustituir en el Data Asset por otro widget derivado.
- `Scripts/BuildMysteryPhoneStage3.py`: crea ambos assets sin sobrescribir la configuración existente.

La prueba `ChopIt.Phone.Stage3.ProjectionAndMVVM` cubre proyección dentro y fuera de pantalla, casos detrás de cámara y notificaciones del ViewModel.
