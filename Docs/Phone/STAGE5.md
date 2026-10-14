# Teléfono misterioso — Etapa 5: variantes y transformación

`DA_PhoneVariants` contiene una lista de variantes. Cada `UChopItPhoneVariantData` tiene ID único, peso, color y parámetros de revelación, además de referencias opcionales a modelos, materiales, Niagara y sonido. Los assets iniciales son `DA_PhoneDevil` (peso 80) y `DA_PhoneAngel` (peso 20). El selector usa pesos relativos: agregar una tercera variante solo requiere crearla y agregarla al pool.

Al atender, `UChopItPhoneVariantComponent` elige una variante una sola vez usando una seed independiente derivada de la partida y del índice del teléfono. Emite `OnVariantSelected` y cambia el estado a `Transforming`. `UChopItPhoneRevealComponent` aplica los recursos que tenga la variante, hace un pulso de luz con su color y, al terminar, cambia a `Dialogue` y emite `OnRevealFinished`. `CompleteRevealEarly()` permite que una animación de Blueprint finalice la transformación antes del timer. No se aplican recompensas ni consecuencias en esta etapa.

El aspecto de los modelos sigue usando los cubos de reemplazo si no se asignan meshes/materiales en las variantes. El pulso de luz ya diferencia visualmente las variantes. Los Data Assets se crean con `Scripts/BuildMysteryPhoneStage5.py`, que conserva la configuración existente.

La prueba `ChopIt.Phone.Stage5.VariantsAndReveal` cubre pesos, límites, seed reproducible, incorporación de otra variante, IDs duplicados, selección única y transición de transformación a diálogo.
