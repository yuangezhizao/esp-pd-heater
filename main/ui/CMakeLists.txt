SET(SOURCES screens/ui_Screen1.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    fonts/ui_font_NovaMono13.c
    fonts/ui_font_NovaMono22.c
    fonts/ui_font_silver12.c
    fonts/ui_font_SmileySans15.c
    fonts/ui_font_UbuntuMono24.c
    fonts/ui_font_UbuntuMono60.c)

add_library(ui ${SOURCES})
