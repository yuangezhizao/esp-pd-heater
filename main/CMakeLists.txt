# Handwritten sources: keep explicit directories so newly added files are picked up without globbing.
# (Avoid CONFIGURE_DEPENDS because ESP-IDF evaluates component CMake in script mode.)
set(SRC_DIRS
    "."
    "core/controller"
    "core/events"
    "core/state"
    "services/backlight"
    "services/buzzer"
    "services/chip_temp"
    "services/heater_pwm"
    "services/motion"
    "services/power_meter"
    "services/temp_control"
    "services/usb_pd"
    "ui_app"
    "utils"
    # Generated UI sources (SquareLine/LVGL)
    "ui"
    "ui/components"
    "ui/fonts"
    "ui/screens"
)

idf_component_register(
    SRC_DIRS ${SRC_DIRS}
    PRIV_INCLUDE_DIRS
        "core/controller"
        "core/events"
        "core/state"
        "services"
        "services/backlight"
        "services/buzzer"
        "services/chip_temp"
        "services/heater_pwm"
        "services/motion"
        "services/power_meter"
        "services/temp_control"
        "services/usb_pd"
        "ui_app"
        "utils"
        "ui"
)
