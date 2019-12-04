﻿#ifndef __in_messages_h
#define __in_messages_h

// Messages, english

#define MSG_HELLO PSTR("Ni Hao!")
#define MSG_BYE PSTR("Sayonara")
#define MSG_VERSION PSTR( \
        "Altimonster I\n" \
        "Platform %c%c%c%c\n" \
        "Version 0.90\n" \
        "COM %ld/8N1\n")

#define MSG_JUMP_CONTENT PSTR("%u/%u\n%02d.%02d.%04d %02d:%02d\nE:%4dm D:%4dm\nC:%4dm T:%4dc\nS:%dm/s %dkm/h\nM:%dm/s %dkm/h\n")
#define MSG_MENU "Menu\n"
#define MSG_EXIT " Exit\n"
#define MSG_SET_TO_ZERO "0Reset to zero\n"
#define MSG_BACKLIGHT "BBacklight: %c\n"
#define MSG_LOGBOOK "LLogbook\n"
#define MSG_SETTINGS "SSettings\n"
#define MSG_POWEROFF "XPower OFF\n"

#define MSG_CONFIRM_NO " No\n"
#define MSG_CONFIRM_YES "YYes\n"

#define MSG_LOGBOOK_TITLE "Logbook\n"
#define MSG_LOGBOOK_VIEW "VView\n"
#define MSG_LOGBOOK_REPLAY_JUMP "RReplay jump\n"
#define MSG_LOGBOOK_CLEAR "CClear logbook\n"

#define MSG_LOGBOOK_CLEAR_CONFIRM_TITLE "Clear logbook?\n"

#define MSG_SETTINGS_TITLE "Settings\n"
#define MSG_SETTINGS_SET_TIME "TTime\n"
#define MSG_SETTINGS_SET_DATE "DDate\n"
#define MSG_SETTINGS_SET_ALARM "AAlarm %s\n"
#define MSG_SETTINGS_SET_MODE "QAutomatics: %c\n"
#define MSG_SETTINGS_SET_AUTO_POWER_OFF "OAuto off: %dч\n"
#define MSG_SETTINGS_SET_SCREEN_ROTATION "RRotate screen\n"
#define MSG_SETTINGS_SET_SCREEN_CONTRAST "CContrast %d\n"
#define MSG_SETTINGS_SET_AUTO_ZERO "0Auto set 0: %c\n"
#define MSG_SETTINGS_ABOUT "VAbout\n"
#define MSG_SETTINGS_PC_LINK "UPC link\n"

#define MSG_SETTINGS_SET_TIME_TITLE "Current time"

#define MSG_SETTINGS_SET_ALARM_TITLE "Alarm\n"
#define MSG_SETTINGS_SET_ALARM_TOGGLE "SArmed: %c\n"
#define MSG_SETTINGS_SET_ALARM_TIME "TTime %02d:%02d\n"

#define MSG_SETDATE_TITLE "Current date"
#define MSG_SETDATETIME_CANCEL "%cCancel"
#define MSG_SETDATETIME_OK "%cSet"

#endif