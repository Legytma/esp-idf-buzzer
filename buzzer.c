/**
 * Copyright 2023 Legytma Soluções Inteligentes LTDA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * buzzer.c
 *
 *  Created on: 01 de fev de 2023
 *      Author: Alex Manoel Ferreira Silva
 */

#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "driver/ledc.h"

#include "log_utils.h"

#include "sdkconfig.h"

#include "buzzer.h"

LOG_TAG("buzzer");

static TaskHandle_t      buzzer_task_handle    = NULL;
static QueueHandle_t     buzzer_queue          = NULL;
static SemaphoreHandle_t buzzer_semaphore      = NULL;
static SemaphoreHandle_t buzzer_beep_semaphore = NULL;
static bool              is_plaing             = false;

static void buzzer_task(void* arg) {
	// uint8_t _pin = *((uint8_t*)arg);
	uint8_t _pin = CONFIG_BUZZER_GPIO;

	if (buzzer_beep_semaphore == NULL) {
		buzzer_beep_semaphore = xSemaphoreCreateBinary();

		xSemaphoreGive(buzzer_beep_semaphore);
	}

	while (true) {
		buzzer_params_t buzzer_params;

		if (xQueueReceive(buzzer_queue, &buzzer_params, portMAX_DELAY) ==
			pdTRUE) {
			if (buzzer_params.frequency > 0) {
				ledc_timer_config_t ledc_timer = {
					.speed_mode      = LEDC_LOW_SPEED_MODE,
					.timer_num       = LEDC_TIMER_0,
					.duty_resolution = LEDC_TIMER_13_BIT,
					.freq_hz         = buzzer_params.frequency,
					.clk_cfg         = LEDC_AUTO_CLK,
				};

				ledc_channel_config_t ledc_channel = {
					.speed_mode = LEDC_LOW_SPEED_MODE,
					.channel    = LEDC_CHANNEL_0,
					.timer_sel  = LEDC_TIMER_0,
					.intr_type  = LEDC_INTR_DISABLE,
					.gpio_num   = _pin,
					.duty       = 4096,
					.hpoint     = 0,
				};

				is_plaing = (ledc_timer_config(&ledc_timer) == ESP_OK) &&
							(ledc_channel_config(&ledc_channel) == ESP_OK);
			} else if (is_plaing) {
				is_plaing =
					ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0) != ESP_OK;
			}

			if (buzzer_params.duration != 0) {
				xSemaphoreTake(buzzer_beep_semaphore, 0);
				xSemaphoreTake(buzzer_beep_semaphore,
							   pdMS_TO_TICKS(buzzer_params.duration));
				xSemaphoreGive(buzzer_beep_semaphore);

				if (is_plaing) {
					is_plaing = ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
										  0) != ESP_OK;
				}
			}
		}
	}
}

static inline void buzzer_check_semaphore() {
	if (buzzer_semaphore == NULL) {
		buzzer_semaphore = xSemaphoreCreateBinary();

		xSemaphoreGive(buzzer_semaphore);
	}
}

bool buzzer_init(/*uint8_t _pin*/) {
	bool ret = false;

	buzzer_check_semaphore();

	if (xSemaphoreTake(buzzer_semaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
		if (buzzer_task_handle == NULL) {
			if (buzzer_queue == NULL) {
				buzzer_queue = xQueueCreate(256, sizeof(buzzer_params_t));

				if (buzzer_queue == NULL) {
					ret = false;

					LOGE("Fail on create queue!");
					goto unlock_and_exit;
				}
			}

			ret = xTaskCreate(buzzer_task, "buzzer_task", 4096, NULL /*&_pin*/,
							  10, &buzzer_task_handle) == pdPASS;

			if (!ret) {
				LOGE("Fail on create buzzer_task!");
			}

		unlock_and_exit:
			xSemaphoreGive(buzzer_semaphore);
		} else {
			ret = true;
		}
	} else {
		LOGE("Fail on take semaphore!");
	}

	return ret;
}

void buzzer_deinit() {
	buzzer_check_semaphore();

	if (xSemaphoreTake(buzzer_semaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
		if (buzzer_task_handle != NULL) {
			vTaskDelete(buzzer_task_handle);

			if (buzzer_queue != NULL) {
				vQueueDelete(buzzer_queue);

				buzzer_queue = NULL;
			}

			if (buzzer_beep_semaphore != NULL) {
				vSemaphoreDelete(buzzer_beep_semaphore);

				buzzer_beep_semaphore = NULL;
			}

			buzzer_task_handle = NULL;
		}

		xSemaphoreGive(buzzer_semaphore);
	}
}

size_t buzzer_play_melody(buzzer_melody_t buzzer_melody) {
	size_t ret = 0;

	for (int thisNote = 0; thisNote < buzzer_melody.size; thisNote++) {
		buzzer_note_t buzzer_note = buzzer_melody.notes[thisNote];

		ret += buzzer_play_note(buzzer_melody.tempo, buzzer_note);
	}

	return ret;
}

size_t buzzer_play_note(uint16_t tempo, buzzer_note_t buzzer_note) {
	buzzer_params_t buzzer_params = {
		.frequency = buzzer_note.note,
		.duration  = (((60000 * 4) / tempo) / buzzer_note.duration),
	};

	buzzer_play_tone(buzzer_params);

	return buzzer_params.duration;
}

void buzzer_play_tone(buzzer_params_t buzzer_params) {
	// if (buzzer_init(_pin)) {
	if (buzzer_queue != NULL) {
		xQueueSend(buzzer_queue, &buzzer_params, pdMS_TO_TICKS(1000));
	}
}

void buzzer_clear_buffer() {
	// if (buzzer_init(_pin)) {
	if (buzzer_queue != NULL) {
		xQueueReset(buzzer_queue);

		if (is_plaing) {
			xSemaphoreGive(buzzer_beep_semaphore);

			ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
		}
	}
}
