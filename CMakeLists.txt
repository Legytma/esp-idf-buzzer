idf_component_register(
    SRCS "buzzer.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES log_utils
    REQUIRES driver esp_timer
)
