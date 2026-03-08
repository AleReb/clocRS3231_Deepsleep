# ESP32 DS3231/DS3232 Alarm & Deep Sleep Controller

Este proyecto demuestra cómo utilizar un microcontrolador ESP32 (específicamente compatible con la serie ESP32-C3 y otras que soportan Deep Sleep en pines RTC) junto con un módulo de reloj en tiempo real (RTC) DS3231 o DS3232.

El sistema administra el modo de reposo profundo (Deep Sleep) para minimizar el consumo de energía y se despierta mediante interrupciones generadas por las alarmas del RTC. Además, incluye la funcionalidad de medición del voltaje de la batería mediante el ADC.

## Características Principales

- **Gestión de Deep Sleep**: El ESP32 entra en modo de bajo consumo y es despertado por el RTC a través de un pin externo.
- **Doble Alarma Configurable**: Soporte para configurar dos alarmas de forma independiente (Alarm 1 y Alarm 2) mediante variables booleanas.
- **Medición de Batería**: Lectura del voltaje de la batería conectada al pin ADC 2 mediante calibración (`esp_adc_cal`).
- **Diagnóstico por Puerto Serial**: Al bootear u ocurrir un reinicio (Wakeup), el sistema muestra la causa del reinicio, el estado del RTC, valores de las alarmas configuradas y el voltaje de la batería antes de volver a dormir.

## Conexiones de Hardware

- **RTC DS3231/DS3232**: Conexión a través del bus I2C lógico (SDA, SCL).
- **Pin de Interrupción (SQW/INT)**: Conectado al pin `GPIO 5` (Debe ser un pin RTC válido en ESP32-C3 para despertar del Deep Sleep).
- **Medición de Batería**: Conectado al pin `GPIO 2` (ADC).
- **LED/Status (Opcional)**: Conectado al pin `GPIO 3`.

Para instrucciones detalladas de configuración y personalización, por favor consulta el [MANUAL.md](./MANUAL.md).

## Licencias y Responsabilidades

- **Licencia**: Este proyecto está licenciado bajo la [Creative Commons Attribution 4.0 International (CC BY 4.0)](./LICENSE).
- **Descargo de Responsabilidad**: Por favor, lee el [DISCLAIMER.md](./DISCLAIMER.md) antes de utilizar o adaptar este proyecto para cualquier fin.
