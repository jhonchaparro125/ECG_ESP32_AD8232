# Monitor ECG con ESP32 y AD8232

Proyecto de un monitor de señal electrocardiográfica (ECG) realizado con un ESP32, un módulo AD8232 y una pantalla TFT ST7789V3.

## Descripción

El proyecto permite captar una señal ECG mediante el módulo AD8232, procesarla con el ESP32 y mostrarla en una pantalla TFT. En la pantalla se visualiza la señal ECG y un valor aproximado de la frecuencia cardíaca en BPM.

Durante las pruebas se realizaron diferentes posiciones de los electrodos y se observaron los cambios producidos en la señal.

## Componentes

- ESP32 de 38 pines ESP-WROOM-32
- Tarjeta de expansión para ESP32 de 38 pines
- Módulo AD8232
- Pantalla TFT ST7789V3 de 1.69"
- 3 electrodos adhesivos (RA, LA y RL)
- Cables de conexión
- Cable USB
- Computador para la programación

## Conexiones

### AD8232 → ESP32

| AD8232 | ESP32 |
|---|---|
| OUTPUT | GPIO34 |
| LO+ | GPIO32 |
| LO- | GPIO33 |
| 3.3V | 3.3V |
| GND | GND |

### TFT ST7789V3 → ESP32

| TFT | ESP32 |
|---|---|
| CS | GPIO5 |
| DC | GPIO2 |
| RST | GPIO4 |
| SCLK | GPIO18 |
| MOSI | GPIO23 |
| VCC | 3.3V |
| BLK | 3.3V |
| GND | GND |

La alimentación de 3.3 V y GND se distribuye mediante la tarjeta de expansión del ESP32.

## Funcionamiento

Los electrodos captan la actividad eléctrica del corazón y el módulo AD8232 acondiciona la señal para que pueda ser leída por el ESP32.

El ESP32 procesa la señal recibida y la muestra en la pantalla TFT en forma de gráfica. También se realiza el cálculo de la frecuencia cardíaca en BPM.

## Pruebas realizadas

Se realizaron pruebas colocando los electrodos principalmente en el pecho y también en los brazos y una pierna.

Al cambiar la posición de los electrodos se observaron cambios en la señal mostrada. En algunas posiciones la señal presentó mayores variaciones, especialmente cuando los electrodos no quedaban bien adheridos o se producía movimiento.

También se realizaron pruebas para observar el efecto de las interferencias eléctricas. Durante las pruebas se presentó interferencia al conectar el computador a la fuente de alimentación, afectando la señal mostrada en la pantalla.

## Archivos

- `ECG_AD8232.ino`: código utilizado para el funcionamiento del sistema.
- `diagrama.png`: diagrama de conexiones del sistema.
- Imágenes de las pruebas realizadas durante el proyecto.

## Nota

Este proyecto fue desarrollado con fines académicos para observar y procesar una señal ECG. El valor de BPM mostrado es aproximado y no debe utilizarse como instrumento médico.
