# Sistema de Control de Acceso Distribuido (STM32 + ESP32-S3)

Este repositorio contiene el código fuente para un **Sistema de Control de Acceso Distribuido**. El proyecto resuelve la necesidad de validar ingresos físicos mediante autenticación de doble factor (RFID local y Visión Artificial remota), dividiendo las responsabilidades computacionales y de control físico entre dos microcontroladores distintos.

Esta arquitectura de "doble nodo" permite aislar y proteger la interacción física en tiempo real, delegando las tareas de procesamiento intensivo, almacenamiento de datos y conectividad web a un nodo especializado.

---

## Filosofía de Diseño: Arquitectura de Nodos

El sistema abandona el paradigma monolítico clásico para operar bajo una topología de **Nodo de Borde (Edge Node)** y **Nodo Central (Core Node)**.

### 1. Nodo Físico / Edge Node (STM32)

Su prioridad absoluta es garantizar la seguridad física y proporcionar una respuesta determinista en tiempo real.

* **Responsabilidades:** Gestión del lector de tarjetas RFID (MFRC522), lectura del sensor de presencia volumétrica (PIR), control de la interfaz visual de usuario (OLED SSD1306) y accionamiento de potencia del relé de la cerradura electromecánica.
* **Estado de Hardware:** Implementado transitoriamente en una tarjeta de desarrollo **NUCLEO-L476RG**. La estructura del firmware está diseñada para garantizar una migración directa y transparente hacia la placa definitiva: una **PCB personalizada basada en el microcontrolador STM32F0**.

### 2. Nodo Central / Core Node (ESP32-S3)

Actúa como el motor computacional del sistema. Gestiona tareas asíncronas de alta carga que comprometerían el tiempo de respuesta del nodo STM32.

* **Responsabilidades:** Procesamiento de algoritmos de visión artificial, gestión de conectividad inalámbrica, alojamiento del servidor HTTP/WebSockets y registro persistente de eventos en memoria no volátil.
* **Nota sobre Retroalimentación Visual (LEDs):** Aunque la capa BSP del ESP32 incorpora el controlador para tiras LED direccionables WS2812 (`bsp_led`), **esta funcionalidad se encuentra inactiva en el flujo principal (`main`) y en la máquina de estados actual**. Las notificaciones visuales del sistema operan exclusivamente a través de la pantalla OLED del STM32.

---

## Diagrama de Bloques del Sistema

El siguiente diagrama ilustra la separación de recursos de hardware y el puente de comunicación asíncrona entre ambas arquitecturas:

