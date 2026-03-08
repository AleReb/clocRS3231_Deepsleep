# Manual de Usuario y Configuración

Este manual describe exhaustivamente cómo configurar y personalizar el firmware del controlador de alarmas ESP32 con RTC DS3231/DS3232.

## 1. Configuración de las Alarmas

El firmware permite configurar dos alarmas independientes del RTC. La configuración de las mismas se encuentra en la sección superior del archivo principal (`clocRS3231_test.ino`), en las variables globales.

### Alarma 1 (Soporta Horas, Minutos y Segundos)
```cpp
bool enableAlarm1 = true; // true para activar, false para desactivar
uint8_t alarm1Hour = 18;  // Hora (formato 24h)
uint8_t alarm1Minute = 7; // Minuto
uint8_t alarm1Second = 0; // Segundo
```

### Alarma 2 (Soporta Horas y Minutos)
```cpp
bool enableAlarm2 = true; // true para activar, false para desactivar
uint8_t alarm2Hour = 18;  // Hora (formato 24h)
uint8_t alarm2Minute = 8; // Minuto
```

Si la bandera `enableAlarm1` o `enableAlarm2` se configura como `false`, esa alarma específica no se programará en el RTC y no causará interrupciones, aunque las variables de tiempo (hora, minuto) estén definidas en código.

## 2. Medida de Batería (ADC)

El sistema lee de manera automática el voltaje de la batería cada vez que sale del "Deep Sleep" o se bootea por primera vez.
- **Pin ADC**: `BAT_ADC 2`.
- El valor leído por el pin ADC (Atenuación de 11dB, 12-bits, Vref ~1100mV) se convierte mediante `esp_adc_cal`.
- El resultado se imprime por puerto Serial para brindar un diagnóstico integral sin interrumpir el funcionamiento continuo.

## 3. Pin de Interrupción (Wake-up)

**IMPORTANTE para ESP32-C3**: El despertar desde Deep Sleep utilizando un pin externo solo funciona si ese pin tiene capacidades RTC (generalmente GPIO0 a GPIO5).
Si modificas la placa base, debes comprobar qué pines están conectados al dominio del RTC.
```cpp
static const int RTC_INT_PIN = 5; // Cambiar según sea necesario
```

## 4. Flujo de Ejecución del Sistema

1. **Arranque (Boot / Wakeup)**: El ESP32 se enciende por un reinicio de hardware, reloj interno o un cambio en el `RTC_INT_PIN` a LOW.
2. **Reporte de Batería**: Se lee el pin ADC configurado, se formatea y se emite el informe de la tensión actual de la batería.
3. **Causa del Reinicio**: Identificación en consola (`Power-on`, `Timer` o `GPIO`).
4. **Validación del RTC / Borrado de Banderas**: Si el GPIO de interrupción fue disparado anteriormente porque la hora coincidía con una Alarma, el sistema borra esas banderas dentro del DS3231.
5. **Cálculo y Reprogramación**: Se reprograman las alarmas activas (`enableAlarmX == true`) calculando su próxima ocurrencia válida (hoy, o al día siguiente si la hora ya pasó).
6. **Reposo Profundo (Deep Sleep)**: El microcontrolador desactiva los periféricos principales y su CPU. El consumo basal drásticamente disminuye y el DS3231 continúa en ejecución gracias a su bateria tipo moneda. Cuando ocurra una Alarma el SQW caerá a `LOW` y repetirá el proceso.

## 5. Monitoreo y Diagnóstico

Para visualizar los mensajes y entender el estado del dispositivo, inicia tu entorno de desarrollo, y abre el **Monitor Serial** (IDE) a **115200 bps**.
Un arranque normal producirá:
```text
Boot start
Battery Voltage measure: 4.15V
Wakeup cause: Power-on or reset
RTC detected
Configured Alarm 1 for: ...
Alarm 1 configured successfully
Configured Alarm 2 for: ...
Alarm 2 configured successfully
Setup complete
Entering deep sleep
```
