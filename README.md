# Lector de Sensor de Presión (ESP32-S3 Esclavo)

Este repositorio contiene el código para un **ESP32-S3** encargado de la lectura de sensores de presión analógicos y el filtrado de datos.

> 🔗 **Dependencia del Sistema:** Este nodo actúa como esclavo y envía datos únicamente cuando el **Nodo Maestro** lo solicita. Para que el sistema funcione, debe estar conectado físicamente a él.
> 👉 **[Repositorio del Nodo Maestro (Heltec V3)](https://github.com/ualamc158/heltecV3LoRaWanSensorAnalogico)**

---

## 🚀 1. ¿Qué hace este código?
Para garantizar lecturas estables y sin colisiones en la radio, el ESP32-S3 realiza tres tareas:

1. **Autocalibración (Tara):** Al arrancar, toma 40 muestras rápidas para establecer el "Voltaje Cero" (Offset dinámico). **Importante: No aplicar presión al encender la placa.**
2. **Filtrado:** Implementa un filtro pasa-bajos exponencial continuo (cada 100ms) para estabilizar la lectura y eliminar el ruido eléctrico.
3. **Comunicación Sincronizada:** Escucha por su puerto UART. Solo cuando recibe el comando `'G'` desde el Maestro, empaqueta el último valor filtrado y se lo envía.

---

## 🔌 2. Conexiones Físicas (Cables)

Utiliza los pines impresos en la placa para conectar el ESP32-S3 con el Heltec:

| Pin ESP32-S3 (Esclavo) | Pin Heltec (Maestro) | Función del Cable |
| :--- | :--- | :--- |
| **GND** | **GND** | **Masa Común:** ¡Obligatorio para compartir referencia eléctrica! |
| **18** (RX) | **33** (TX) | **Recibe Comando:** Escucha la orden `'G'` del Maestro. |
| **17** (TX) | **35** (RX) | **Envía Datos:** Transmite la lectura de presión (`P:XX.XX\n`). |

### Conexión del Sensor
- **VCC:** 3V3 o 5V (según tu sensor).
- **GND:** GND.
- **Señal (Out):** Conectado al **GPIO 4** (Configurado como ADC1_CH3).

---

## ⚙️ 3. Compilación y Ejecución
Este proyecto utiliza **ESP-IDF**.
1. Configura el target: `idf.py set-target esp32s3`
2. Construye y sube: `idf.py build flash monitor`