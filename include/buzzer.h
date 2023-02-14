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
 * buzzer.h
 *
 *  Created on: 01 de fev de 2023
 *      Author: Alex Manoel Ferreira Silva
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "driver/ledc.h"
#include "sdkconfig.h"

#define _NO   0
#define _BEEP CONFIG_BUZZER_RESSONANCE_FREQUENCY

#define _C0  16
#define _CS0 17
#define _D0  18
#define _DS0 19
#define _E0  21
#define _F0  22
#define _FS0 23
#define _G0  24
#define _GS0 26
#define _A0  28
#define _AS0 29
#define _B0  31

#define _C1  33
#define _CS1 35
#define _D1  37
#define _DS1 39
#define _E1  41
#define _F1  44
#define _FS1 46
#define _G1  49
#define _GS1 52
#define _A1  55
#define _AS1 58
#define _B1  62

#define _C2  65
#define _CS2 69
#define _D2  73
#define _DS2 78
#define _E2  82
#define _F2  87
#define _FS2 92
#define _G2  98
#define _GS2 104
#define _A2  110
#define _AS2 117
#define _B2  123

#define _C3  131
#define _CS3 139
#define _D3  147
#define _DS3 156
#define _E3  165
#define _F3  175
#define _FS3 185
#define _G3  196
#define _GS3 208
#define _A3  220
#define _AS3 233
#define _B3  247

#define _C4  262
#define _CS4 277
#define _D4  294
#define _DS4 311
#define _E4  330
#define _F4  349
#define _FS4 370
#define _G4  392
#define _GS4 415
#define _A4  440
#define _AS4 466
#define _B4  494

#define _C5  523
#define _CS5 554
#define _D5  587
#define _DS5 622
#define _E5  659
#define _F5  698
#define _FS5 740
#define _G5  784
#define _GS5 831
#define _A5  880
#define _AS5 932
#define _B5  988

#define _C6  1047
#define _CS6 1109
#define _D6  1175
#define _DS6 1245
#define _E6  1319
#define _F6  1397
#define _FS6 1480
#define _G6  1568
#define _GS6 1661
#define _A6  1760
#define _AS6 1865
#define _B6  1976

#define _C7  2093
#define _CS7 2217
#define _D7  2349
#define _DS7 2489
#define _E7  2637
#define _F7  2794
#define _FS7 2960
#define _G7  3136
#define _GS7 3322
#define _A7  3520
#define _AS7 3729
#define _B7  3951

#define _C8  4186
#define _CS8 4435
#define _D8  4699
#define _DS8 4978
#define _E8  5274
#define _F8  5588
#define _FS8 5920
#define _G8  6272
#define _GS8 6645
#define _A8  7040
#define _AS8 7459
#define _B8  7902

#define _C9  8372
#define _CS9 8870
#define _D9  9397
#define _DS9 9956
#define _E9  10548
#define _F9  11175
#define _FS9 11840
#define _G9  12544
#define _GS9 13290
#define _A9  14080
#define _AS9 14917
#define _B9  15804

#define _C10  16744
#define _CS10 17740
#define _D10  18795
#define _DS10 19912
#define _E10  21096
#define _F10  22351
#define _FS10 23680
#define _G10  25088
#define _GS10 26580
#define _A10  28160
#define _AS10 29834
#define _B10  31609

#define BUZZER_CONFIG_DEFAULT()                                   \
	{                                                             \
		.ledc_channel_config =                                    \
			{                                                     \
				.speed_mode = LEDC_LOW_SPEED_MODE,                \
				.channel    = LEDC_CHANNEL_0,                     \
				.timer_sel  = LEDC_TIMER_0,                       \
				.intr_type  = LEDC_INTR_DISABLE,                  \
				.gpio_num   = CONFIG_BUZZER_GPIO,                 \
				.duty       = 4096,                               \
				.hpoint     = 0,                                  \
			},                                                    \
		.resonant_frequency = CONFIG_BUZZER_RESSONANCE_FREQUENCY, \
		.task_handle = NULL, .queue = NULL, .semaphore = NULL,    \
		.beep_semaphore = NULL, .is_plaing = false,               \
	}

typedef struct buzzer_params_s {
	uint32_t frequency;
	uint32_t duration;
} buzzer_params_t;

typedef struct buzzer_note_s {
	uint16_t note;
	uint8_t  duration;
} buzzer_note_t;

typedef struct buzzer_melody_s {
	buzzer_note_t *notes;
	size_t         size;
	uint16_t       tempo;
} buzzer_melody_t;

typedef struct buzzer_config_s {
	ledc_channel_config_t ledc_channel_config;
	uint32_t              resonant_frequency;
	TaskHandle_t          task_handle;
	QueueHandle_t         queue;
	SemaphoreHandle_t     semaphore;
	SemaphoreHandle_t     beep_semaphore;
	bool                  is_plaing;
} buzzer_config_t;

bool   buzzer_init(buzzer_config_t *buzzer_config);
void   buzzer_deinit(buzzer_config_t *buzzer_config);
size_t buzzer_play_melody(buzzer_config_t *buzzer_config,
						  buzzer_melody_t  buzzer_melody);
size_t buzzer_play_note(buzzer_config_t *buzzer_config, uint16_t tempo,
						buzzer_note_t buzzer_note);
void   buzzer_beep_start(buzzer_config_t *buzzer_config);
void   buzzer_beep_stop(buzzer_config_t *buzzer_config);
void   buzzer_beep(buzzer_config_t *buzzer_config, uint32_t duration);
void   buzzer_play_tone_now(buzzer_config_t *buzzer_config,
							buzzer_params_t  buzzer_params);
void   buzzer_play_tone(buzzer_config_t *buzzer_config,
						buzzer_params_t  buzzer_params);
void   buzzer_clear_buffer(buzzer_config_t *buzzer_config);
