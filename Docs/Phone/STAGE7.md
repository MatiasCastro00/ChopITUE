# Teléfono misterioso — Etapa 7: modelo 3D

El teléfono del mundo y su captura de diálogo usan ahora `SM_RotaryPhoneBase` y `SM_RotaryPhoneReceiver`. Son meshes low-poly de un teléfono de disco: carcasa biselada, dial, diez posiciones de números, soportes, patas y auricular. Los archivos fuente OBJ/MTL están en `SourceArt/Phone` y `Scripts/BuildMysteryPhoneStage7.py` los importa y configura en Unreal. El actor conserva los cubos como respaldo si faltan ambos meshes.

La carcasa, el caucho, el latón y el disco de marfil son materiales separados. `DA_PhoneDevil` y `DA_PhoneAngel` comparten la geometría y cambian el material de carcasa durante la revelación; los colores y cualquier reemplazo de mesh o material siguen editables desde cada Data Asset. `M_PhoneShellRed` es la apariencia inicial antes de contestar.

La prueba `ChopIt.Phone.Stage7.RotaryVisuals` verifica la carga, dimensiones, materiales, configuración de ambas variantes y apariencia inicial del actor. Las ocho pruebas del teléfono pasan. La previsualización `Saved/PhonePreview.png` es un render sencillo del OBJ para comprobar la silueta; aún hace falta revisar la iluminación y composición final en PIE.
