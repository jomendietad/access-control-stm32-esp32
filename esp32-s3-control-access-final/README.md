# Firmware del Nodo de Conectividad y Procesamiento Avanzado (ESP32-S3) - Control de Acceso

Este directorio contiene el código fuente del firmware para el microcontrolador **ESP32-S3**, el cual actúa como el **Nodo Central de Conectividad y Procesamiento Avanzado (Core Node)** dentro del sistema distribuido de control de acceso. Su propósito principal es gestionar las tareas de alta carga lógica: administración de la pila de red (Wi-Fi), despliegue de servicios web integrados (servidor HTTP y WebSockets), procesamiento de algoritmos de visión artificial (`app_cv`) y el almacenamiento de registros en la memoria Flash no volátil.

Adicionalmente, el firmware incluye la base para el control de retroalimentación lumínica mediante tiras LED direccionables (WS2812), aunque **esta funcionalidad se encuentra inactiva en la lógica principal de la versión actual**.

Este nodo envía de manera asíncrona las solicitudes de hardware enviadas por el nodo periférico (STM32) mediante UART, evalúa el veredicto de acceso y notifica el resultado para autorizar o denegar la apertura física de la cerradura.

---

## Arquitectura de Software (Strict 3-Layer Architecture)

El firmware implementa de forma rigurosa una **Arquitectura Modular de 3 Capas**, aislando completamente la lógica de negocio de las APIs nativas del framework de Espressif (ESP-IDF) y de FreeRTOS. Todo el código fuente (funciones, variables, estructuras y comentarios) está redactado en inglés.

### 1. Main Layer (`main/main.c`)

Es el punto de entrada principal del chip (`app_main`). Su estructura es minimalista; se limita a incluir la cabecera maestra de la aplicación (`app.h`), invocar la función de inicialización del núcleo (`App_Init()`) e ingresar al bucle infinito ejecutando de forma cíclica y no bloqueante la tarea principal (`App_Task()`).

```c
#include "app.h"

void app_main(void) {
    App_Init();
    while (1) {
        App_Task();
    }
}

```

### 2. App Layer (`main/app/`)

Contiene las reglas de negocio, los algoritmos de toma de decisiones y el procesamiento de datos biométricos. **Se prohíbe estrictamente la inclusión de cabeceras nativas del SDK (como `esp_wifi.h`, `driver/gpio.h`, etc.) o llamadas directas a FreeRTOS.** Toda interacción con el hardware se canaliza mediante las abstracciones del BSP.

* **`app.h`:** Cabecera unificada de la capa de aplicación. Es el único enlace visible para el archivo `main.c`.
* **`app_core.c` / `.h`:** Coordina la Máquina de Estados Finitos (FSM) del sistema central. Procesa las tramas UART recibidas del STM32, solicita la validación facial al módulo de visión y determina si el acceso debe ser concedido. *(Nota: Aunque posee la estructura para coordinar efectos visuales, las llamadas hacia el driver de los LEDs no están implementadas en el flujo actual).*
* **`app_cv.c` / `.h`:** Submódulo encargado del procesamiento de imágenes y visión artificial. Para adaptarse a las limitaciones del microcontrolador, emplea un enfoque algorítmico tradicional, ultraligero y de bajo costo computacional:
1. **Detección Facial:** Utiliza segmentación por umbrales de color de piel (*Skin Color Thresholding*). Escanea el búfer de imagen buscando píxeles en el espacio RGB que cumplan reglas heurísticas estrictas para trazar una caja delimitadora alrededor del sujeto.
2. **Reconocimiento Facial:** Extrae características de la imagen recortada comprimiendo la región a una cuadrícula de 10x10 píxeles en una escala de grises aproximada (*Template Matching*).
3. **Inferencia:** Compara la cuadrícula de captura actual con la plantilla almacenada del usuario registrado evaluando el margen de error a través de la Suma de Diferencias al Cuadrado (SSD):

$$SSD = \sum_{i=0}^{99} (PixelActual_i - PixelPlantilla_i)^2$$



Si la diferencia es inferior al umbral predefinido, el sistema emite un veredicto de coincidencia exitosa (`'K'` - *Known*); de lo contrario, lo rechaza (`'U'` - *Unknown*).



### 3. BSP Layer (Board Support Package) (`main/bsp/`)

Es la capa de abstracción de hardware de bajo nivel. **Es la única sección del firmware autorizada para realizar llamadas a las funciones de ESP-IDF y operar primitivas de FreeRTOS.** Se encuentra modularizada por servicios físicos:

