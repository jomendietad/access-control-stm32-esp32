# Firmware del Nodo Local (STM32) - Control de Acceso

Este directorio contiene el código fuente del firmware para el microcontrolador STM32, el cual funciona como el **Nodo de Borde (Edge Node)** dentro del sistema distribuido de control de acceso. Su función principal es gestionar las interfaces físicas de entrada/salida en tiempo real, capturar las credenciales de las tarjetas RFID, evaluar la presencia de usuarios mediante un sensor PIR, operar el actuador de la cerradura electromecánica y proporcionar retroalimentación visual inmediata a través de una pantalla OLED.

Toda la lógica de validación remota y la conectividad avanzada se delegan al nodo central (ESP32-S3), con el cual este nodo se comunica de manera asíncrona.

> **NOTA DE MIGRACIÓN DE HARDWARE:** La implementación actual está configurada y validada para la tarjeta de desarrollo **NUCLEO-L476RG**. No obstante, la estructura del firmware ha sido diseñada bajo una arquitectura estrictamente desacoplada para permitir una migración directa a una PCB personalizada basada en el microcontrolador **STM32F0**. Los detalles técnicos para llevar a cabo esta transición se describen en la sección final de este documento.

---

## Arquitectura de Software (Strict 3-Layer Architecture)

El firmware se ha desarrollado siguiendo de forma rigurosa una **Arquitectura Modular de 3 Capas**, lo que garantiza que la lógica de negocio permanezca completamente aislada de los cambios de hardware o de fabricante. Todo el código fuente, incluyendo funciones, variables y comentarios, está redactado en inglés.

### 1. Main Layer (`src/main.c`)

Es el punto de entrada del programa. Se mantiene minimalista y libre de código de inicialización directa de periféricos. Su único propósito es incluir el archivo maestro de la aplicación (`app.h`), invocar la función de inicialización global de la lógica de negocio y mantener el bucle de ejecución principal de forma no bloqueante.

```c
#include "app.h"

int main(void) {
    App_Init();
    while (1) {
        App_Task();
    }
}

```

### 2. App Layer (`src/app/`)

Esta capa contiene la lógica de negocio y las reglas operativas del control de acceso. **Tiene estrictamente prohibido incluir librerías del fabricante (HAL) o archivos de cabecera del hardware.** Toda interacción con el entorno físico se realiza mediante las abstracciones provistas por el BSP.

* **`app.h`:** Cabecera maestra de la aplicación. Es el único archivo que interactúa directamente con `main.c`.
* **`app_core.c` / `.h`:** Implementa la Máquina de Estados Finitos (FSM) del sistema y las reglas de validación temporal utilizando marcas de tiempo no bloqueantes (`BSP_GetTicks()`).

**Máquina de Estados de la Aplicación (FSM)**

El sistema transita de manera cooperativa entre tres estados principales:

* **`STATE_IDLE`:** El sistema se encuentra en reposo. Borra la pantalla OLED, mantiene el relé de la cerradura desactivado, realiza sondeos continuos al lector RFID y verifica las banderas de recepción de la UART y del sensor PIR.
* **`STATE_ACCESS_GRANTED`:** Se activa cuando una credencial es válida (local o remota). Ordena al BSP energizar el relé para abrir la puerta y actualiza el display con el mensaje `"ACCESS GRANTED"`. Inicia un esquema de temporización no bloqueante para retornar automáticamente a `STATE_IDLE` tras 3000 ms.
* **`STATE_ACCESS_DENIED`:** Se activa ante un intento de acceso inválido. Muestra en la pantalla `"ACCESS DENIED"`, bloquea el actuador y regresa al estado `STATE_IDLE` tras un periodo de penalización.

**Reglas de Validación de Acceso**

El sistema concede el acceso si se cumple alguno de los siguientes escenarios mientras se encuentra en `STATE_IDLE`:

* **Validación Local (RFID):** El BSP reporta la lectura de una tarjeta cuyo identificador coincide exactamente con el UID autorizado almacenado localmente en el firmware.
* **Validación Remota Combinada (Visión Artificial + Presencia):** La capa de aplicación detecta que la UART ha recibido el carácter de validación exitosa desde el ESP32 (`'K'` de *Known*) y, de manera simultánea, el BSP confirma detección de movimiento en el sensor PIR. Esto previene falsos positivos por rostros detectados sin un usuario real presente.

### 3. BSP Layer (Board Support Package) (`src/bsp/`)

Representa la capa de abstracción de hardware. **Es la única sección del firmware autorizada para interactuar con la HAL (Hardware Abstraction Layer) de STM32 y manipular registros físicos.** Se encuentra modularizada por componentes físicos:

* **`bsp_placa.c` / `.h`:** Centraliza la inicialización de todos los periféricos, configura los árboles de reloj (Clock Tree) y expone la base de tiempo del sistema en milisegundos (`BSP_GetTicks()`) a través del SysTick.
* **`bsp_gpio.c` / `.h`:** Administra los pines de propósito general. Configura el pin del sensor PIR como entrada digital y el pin del relé de la cerradura como salida digital Push-Pull.
* **`bsp_uart.c` / `.h`:** Configura `USART2` para la salida de depuración (115200 bps) y `USART1` para la comunicación bidireccional con el ESP32-S3. La recepción de datos se maneja estrictamente mediante interrupciones de hardware (RXNE) para evitar bloqueos en la FSM.
* **`bsp_display.c` / `.h`:** Controlador optimizado para la pantalla OLED SSD1306 vía `I2C1`. Utiliza un buffer de video en la memoria RAM y transmite los cambios por el bus I2C de forma masiva, optimizando los tiempos de procesamiento.
* **`bsp_rfid.c` / `.h` y `mfrc522.c` / `.h`:** Implementan el protocolo SPI para la comunicación con el módulo RFID MFRC522 a través de `SPI1`. Gestionan la detección de tarjetas y la extracción del UID. *Nota: El UID autorizado (`0xB6, 0x7B, 0x0C, 0x03`) se encuentra definido temporalmente en este módulo.*