```mermaid
graph TD
    %% --- NODO FISICO: STM32 ---
    subgraph STM32 ["EDGE NODE: STM32 (Control Físico)"]
        direction TB
        
        subgraph SwSTM ["Capa de Software (Firmware)"]
            M_STM["Capa Main<br><i>(main.c / App_Task)</i>"]
            A_STM["Capa App: FSM Local<br><i>(app_core.c)</i>"]
            B_STM["Capa BSP: Drivers Placa<br><i>(bsp_placa.h)</i>"]
            
            M_STM --> A_STM
            A_STM <--> B_STM
        end

        %% Periféricos Físicos
        RFID["Lector RFID<br><b>(MFRC522)</b>"]
        PIR["Sensor Presencia<br><b>(PIR)</b>"]
        RELE["Cerradura<br><b>(Relé Potencia)</b>"]
        OLED["Interfaz Usuario<br><b>(OLED SSD1306)</b>"]

        %% Conexiones de Hardware Internas
        B_STM <-->|Bus SPI| RFID
        PIR -->|GPIO Input| B_STM
        B_STM -->|GPIO Output| RELE
        B_STM -->|Bus I2C| OLED
    end

    %% --- NODO COMPUTACIONAL: ESP32-S3 ---
    subgraph ESP32 ["CORE NODE: ESP32-S3 (Procesamiento)"]
        direction TB
        
        subgraph SwESP ["Capa de Software (Infraestructura)"]
            M_ESP["Capa Main<br><i>(main.c)</i>"]
            A_ESP["Capa App: Visión/Lógica<br><i>(app_cv.c)</i>"]
            B_ESP["Capa BSP: Drivers Placa<br><i>(bsp_placa.h)</i>"]
            
            M_ESP --> A_ESP
            A_ESP <--> B_ESP
        end

        %% Periféricos / Conectividad
        RED(("Red Local / Web<br><b>(HTTP / WebSockets)</b>"))
        MEM["Memoria Flash<br><b>(NVS)</b>"]
        LED["Feedback Visual<br><b>(WS2812 RGB)</b>"]

        %% Conexiones de Hardware Internas
        B_ESP <-->|Wi-Fi / TCP-IP| RED
        B_ESP <-->|Bus Interno| MEM
        B_ESP -.->|Driver Inactivo| LED
    end

    %% --- BUS DE COMUNICACIÓN INTER-NODO ---
    B_STM <-->|Canal UART Asíncrono<br>RX / TX Directo| B_ESP

    %% --- ESTILOS VISUALES (Clases CSS para Escaneabilidad) ---
    classDef mainLayer fill:#e3f2fd,stroke:#1565c0,stroke-width:2px,color:#0d47a1;
    classDef appLayer fill:#fff3e0,stroke:#ef6c00,stroke-width:2px,color:#e65100;
    classDef bspLayer fill:#e8f5e9,stroke:#2e7d32,stroke-width:2px,color:#1b5e20;
    classDef hw fill:#f5f5f5,stroke:#424242,stroke-width:1px,color:#212121;
    classDef inactive fill:#fafafa,stroke:#b0bec5,stroke-width:1px,color:#90a4ae,stroke-dasharray: 5 5;

    %% Asignación de Estilos
    class M_STM,M_ESP mainLayer;
    class A_STM,A_ESP appLayer;
    class B_STM,B_ESP bspLayer;
    class RFID,PIR,RELE,OLED,RED,MEM hw;
    class LED inactive;


```

---

## Arquitectura de Software (Strict 3-Layer Architecture)

Para facilitar la inminente migración de hardware (de Nucleo a la PCB personalizada) sin necesidad de refactorizar la lógica central, **ambos microcontroladores** implementan de forma rigurosa una arquitectura de software de tres capas. La totalidad del código fuente está redactada en inglés.

1. **Capa BSP (Board Support Package):** Modularizada por componente físico (ej. `bsp_uart.c`, `bsp_rfid.c`). Es la **única capa autorizada** para interactuar con las bibliotecas del fabricante (HAL de STM32 o ESP-IDF) y manipular registros físicos. Todos los submódulos convergen en un archivo maestro `bsp_placa.h`.
2. **Capa App (Business Logic):** Modularizada por funcionalidad (ej. `app_core.c`, `app_cv.c`). Es completamente agnóstica al hardware; está **estrictamente prohibida** la inclusión de directivas de registros o manejo de periféricos a este nivel. Contiene las máquinas de estado y expone sus interfaces a través de `app.h`.
3. **Capa Main:** Diseñada con un enfoque minimalista. Únicamente incluye `app.h`, ejecuta la inicialización global (`App_Init()`) y mantiene un bucle infinito dedicado de forma exclusiva a la invocación de `App_Task()`.

---

## Flujo de Ejecución y Casos de Uso

El núcleo operativo del sistema reside en la Máquina de Estados Finitos (FSM) ejecutada en la capa de aplicación del STM32 (`app_core.c`). Este bucle principal (`App_Task`) opera de manera estrictamente asíncrona, evitando bloqueos por funciones de retardo.

### Estado Inicial: `STATE_IDLE`

Representa el estado de reposo y vigilancia. El relé permanece desenergizado (cerradura bloqueada). La pantalla OLED se limpia o muestra un mensaje de espera. En cada iteración del bucle, el STM32 ejecuta tres acciones:

1. Interroga el bus SPI para detectar la presencia de una tarjeta en el lector RFID.
2. Verifica si la rutina de interrupción UART ha señalado la recepción de un comando desde el ESP32.
3. Evalúa el estado lógico del pin asociado al sensor PIR.

A partir de este estado de vigilancia, pueden desencadenarse tres eventos principales:

### Caso 1: Ingreso Concedido por RFID (Validación Local)

