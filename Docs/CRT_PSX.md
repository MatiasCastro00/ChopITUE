# CRT en L_PSX_test

El efecto CRT ya está instalado en el `PostProcessVolume` de `L_PSX_test`. Lo forman `M_PSX_CRT_Post` y su instancia `MI_PSX_CRT`. Incluye líneas animadas, barrido tenue, curvatura y marco oscuro con esquinas redondeadas. El instalador `Tools/install_psx_crt.py` se puede volver a ejecutar sin duplicar el efecto ni sustituir los ajustes hechos en la instancia.

## Reinstalar en el mapa

Con el proyecto abierto en Unreal Editor, guarda cualquier cambio pendiente del nivel. Abre **Tools → Debug → Python Console** y ejecuta:

```python
exec(open(r"C:\Users\Elmr\Documents\Unreal Projects\ChopIt\Tools\install_psx_crt.py", encoding="utf-8").read())
```

El Output Log debe mostrar `PSX_CRT_OK`. El script guarda el material, la instancia y el mapa.

## Configurar

Abre `Content/ChopIt/Art/Materials/PSX_Materials/MI_PSX_CRT` y marca el checkbox junto a cada parámetro que quieras cambiar:

| Parámetro | Valor inicial | Qué hace |
| --- | ---: | --- |
| `EffectStrength` | 1.0 | Intensidad general. `0` desactiva el CRT sin quitarlo del mapa. |
| `ScanlineStrength` | 0.12 | Oscurecimiento de las líneas horizontales. |
| `ScanlineCount` | 360 | Cantidad de líneas verticalmente. |
| `ScanlineSpeed` | 0.4 | Velocidad con que se desplazan las líneas, en ciclos por segundo. `0` las deja quietas. |
| `RefreshStrength` | 0.05 | Brillo del barrido horizontal que recorre la pantalla. |
| `RefreshSpeed` | 0.18 | Velocidad del barrido, en vueltas de pantalla por segundo. |
| `VignetteStrength` | 0.16 | Oscurecimiento de las esquinas. |
| `ChromaticStrength` | 0.35 | Mezcla de la separación RGB. |
| `ColorOffsetPixels` | 0.6 | Separación horizontal de rojo y azul, en píxeles. |
| `ScreenCurvature` | 0.02 | Curvatura suave de la imagen, sin estirar los píxeles laterales. |
| `BorderWidth` | 0.025 | Ancho del marco oscuro, como fracción del lado de la pantalla. |
| `CornerRadius` | 0.11 | Redondeo de las esquinas interiores del marco. |
| `HorizontalWobblePixels` | 0.35 | Oscilación horizontal sutil de la imagen, en píxeles. `0` la desactiva. |

También puedes seleccionar el `PostProcessVolume` en `L_PSX_test`, buscar **Rendering Features → Post Process Materials** y cambiar el peso de `MI_PSX_CRT`: `1` es el efecto completo y `0` lo desactiva.
