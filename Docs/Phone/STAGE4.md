# Teléfono misterioso — Etapa 4: indicación de interacción

El HUD instala una presentación local en cada teléfono. Un `UWidgetComponent` en espacio de pantalla mantiene la tecla encima del modelo. Un volumen de solapamiento detecta al jugador cercano; solo mientras está dentro se usa un timer breve para comprobar el rango exacto de `AChopItMysteryPhone::CanInteract`. Al salir se oculta la indicación y se detiene el timer. Al atender, el cambio de estado oculta la indicación inmediatamente. La interacción y la validación definitiva siguen en `UChopItInteractionComponent` y el actor del teléfono.

El widget observa `UChopItPhonePromptViewModel` mediante Field Notifications. Toma la tecla asignada a `IA_Interact` cuando el mapa de entrada está disponible, y usa el texto de respaldo del Data Asset en otro caso. No crea widgets por cada entrada al rango.

## Assets editables

- `Content/ChopIt/Phone/DA_PhonePrompt`: texto de acción, tecla de respaldo, widget, altura, tamaño, margen de detección y frecuencia de comprobación.
- `Content/ChopIt/Phone/WBP_PhoneInteractPrompt`: Blueprint del widget flotante.
- `Scripts/BuildMysteryPhoneStage4.py`: crea los assets si aún no existen y conserva ajustes existentes.

La prueba `ChopIt.Phone.Stage4.PromptEligibility` cubre rango, teléfono sonando, respuesta única y notificaciones del ViewModel.