1. El usuario aproxima un transpondedor RFID al lector.
2. La capa BSP extrae el UID de la tarjeta y lo transfiere a la capa App.
3. La capa App compara el UID contra la base de datos almacenada localmente.
4. **Al verificar una coincidencia**, la FSM transiciona a `STATE_ACCESS_GRANTED`.
5. Se comanda al BSP para energizar el relé y liberar la cerradura. El display OLED indica **"ACCESS GRANTED"**.
6. Se captura la marca de tiempo actual (`BSP_GetTicks()`). Una vez expirado el intervalo de apertura preconfigurado (ej. 3000 ms), el sistema revierte automáticamente a `STATE_IDLE`.

### Caso 2: Ingreso Concedido por Visión Artificial (Validación Remota + Físico)

1. El ESP32-S3 captura y procesa continuamente secuencias de video buscando identificar un rostro válido.
2. Tras identificar positivamente a un usuario autorizado, el ESP32 transmite un byte de confirmación vía UART: `'K'` (*Known*).
3. La interfaz UART del STM32 recibe el byte asíncronamente y activa una bandera de validación.
4. En el siguiente ciclo de `STATE_IDLE`, la capa App detecta esta bandera y ejecuta la comprobación de **Seguridad de Factor Físico**:
* *¿Se recibió la validación `'K'` y, simultáneamente, el sensor PIR registra nivel ALTO (confirmando la presencia física frente a la puerta)?*


5. Si ambas condiciones se cumplen (mitigando vulnerabilidades por suplantación fotográfica), la FSM transiciona a `STATE_ACCESS_GRANTED` y procede con la rutina de apertura.

### Caso 3: Ingreso Denegado

Este escenario se activa bajo dos premisas:

* **Rechazo Local:** Se detecta una tarjeta RFID, pero su UID no existe en los registros locales.
* **Rechazo Remoto:** El ESP32 procesa un rostro no autorizado y transmite el byte `'U'` (*Unknown*).
* **Flujo Operativo:** La FSM transiciona a `STATE_ACCESS_DENIED`. El relé conserva su estado inactivo (seguridad en modo a prueba de fallos). El display OLED muestra **"ACCESS DENIED"** durante un periodo de penalización, tras el cual la pantalla se limpia y el sistema retorna a `STATE_IDLE`.

