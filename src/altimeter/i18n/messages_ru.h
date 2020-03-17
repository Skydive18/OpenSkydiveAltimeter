#ifndef __in_messages_h
#define __in_messages_h

// Messages, russian

#define LANGUAGE "RU"

#define MSG_HELLO PSTR("Ossu!")
#define MSG_BYE PSTR("Sayonara")
#define MSG_VERSION PSTR( \
        "Pegasus I\n" \
        "Платформа %c%c%c%c\n" \
        "Версия " \
        VERSION \
        "\n")

#define MSG_JUMP_CONTENT PSTR("%u/%u %c\n%02d.%02d.%04d %02d:%02d\nO:%4dм Р:%4dм\nП:%4dм В:%4dc\nС:%dм/с %dкм/ч\nМ:%dм/с %dкм/ч\n")
#define MSG_MENU "Меню\n"
#define MSG_EXIT " Выход\n"
#define MSG_SET_TO_ZERO "0Сброс на ноль\n"
#define MSG_PROFILE "PПрофиль: %c%c\n"
#define MSG_BACKLIGHT "BПодсветка: %c\n"
#define MSG_LOGBOOK "LЖурнал прыжков\n"
#define MSG_SETTINGS "SНастройки\n"
#define MSG_POWEROFF "XВыключение\n"

#define MSG_CONFIRM_NO " Нет\n"
#define MSG_CONFIRM_YES "YДа\n"

#define MSG_LOGBOOK_TITLE "Журнал прыжков\n"
#define MSG_LOGBOOK_VIEW "VПросмотр\n"
#define MSG_LOGBOOK_REPLAY_JUMP "RТест всего\n"
#define MSG_LOGBOOK_CLEAR "CОчистить журнал\n"
#define MSG_LOGBOOK_DELETE_LAST_JUML "DУдалить крайний\n"

#define MSG_LOGBOOK_CLEAR_CONFIRM_TITLE "Очистить журнал?\n"
#define MSG_LOGBOOK_DELETE_LAST_JUMP_CONFIRM_TITLE "Удалить крайний?\n"

#define MSG_SETTINGS_TITLE "Настройки\n"
#define MSG_SETTINGS_SET_TIME "TВремя\n"
#define MSG_SETTINGS_SET_DATE "DДата\n"
#define MSG_SETTINGS_SET_ALARM "AБудильник %s\n"
#define MSG_SETTINGS_SET_SIGNALS "SСигналы\n"
#define MSG_SETTINGS_SET_SIGNALS_TEST "tТест сигналов\n"
#define MSG_SETTINGS_LOUDNESS "lГромкость: %d\n"
#define MSG_SETTINGS_SET_SIGNALS_LED "LСветодиод: %c\n"
#define MSG_SETTINGS_SET_SIGNALS_BUZZER "BПищалка: %c\n"
#define MSG_SETTINGS_SET_MODE "QАвтоматика: %c\n"
#define MSG_SETTINGS_SET_AUTO_POWER_OFF "OАвтовыкл: %dч\n"
#define MSG_SETTINGS_SET_SCREEN_ROTATION "RПоворот экрана\n"
#define MSG_SETTINGS_SET_SCREEN_CONTRAST "CКонтраст %d\n"
#define MSG_SETTINGS_SET_AUTO_ZERO "0Авто ноль: %c\n"
#define MSG_SETTINGS_ABOUT "VВерсия ПО\n"
#define MSG_SETTINGS_PC_LINK "UДамп памяти\n"

#define MSG_SETTINGS_SET_TIME_TITLE "Текущее время"

#define MSG_SETTINGS_SET_ALARM_TITLE "Будильник\n"
#define MSG_SETTINGS_SET_ALARM_TOGGLE "SВключен: %c\n"
#define MSG_SETTINGS_SET_ALARM_TIME "TВремя %02d:%02d\n"

#define MSG_SETDATE_TITLE "Текущая дата"
#define MSG_SETDATETIME_CANCEL "%cОтмена"
#define MSG_SETDATETIME_OK "%cОК"

#endif
