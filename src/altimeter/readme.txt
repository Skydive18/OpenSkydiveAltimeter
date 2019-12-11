!Charset UTF-8

* Настройка Arduino IDE для поддержки Arduino Pro Micro.

Статья на английском языке и все необходимые драйвера и дополнения для Arduino IDE
расположена тут:
https://learn.sparkfun.com/tutorials/pro-micro--fio-v3-hookup-guide/all

После установки дополнений и драйверов нужно выбрать в IDE тип платы Arduino Pro Micro
и версию процессора 3.3v/8MHz


* Оптимизируем UsbCore для Arduino Pro Micro (процессор Atmega32u4)
Высотомер использует контакты D17 и D30 для связи с дисплеем. В Pro Micro эти контакты отвечают
за светодиоды RX и TX. Для нормальной работы требуется поправить код драйвера USBSERIAL.

Расположен он здесь:
[диск]:\Program Files (x86)\Arduino\hardware\arduino\avr\cores\arduino\USBCore.cpp

Из него нужно удалить все упоминания о светодиодах - это строчки, содержащие
TX_RX_LED_INIT
TX_RX_LED_PULSE_MS
TxLEDPulse
RxLEDPulse
RXLED0
RXLED1
TXLED0
TXLED1

Рекомендуется сохранить предыдущую версию файла.


* Устанавливаем библиотеки
Low-Power by Rocket Scream Electronics https://github.com/rocketscream/Low-Power
U8G2 https://github.com/olikraus/u8g2


* Оптимизируем U8G2
Подробности тут: https://github.com/olikraus/u8g2/wiki/u8g2optimization#u8g2-feature-selection

Файл, который нужно поправить, находится тут:
[home]\Documents\Arduino\libraries\U8g2\src\clib\u8g2.h

Комментируем там следующие строки:
#define U8G2_WITH_FONT_ROTATION
#define U8G2_WITH_INTERSECTION
#define U8G2_WITH_CLIP_WINDOW_SUPPORT
