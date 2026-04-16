#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/uart.h"

static const char *TAG = "CEREBRO_SENSOR";

#define UART_NUM         UART_NUM_1
#define TXD_PIN          17 // Al RX del Heltec (Pin 35)
#define RXD_PIN          18 // Al TX del Heltec (Pin 33)
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

    // Calibración inicial (Tara)
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
    int contador_muestras = 0;

    while (1) {
        // LEER Y FILTRAR SIEMPRE
        int raw;
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &raw);
        float v = ((raw / 4095.0) * 3.3) * 1.5;
        float p_inst = (v - voltaje_zero_dinamico) * -250.0;

        if (primera) { presion_filtrada = p_inst; primera = false; }
        else { presion_filtrada = (alfa * p_inst) + ((1.0 - alfa) * presion_filtrada); }

        // Mostrar cada muestra por terminal (Dato fresco)
        ESP_LOGI(TAG, "Presion Actual (Filtrada): %.2f mB", presion_filtrada);

        // Cada 30 segundos (con vTaskDelay 100ms, son 300 iteraciones)
        contador_muestras++;
        if (contador_muestras >= 300) {
            char p_msg[64];
            int len = snprintf(p_msg, sizeof(p_msg), "P:%.2f\n", presion_filtrada);
            
            // Enviar proactivamente por el cable
            uart_write_bytes(UART_NUM, p_msg, len);
            ESP_LOGW(TAG, ">>> MEDIA DE 30s ENVIADA AL HELTEC: %s", p_msg);
            
            contador_muestras = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Muestreo a 10Hz
    }
}