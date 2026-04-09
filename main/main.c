#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/uart.h"

static const char *TAG = "SENSOR_PRESION";

// --- CONFIGURACIÓN DEL SENSOR ---
#define VOLTAJE_CERO_MB 0.445 

// --- CONFIGURACIÓN UART ---
#define UART_NUM        UART_NUM_1
#define TXD_PIN         17
#define RXD_PIN         18
#define UART_BAUD_RATE  115200
#define BUF_SIZE        1024

// Función para inicializar el UART
void init_uart(void) {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    // Instalar el driver UART
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    // Asignar los pines físicos
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    ESP_LOGI(TAG, "UART inicializado en TX:%d, RX:%d a %d baudios.", TXD_PIN, RXD_PIN, UART_BAUD_RATE);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando sistema de adquisicion y transmision...");

    // 1. Inicializar UART
    init_uart();

    // 2. Inicializar ADC (Sensor)
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,         
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config));

    char payload[64]; // Buffer para guardar el texto a enviar

    // Bucle principal
    while (1) {
        int adc_raw;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &adc_raw));

        // Matemáticas del sensor
        float voltaje_pin = (adc_raw / 4095.0) * 3.3;
        float voltaje_sensor = voltaje_pin * 1.5;
        float presion_mB = (voltaje_sensor - VOLTAJE_CERO_MB) * -250.0;

        // Imprimir en la consola local (Para depuración)
        ESP_LOGI(TAG, "V_Sensor: %.2f V | Presion: %.2f mB", voltaje_sensor, presion_mB);

        // Formatear el dato a enviar por UART (Agregamos \n al final para que el receptor sepa que terminó)
        int len = snprintf(payload, sizeof(payload), "P:%.2f\n", presion_mB);

        // Enviar por UART hacia el Heltec LoRa
        uart_write_bytes(UART_NUM, payload, len);

        // Esperar 1 segundo
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}