* **`bsp_placa.c` / `.h`:** Inicializa los componentes esenciales, coordina el orden de arranque de los subsistemas y expone la base de tiempo en milisegundos (`BSP_GetTicks()`).
* **`bsp_os.c` / `.h`:** Encapsula las funciones de FreeRTOS (creación de tareas, colas de mensajes y semáforos), evitando que la capa de aplicación dependa de tipos de datos nativos como `TaskHandle_t`.
* **`bsp_wifi.c` / `.h`:** Configura el periférico de radio, inicializa la pila TCP/IP de lwIP y gestiona el ciclo de vida de la conexión Wi-Fi en modo Estación (STA), incluyendo rutinas de reconexión automática.
* **`bsp_uart.c` / `.h`:** Configura el periférico UART interno para el enlace serie con el STM32. Utiliza interrupciones y buffers circulares para una recepción asíncrona no bloqueante (recepción implementada solo para una prueba de funcionamiento del UART en loopback).
* **`bsp_led.c` / `.h`:** Driver desarrollado para el control de la tira LED WS2812 mediante el periférico RMT o SPI. *Aclaración: El driver se encuentra programado y disponible a nivel de hardware, pero sus rutinas no son invocadas actualmente por la lógica principal de la aplicación.*
* **`bsp_storage.c` / `.h`:** Abstrae el almacenamiento en la memoria flash interna utilizando NVS (Non-Volatile Storage) o sistemas de archivos (SPIFFS/FAT) para la persistencia de configuraciones.
* **`bsp_tcp.c` / `.h` y `bsp_web.c` / `.h`:** Implementan la capa de sockets y el servidor web local. Exponen los endpoints HTTP y el canal de WebSockets para la transmisión en tiempo real de telemetría y video fluido (*streaming*) hacia un panel de control externo.

---

## Configuración de Periféricos y Recursos del Sistema

| Recurso / Periférico | Función en el Sistema | Configuración de Software (ESP-IDF) |
| --- | --- | --- |
| **UART Connection** | Enlace serie bidireccional con STM32 | Puerto `UART_NUM_1` (Pines TX/RX según ruteado) |
| **WS2812 LED Strip** | Driver lumínico | Periférico `RMT` / Driver `led_strip` *(Inactivo en App)* |
| **Wi-Fi Radio** | Conectividad a la red local | Modo `WIFI_MODE_STA` con DHCP activo |
| **NVS Flash** | Almacenamiento no volátil | Partición `nvs` por defecto en la tabla de memoria |
| **HTTP/WS Server** | API de control y WebSockets | Servidor asíncrono en puerto `80` |

El particionado de la memoria flash externa se encuentra definido explícitamente en el archivo `partitions.csv` ubicado en la raíz del proyecto, con el fin de optimizar el espacio asignado al almacenamiento y a las operaciones de visión artificial.

---

## Entorno de Desarrollo y Flujo de Trabajo (ESP-IDF Nativo)

A diferencia del nodo STM32 (gestionado por PlatformIO), el desarrollo y compilación de este firmware se realiza de manera exclusiva a través del framework **ESP-IDF nativo de Espressif**, utilizando el sistema de construcción **CMake** e integrado en **Visual Studio Code**.

### Requisitos e Instalación

1. Disponer de un entorno operativo compatible con herramientas de compilación de GNU (Toolchains).
2. Instalación de **Visual Studio Code**.
3. Instalación de la extensión **ESP-IDF Extension** oficial desde el marketplace.
4. Ejecutar el asistente de la extensión para descargar la versión estable del SDK de ESP-IDF, que incluye herramientas vitales como el compilador `xtensa-esp32s3-elf-gcc`, `cmake` y `ninja`.

### Configuración Inicial de Credenciales

Por directrices de seguridad, las credenciales de la red Wi-Fi no están incluidas en el repositorio. Es obligatorio configurarlas antes de iniciar el proceso de compilación:

1. Localice el archivo de plantilla `main/secrets.example.h`.
2. Genere una copia en el mismo directorio y renómbrela estrictamente como `secrets.h`.
3. Edite las macros en `main/secrets.h` con los parámetros de su red local:

```c
#define WIFI_SSID     "Tu_Nombre_De_Red"
#define WIFI_PASSWORD "Tu_Contraseña_Segura"

```

### Flujo de Compilación y Grabación desde VS Code

Abra la carpeta `esp32-s3-control-access-final/` en su editor. La extensión ESP-IDF detectará automáticamente el archivo `CMakeLists.txt`. El ciclo de desarrollo se controla mediante la barra de acciones inferior o la paleta de comandos:

* **Set Device Target:** Seleccione el microcontrolador `esp32s3` y la configuración base (`sdkconfig`).
* **Build:** Ejecuta la cadena de compilación de todo el firmware, generando los archivos binarios en el directorio `build/`.
* **Flash:** Conecte la placa ESP32-S3 vía USB. Seleccione el modo de grabación (UART o JTAG USB nativo) y el puerto correspondiente para transferir el firmware a la memoria flash.
* **Monitor:** Inicia la terminal serie de Espressif (configurada por defecto a 115200 baudios) para visualizar los registros de arranque del bootloader, las trazas del proceso de conexión Wi-Fi y la depuración del sistema.