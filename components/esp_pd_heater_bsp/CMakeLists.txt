idf_component_register(
    SRCS "esp_pd_heater_bsp.c" "lvgl_port_knob/lvgl_port_knob.c"
    INCLUDE_DIRS "include" "lvgl_port_knob"
    PRIV_INCLUDE_DIRS "priv_include"
    PRIV_REQUIRES
        "esp_lcd"
    REQUIRES
        "driver"
        "i2c_bus"
)