```mermaid
flowchart TD
    Start([Bucle Continuo: App_Task]) --> CheckState{"¿Cuál es el<br>Estado Actual?"}
    
    %% ==========================================
    %% --- SECCIÓN: STATE_IDLE ---
    %% ==========================================
    CheckState -->|STATE_IDLE| Poll["Sondear Capa BSP<br><i>(MFRC522, UART, PIR)</i>"]
    Poll --> CondRFID{"¿Tarjeta RFID<br>detectada?"}
    
    %% Rama de Validación RFID
    CondRFID -->|Sí| CheckUID{"¿UID registrado<br>en código?"}
    CheckUID -->|Sí| LockG["Cambiar Estado a:<br><b>STATE_ACCESS_GRANTED</b><br>Guardar t_actual (Ticks)"]
    CheckUID -->|No| LockD["Cambiar Estado a:<br><b>STATE_ACCESS_DENIED</b><br>Guardar t_actual (Ticks)"]
    
    %% Rama de Validación UART + PIR (Se alinea a la izquierda para evitar cruces)
    CondRFID -->|No| CondK{"¿Llegó comando 'K'<br><i>(Rostro Conocido)</i>?"}
    CondK -->|Sí| CheckPIR{"¿Sensor PIR == ALTO?<br><i>(Filtro Anti-Spoofing)</i>"}
    CheckPIR -->|Sí| LockG
    CheckPIR -->|No| ClearK["Limpiar Flag UART<br><i>(Ignorar por seguridad)</i>"] --> End
    
    %% Rama de Rostro Desconocido (Se alinea a la derecha)
    CondK -->|No| CondU{"¿Llegó comando 'U'<br><i>(Desconocido)</i>?"}
    CondU -->|Sí| LockD
    CondU -->|No| End

    %% ==========================================
    %% --- SECCIÓN: STATE_ACCESS_GRANTED ---
    %% ==========================================
    CheckState -->|STATE_ACCESS_GRANTED| ExecG["Acción de Apertura"]
    ExecG --> ActG1["BSP_Relay(ON)<br>OLED = 'ACCESS GRANTED'"]
    ActG1 --> TimeG{"¿Transcurrieron<br>3000 ms?<br><i>(Ticks - t_actual)</i>"}
    
    %% Bucle de retención de tiempo
    TimeG -->|No| ActG1
    TimeG -->|Sí| ResetG["BSP_Relay(OFF)<br>Limpiar Banderas UART<br>Cambiar Estado a: <b>STATE_IDLE</b>"] --> End

    %% ==========================================
    %% --- SECCIÓN: STATE_ACCESS_DENIED ---
    %% ==========================================
    CheckState -->|STATE_ACCESS_DENIED| ExecD["Acción de Rechazo"]
    ExecD --> ActD1["BSP_Relay(OFF)<br>OLED = 'ACCESS DENIED'"]
    ActD1 --> TimeD{"¿Expiró tiempo<br>de penalización?"}
    
    %% Bucle de retención de tiempo
    TimeD -->|No| ActD1
    TimeD -->|Sí| ResetD["Limpiar Pantalla OLED<br>Limpiar Banderas UART<br>Cambiar Estado a: <b>STATE_IDLE</b>"] --> End

    %% Nodo de salida
    LockG --> End
    LockD --> End
    End([Fin de la Iteración])

    %% ==========================================
    %% --- ESTILOS VISUALES (Clases CSS) ---
    %% ==========================================
    classDef stateNode fill:#e3f2fd,stroke:#1565c0,stroke-width:2px,color:#0d47a1;
    classDef decision fill:#fff3e0,stroke:#ef6c00,stroke-width:2px,color:#e65100;
    classDef granted fill:#e8f5e9,stroke:#2e7d32,stroke-width:2px,color:#1b5e20;
    classDef denied fill:#ffebee,stroke:#c62828,stroke-width:2px,color:#b71c1c;
    classDef normal fill:#f5f5f5,stroke:#424242,stroke-width:1px,color:#212121;

    %% Asignación de Estilos
    class Start,End,CheckState,StateIdle stateNode;
    class CondRFID,CheckUID,CondK,CheckPIR,CondU,TimeG,TimeD decision;
    class LockG,ExecG,ActG1,ResetG granted;
    class LockD,ExecD,ActD1,ResetD denied;
    class Poll,ClearK normal;

```

---

## Estructura del Repositorio

El repositorio separa de forma lógica y física los entornos de desarrollo para prevenir conflictos entre dependencias, agilizar el proceso de compilación y centralizar la documentación de hardware.

```text
/
├── PCB-Design/                        # Archivos de hardware y manufactura de la PCB custom
│   ├── Altium/                        # Archivos fuente de Altium Designer (esquemáticos, PCB layout)
│   └── gerbers.zip                    # Archivos Gerber listos para fabricación (basada en STM32F0)
│
├── esp32-s3-control-access-final/     # Firmware del ESP32-S3
│   ├── main/                          # Código fuente (App, BSP, Main)
│   ├── CMakeLists.txt                 # Sistema de construcción de ESP-IDF
│   └── partitions.csv                 # Mapa de particiones NVS y de la aplicación
│
├── stm32-nucleo-l476rg-control-access/# Firmware del STM32
│   ├── src/                           # Código fuente (App, BSP, Main)
│   ├── platformio.ini                 # Configuración de entorno y especificación de hardware
│   └── README.md                      # Documentación para la migración a STM32F0
│
└── proyecto.code-workspace            # Espacio de trabajo integrado para Visual Studio Code

```

## Herramientas de Desarrollo

* **Entorno STM32:** El desarrollo, compilación y despliegue se administran a través del ecosistema **PlatformIO**.
* **Entorno ESP32-S3:** El desarrollo se rige estrictamente por la **extensión oficial de ESP-IDF** para VS Code, empleando CMake como sistema de construcción nativo.
* **Diseño de Hardware:** La captura esquemática y el diseño de la placa de circuito impreso (PCB) se realizan en **Altium Designer**, generando los archivos estandarizados Gerber para la manufactura física.