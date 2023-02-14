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

static void buzzer_task(void* args) {
	buzzer_config_t* buzzer_config = (buzzer_config_t*)args;

	if (buzzer_config->beep_semaphore == NULL) {
		buzzer_config->beep_semaphore = xSemaphoreCreateBinary();

		xSemaphoreGive(buzzer_config->beep_semaphore);
	}

	while (true) {
		buzzer_params_t buzzer_params;

		if (xQueueReceive(buzzer_config->queue, &buzzer_params,
						  portMAX_DELAY) == pdTRUE) {
			if (buzzer_params.frequency > 0) {
				ledc_timer_config_t ledc_timer = {
					.speed_mode      = LEDC_LOW_SPEED_MODE,
					.timer_num       = LEDC_TIMER_0,
					.duty_resolution = LEDC_TIMER_13_BIT,
					.freq_hz         = buzzer_params.frequency,
					.clk_cfg         = LEDC_AUTO_CLK,
				};

				buzzer_config->is_plaing =
					(ledc_timer_config(&ledc_timer) == ESP_OK) &&
					(ledc_channel_config(&buzzer_config->ledc_channel_config) ==
					 ESP_OK);
			} else if (buzzer_config->is_plaing) {
				buzzer_config->is_plaing =
					ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0) != ESP_OK;
			}

			if (buzzer_params.duration != 0) {
				xSemaphoreTake(buzzer_config->beep_semaphore, 0);
				xSemaphoreTake(buzzer_config->beep_semaphore,
							   pdMS_TO_TICKS(buzzer_params.duration));
				xSemaphoreGive(buzzer_config->beep_semaphore);

				if (buzzer_config->is_plaing) {
					buzzer_config->is_plaing =
						ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0) !=
						ESP_OK;
				}
			}
		}
	}
}

static inline void buzzer_check_semaphore(buzzer_config_t* buzzer_config) {
	if (buzzer_config->semaphore == NULL) {
		buzzer_config->semaphore = xSemaphoreCreateBinary();

		xSemaphoreGive(buzzer_config->semaphore);
	}
}

bool buzzer_init(buzzer_config_t* buzzer_config) {
	bool ret = false;

	buzzer_check_semaphore(buzzer_config);

	if (xSemaphoreTake(buzzer_config->semaphore, pdMS_TO_TICKS(1000)) ==
		pdTRUE) {
		if (buzzer_config->task_handle == NULL) {
			if (buzzer_config->queue == NULL) {
				buzzer_config->queue =
					xQueueCreate(256, sizeof(buzzer_params_t));

				if (buzzer_config->queue == NULL) {
					ret = false;

					LOGE("Fail on create queue!");
					goto unlock_and_exit;
				}
			}

			ret = xTaskCreate(buzzer_task, "buzzer_task", 4096, buzzer_config,
							  10, &buzzer_config->task_handle) == pdPASS;

			if (!ret) {
				LOGE("Fail on create buzzer_task!");
			}

		unlock_and_exit:
			xSemaphoreGive(buzzer_config->semaphore);
		} else {
			ret = true;
		}
	} else {
		LOGE("Fail on take semaphore!");
	}

	return ret;
}

void buzzer_deinit(buzzer_config_t* buzzer_config) {
	buzzer_check_semaphore(buzzer_config);

	if (xSemaphoreTake(buzzer_config->semaphore, pdMS_TO_TICKS(1000)) ==
		pdTRUE) {
		if (buzzer_config->task_handle != NULL) {
			vTaskDelete(buzzer_config->task_handle);

			if (buzzer_config->queue != NULL) {
				vQueueDelete(buzzer_config->queue);

				buzzer_config->queue = NULL;
			}

			if (buzzer_config->beep_semaphore != NULL) {
				vSemaphoreDelete(buzzer_config->beep_semaphore);

				buzzer_config->beep_semaphore = NULL;
			}

			buzzer_config->task_handle = NULL;
		}

		xSemaphoreGive(buzzer_config->semaphore);
	}
}

size_t buzzer_play_melody(buzzer_config_t* buzzer_config,
						  buzzer_melody_t  buzzer_melody) {
	size_t ret = 0;

	for (int thisNote = 0; thisNote < buzzer_melody.size; thisNote++) {
		buzzer_note_t buzzer_note = buzzer_melody.notes[thisNote];

		ret +=
			buzzer_play_note(buzzer_config, buzzer_melody.tempo, buzzer_note);
	}

	return ret;
}

size_t buzzer_play_note(buzzer_config_t* buzzer_config, uint16_t tempo,
						buzzer_note_t buzzer_note) {
	buzzer_params_t buzzer_params = {
		.frequency = buzzer_note.note,
		.duration  = (((60000 * 4) / tempo) / buzzer_note.duration),
	};

	buzzer_play_tone(buzzer_config, buzzer_params);

	return buzzer_params.duration;
}

void buzzer_beep_start(buzzer_config_t* buzzer_config) {
	buzzer_beep(buzzer_config, 0);
}

void buzzer_beep_stop(buzzer_config_t* buzzer_config) {
	buzzer_params_t buzzer_params = {
		.frequency = 0,
		.duration  = 0,
	};

	buzzer_play_tone_now(buzzer_config, buzzer_params);
}

void buzzer_beep(buzzer_config_t* buzzer_config, uint32_t duration) {
	buzzer_params_t buzzer_params = {
		.frequency = buzzer_config->resonant_frequency,
		.duration  = duration,
	};

	buzzer_play_tone_now(buzzer_config, buzzer_params);
}

void buzzer_play_tone_now(buzzer_config_t* buzzer_config,
						  buzzer_params_t  buzzer_params) {
	buzzer_clear_buffer(buzzer_config);
	buzzer_play_tone(buzzer_config, buzzer_params);
}

void buzzer_play_tone(buzzer_config_t* buzzer_config,
					  buzzer_params_t  buzzer_params) {
	// if (buzzer_init(_pin)) {
	if (buzzer_config->queue != NULL) {
		xQueueSend(buzzer_config->queue, &buzzer_params, pdMS_TO_TICKS(1000));
	}
}

void buzzer_clear_buffer(buzzer_config_t* buzzer_config) {
	// if (buzzer_init(_pin)) {
	if (buzzer_config->queue != NULL) {
		xQueueReset(buzzer_config->queue);

		if (buzzer_config->is_plaing) {
			xSemaphoreGive(buzzer_config->beep_semaphore);

			ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
		}
	}
}
