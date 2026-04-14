#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/uart.h"

static const char *TAG = "DEBUG_ESCLAVO";

#define UART_NUM         UART_NUM_1
#define TXD_PIN          17 // Al pin 35 del Heltec
#define RXD_PIN          18 // Al pin 33 del Heltec
#define UART_BAUD_RATE   115200

float voltaje_zero_dinamico = 0.502;

void init_uart(void) {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void app_main(void) {
    init_uart();
    
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_cfg = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc1_handle));
    adc_oneshot_chan_cfg_t config = { .bitwidth = ADC_BITWIDTH_DEFAULT, .atten = ADC_ATTEN_DB_12 };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config));

    // Calibración inicial
    ESP_LOGI(TAG, "Calibrando...");
    float suma = 0;
    for(int i=0; i<40; i++) {
        int r; adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &r);
        suma += (r / 4095.0) * 3.3 * 1.5;
        vTaskDelay(pdMS_TO_TICKS(25));
    }
    voltaje_zero_dinamico = suma / 40.0;
    ESP_LOGI(TAG, "Calibracion OK. Offset: %.3fV", voltaje_zero_dinamico);

    float presion_filtrada = 0;
    float alfa = 0.15;
    bool primera = true;

    ESP_LOGI(TAG, "Esperando orden 'G' desde el Heltec (GPIO 18)...");

    while (1) {
        // LEER Y FILTRAR SIEMPRE (Para que el dato esté fresco)
        int raw;
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &raw);
        float v = ((raw / 4095.0) * 3.3) * 1.5;
        float p_inst = (v - voltaje_zero_dinamico) * -250.0;

        if (primera) { presion_filtrada = p_inst; primera = false; }
        else { presion_filtrada = (alfa * p_inst) + ((1.0 - alfa) * presion_filtrada); }

        // REVISAR UART
        uint8_t cmd;
        int n = uart_read_bytes(UART_NUM, &cmd, 1, pdMS_TO_TICKS(5));
        
        if (n > 0) {
            if (cmd == 'G') {
                // --- VISUALIZACIÓN DEL EVENTO ---
                ESP_LOGW(TAG, "¡ORDEN RECIBIDA! -> [G]");
                
                char p[64];
                int len = snprintf(p, sizeof(p), "P:%.2f\n", presion_filtrada);
                
                ESP_LOGI(TAG, "Dato leido del sensor: %.2f mB", presion_filtrada);
                
                // Mando por el cable
                uart_write_bytes(UART_NUM, p, len);
                
                ESP_LOGE(TAG, "Enviado por UART (GPIO 17): %s", p);
                ESP_LOGI(TAG, "------------------------------------------");
            } else {
                ESP_LOGD(TAG, "Recibido caracter inesperado: %c", cmd);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}