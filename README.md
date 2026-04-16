# Nodo Sensor de Presión - ESP32-S3 (v2)

Este repositorio contiene el firmware para un **ESP32-S3** encargado de la adquisición, filtrado y envío proactivo de datos de presión. En esta versión (rama `v2_sensor-node`), el dispositivo no espera órdenes, sino que calcula y transmite la información automáticamente.

> [!IMPORTANT]
> **Dependencia de Red:** Para que los datos lleguen a la nube, este nodo debe estar conectado al **[Heltec V3 Bridge (LoRaWAN)](https://github.com/ualamc158/heltecV3LoRaWanSensorAnalogico)**, que actúa como pasarela hacia la red LoRaWAN.

## 🚀 Características de la Versión 2
* **Lectura Analógica:** Captura de datos mediante el ADC1 (Canal 3).
* **Calibración Dinámica (Tara):** Al arrancar, el sensor realiza un muestreo de 40 puntos para establecer el voltaje de residuo (zero offset).
* **Filtrado EMA:** Implementa un Filtro de Media Móvil Exponencial ($\alpha = 0.15$) para eliminar el ruido eléctrico del sensor de presión.
* **Envío Proactivo:** Transmisión automática de datos por UART cada 30 segundos (`P:XX.XX\n`).

## 🔌 Conexiones (Pinout)

| ESP32-S3 (Nodo Sensor) | Heltec V3 (Bridge LoRa) | Función |
| :--- | :--- | :--- |
| **GND** | **GND** | Masa de referencia (Obligatorio) |
| **GPIO 17** (TX) | **GPIO 35** (RX) | **Salida de Datos:** Envío de presión |
| **GPIO 18** (RX) | **GPIO 33** (TX) | Canal de control (Reservado) |

* **Sensor de Presión:** Conectado al pin analógico correspondiente al ADC1_CHANNEL_3 (GPIO 4 en la mayoría de esquemas de S3).

## 📊 Lógica de Software

El código opera en un bucle de 10Hz (100ms):
1.  **Muestreo:** Lee el ADC y convierte a milibares (mB) aplicando la calibración inicial.
2.  **Filtrado:** Suaviza la lectura para evitar picos falsos.
3.  **Contador:** Al alcanzar las 300 muestras (30 segundos), empaqueta el valor y lo envía por el puerto UART_1.

## 🏗️ Compilación

Diseñado para **ESP-IDF v5.x**.

```bash
idf.py set-target esp32s3
idf.py build flash monitor