# Project Rules

## Configuration Rules
1. **Default Configurations**: ALL default configurations MUST be declared in `include/config.h` (TUTTE le configurazioni predefinite vanno sempre in `config.h`).
2. **User Variable Configurations**: User-configurable settings that can change at runtime MUST be saved to and loaded from `config.cfg` (stored on LittleFS/SPIFFS/SD card).
3. **Parameterization**: Everything MUST be fully parameterized; no hardcoded values in logic or UI code.

## Workflow Rules
4. **Auto-Upload**: Upon creating or modifying code, automatically build and upload the firmware to the ESP32 board using `pio run --target upload` without waiting for manual upload instructions.