---

## Conexiones de Hardware y Pines (Configuración NUCLEO-L476RG)

| Periférico / Módulo | Señal | Pin STM32 (L476RG) | Configuración de Bus / Tipo |
| --- | --- | --- | --- |
| **MFRC522 (RFID)** | SPI_SCK | `PA5` | Alternate Function (`SPI1`) |
| **MFRC522 (RFID)** | SPI_MISO | `PA6` | Alternate Function (`SPI1`) |
| **MFRC522 (RFID)** | SPI_MOSI | `PA7` | Alternate Function (`SPI1`) |
| **MFRC522 (RFID)** | SPI_CS / NSS | `PB6` | GPIO Output (Software CS) |
| **MFRC522 (RFID)** | HARD_RESET | `PA9` | GPIO Output |
| **SSD1306 (OLED)** | I2C_SCL | `PB8` | Alternate Function (`I2C1`, Open-Drain) |
| **SSD1306 (OLED)** | I2C_SDA | `PB9` | Alternate Function (`I2C1`, Open-Drain) |
| **Sensor PIR** | DIGITAL_OUT | `PA0` | GPIO Input (Floating/Pull-Down) |
| **Relé Cerradura** | CONTROL_GATE | `PA1` | GPIO Output (Push-Pull) |
| **Conexión ESP32** | UART_TX (Hacia ESP) | `PA9` | Alternate Function (`USART1`) |
| **Conexión ESP32** | UART_RX (Desde ESP) | `PA10` | Alternate Function (`USART1`, RX Interrupt) |
| **Depuración PC** | ST-LINK TX | `PA2` | Alternate Function (`USART2`) |
| **Depuración PC** | ST-LINK RX | `PA3` | Alternate Function (`USART2`) |

---

## Flujo de Trabajo y Herramientas de Desarrollo

El desarrollo y compilación del firmware están completamente automatizados mediante **PlatformIO**, eliminando la dependencia de IDEs propietarios.

**Requisitos Previos**

* Instalación de **Visual Studio Code**.
* Instalación de la extensión **PlatformIO IDE**.

**Compilación y Despliegue**
Al abrir el directorio `stm32-nucleo-l476rg-control-access/` en VS Code, PlatformIO gestionará automáticamente las dependencias, herramientas de compilación y el framework STM32Cube HAL.

* **Build:** Compila el código fuente y genera los binarios (ELF/HEX/BIN) en el directorio `.pio/build/`.
* **Upload:** Se conecta a través del ST-LINK integrado (SWD), borra la memoria flash y graba el nuevo firmware.
* **Serial Monitor:** Abre la consola de depuración conectada al `USART2` (115200 baudios) para monitorear los registros del sistema y las transiciones de la FSM.

---

## Hoja de Ruta para la Migración a PCB Personalizada (STM32F0)

La arquitectura por capas asegura que **no se deba modificar el código de la capa `App` ni el archivo `main.c**` durante el proceso de migración. Las adaptaciones se limitan exclusivamente a la reconfiguración de la capa `BSP`. Siga los siguientes pasos:

### Paso 1: Configuración del Entorno (`platformio.ini`)

Abra el archivo `platformio.ini` y reemplace los parámetros de la placa Nucleo por los del chip STM32F0 correspondiente a su hardware:

```ini
[env:genericSTM32F030R8]
platform = ststm32
board = genericSTM32F030R8 ; Sustituir por el número de parte exacto
framework = stm32cube
monitor_speed = 115200

```

### Paso 2: Actualización de las Librerías HAL

En cada archivo de la carpeta `src/bsp/`, reemplace la cabecera de la familia L4 por la correspondiente a la familia F0:

```c
// Cambiar:
#include "stm32l4xx_hal.h"

// Por:
#include "stm32f0xx_hal.h"

```

### Paso 3: Reconfiguración de Pines y Funciones Alternas (AF)

Actualice las asignaciones de hardware basándose en el ruteo de la nueva PCB:

* **Pines de Propósito General (`bsp_gpio.c`):** Ajuste los puertos y pines asignados al sensor PIR y al relé.
* **Buses de Comunicación (`bsp_uart.c`, `bsp_display.c`, `bsp_rfid.c`):** Si los canales periféricos cambiaron (ej. de `SPI1` a `SPI2`), actualice las instancias en el código.
* **Mapeo de Funciones Alternas:** Verifique las macros de función alterna (ej. `GPIO_AFX_USARTX`). La familia STM32F0 emplea un mapa de registros diferente al de la serie L4.

### Paso 4: Ajuste del Árbol de Relojes (`bsp_placa.c`)

La serie STM32F0 no cuenta con el oscilador interno MSI de la serie L4. Localice la función `SystemClock_Config(void)` en `bsp_placa.c` y reescriba las estructuras `RCC_OscInitTypeDef` y `RCC_ClkInitTypeDef` para configurar el sistema utilizando el oscilador interno HSI (48 MHz) o un cristal externo HSE. Se recomienda generar una plantilla base con STM32CubeMX para asegurar la precisión de esta configuración.