#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lora.h"

#if CONFIG_PRIMARY

#define TIMEOUT 100

void task_primary(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) {
		TickType_t nowTick = xTaskGetTickCount();
		int send_len = sprintf((char *)buf,"Hello World!! %d",nowTick);

#if 0
		// Maximum Payload size of SX1276/77/78/79 is 255
		memset(&buf[send_len], 0x20, 255-send_len);
		send_len = 255;
#endif

		lora_send_packet(buf, send_len);
		ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent:[%.*s]", send_len, send_len, buf);

		bool waiting = true;
		TickType_t startTick = xTaskGetTickCount();
		while(waiting) {
			lora_receive(); // put into receive mode
			if(lora_received()) {
				int receive_len = lora_receive_packet(buf, sizeof(buf));
				TickType_t currentTick = xTaskGetTickCount();
				TickType_t diffTick = currentTick - startTick;
				ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", receive_len, receive_len, buf);
				ESP_LOGI(pcTaskGetName(NULL), "Response time is %d MillSecs", diffTick * portTICK_RATE_MS);
				waiting = false;
			}
			TickType_t currentTick = xTaskGetTickCount();
			TickType_t diffTick = currentTick - startTick;
			ESP_LOGD(pcTaskGetName(NULL), "diffTick=%d", diffTick);
			if (diffTick > TIMEOUT) {
				ESP_LOGW(pcTaskGetName(NULL), "Respoce timeout");
				waiting = false;
			}
			vTaskDelay(1); // Avoid WatchDog alerts
		}
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
}
#endif // CONFIG_PRIMARY

#if CONFIG_SECONDARY
void task_seconfary(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) {
		lora_receive(); // put into receive mode
		if(lora_received()) {
			int receive_len = lora_receive_packet(buf, sizeof(buf));
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", receive_len, receive_len, buf);
			for (int i=0;i<receive_len;i++) {
				if (isupper(buf[i])) {
					buf[i] = tolower(buf[i]);
				} else {
					buf[i] = toupper(buf[i]);
				}
			}
			vTaskDelay(1);
			lora_send_packet(buf, receive_len);
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent back...", receive_len);
		}
		vTaskDelay(1); // Avoid WatchDog alerts
	}
}
#endif // CONFIG_SECONDARY

void app_main()
{
	lora_init();

#if CONFIG_169MHZ
  ESP_LOGI(pcTaskGetName(NULL), "Frequency is 169MHz");
  lora_set_frequency(169e6); // 169MHz
#elif CONFIG_433MHZ
  ESP_LOGI(pcTaskGetName(NULL), "Frequency is 433MHz");
  lora_set_frequency(433e6); // 433MHz
#elif CONFIG_470MHZ
  ESP_LOGI(pcTaskGetName(NULL), "Frequency is 470MHz");
  lora_set_frequency(470e6); // 470MHz
#elif CONFIG_866MHZ
  ESP_LOGI(pcTaskGetName(NULL), "Frequency is 866MHz");
  lora_set_frequency(866e6); // 866MHz
#elif CONFIG_915MHZ
  ESP_LOGI(pcTaskGetName(NULL), "Frequency is 915MHz");
  lora_set_frequency(915e6); // 915MHz
#elif CONFIG_OTHER
  ESP_LOGI(pcTaskGetName(NULL), "Frequency is %dMHz", CONFIG_OTHER_FREQUENCY);
  long frequency = CONFIG_OTHER_FREQUENCY * 1000000;
  lora_set_frequency(frequency);
#endif

	lora_enable_crc();

#if CONFIG_PRIMARY
	xTaskCreate(&task_primary, "task_primary", 1024*2, NULL, 5, NULL);
#endif
#if CONFIG_SECONDARY
	xTaskCreate(&task_seconfary, "task_secondary", 1024*2, NULL, 5, NULL);
#endif
}