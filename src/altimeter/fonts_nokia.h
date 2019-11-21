#ifndef __in_fonts_h
#define __in_fonts_h

#define font_hello u8g2_font_helvB10_tr

const uint8_t font_status_line[193] U8G2_FONT_SECTION("font_status_line") = 
  "\30\0\2\3\2\3\3\4\4\3\5\0\0\5\377\5\377\0\0\0\0\0\244 \5\200\315\0!\7\227\310"
  "\245T\4\42\7\227\310\245\244\3#\7\227\310\245\216\1$\7\227\310EG\1%\10\227\310\304P\66\24"
  "&\7\227\310F\225\30'\10\227\310\204R\25\2*\10\227\310D\251)\3-\5\207\312\14.\5\245\310"
  "\4/\10\227\310\246\222!\0\60\7\227\310P*\2\61\5\325\310\24\62\7\227\310\214\345\0\63\6\227\310"
  "\214\14\64\10\227\310D\22\231\0\65\7\227\310\220#\1\66\7\227\310\220\24\1\67\6\227\310\214\71\70\7"
  "\227\310P\25\1\71\7\227\310P$\1:\6\261\310\204\1@\7\227\310L\21\3\0\0\0\4\377\377\0"
  "";


//#define font_altitude u8g2_font_logisoso28_tn
const uint8_t font_altitude[244] U8G2_FONT_SECTION("font_altitude") = 
  "\14\0\4\6\5\5\6\5\6\24\34\0\0\34\0\0\0\0\0\0\0\0\327 \6\0\200\220\6-\13\216"
  "\214\234\16Fp\202\21\0\60\20\221\217\220\216G\334\22\371\377_\336\342\21\0\61\15\204\303\220\16A\374"
  " \25\202\0\0\62\30\221\217\220\216G\210\2\322\304_\374`\20\204&~\15\11Q<\2\0\63\25\221"
  "\217\220\216G\210\2\322\304_\310\2\322\304_\310\342\21\0\64\25\221\217\220\16\301\12\202\220\310\177y\13"
  "H\23\377sA\0\0\65\27\221\217\220\216G\310\202\320\304\257!!\12H\23\177!\213G\0\0\66\25"
  "\221\217\220\216G\310\202\320\304\257!qK\344\227\267x\4\0\67\21\221\217\220\216G\210\2\322\304\377\377"
  "\317\5\1\0\70\24\221\217\220\216G\334\22\371\345\17\32\211\374\362\26\217\0\0\71\24\221\217\220\216G\334"
  "\22\371\345- M\374\205,\36\1\0\0\0\0\4\377\377\0";


#define MENU_FONT_HEIGHT 7
//#define font_menu u8g2_font_5x8_t_cyrillic
const uint8_t font_menu[1538] U8G2_FONT_SECTION("font_menu") = 
  "\237\0\2\2\3\4\3\4\4\5\10\0\377\6\377\6\0\1%\2L\3X \5\0~\3!\7\61c"
  "\63R\0\42\7\233n\223\254\0#\15=bW\246\64T\65T\231\22\0$\15=b\233\301\252\301\6"
  ")m\20\1%\10\253f\23Sg\0&\12<b\27S\263j\246\0'\5\31o\63(\7\262b\247"
  "\232\1)\10\262b\23S\245\0*\12,b\23\223\32I\305\0+\12-b\233Q\34\62\243\10,\7"
  "\233^\247J\0-\6\14j\63\2.\6\222b\63\2/\12\64b\17\62\210m\220\1\60\10\263bW"
  "\271*\0\61\7\263b\227dk\62\13\64b\247b\6Ie\60\2\63\12\64b\63b\324H&\5\64"
  "\12\64b\33U\65b\6\11\65\12\64b\63\64\330H&\5\66\12\64b\247\62XQ&\5\67\14\64"
  "b\63\62\210\31\304\14\42\0\70\12\64b\247bRQ&\5\71\12\64b\247\242L\33$\5:\7\252"
  "b\63\342\10;\10\263^g#U\2<\11\263b\233\312\14\62\10=\10\34f\63\62\32\1>\12\263"
  "b\223A\6\61\225\0\77\11\263b\327L\31&\0@\14E^+\243\134I%YC\5A\11\64b"
  "\247\242\34S\6B\12\64b\263\342HQ\216\4C\13\64b\247\242\6\31\304\244\0D\11\64b\263\242"
  "s$\0E\13\64b\63\64X\31d\60\2F\13\64b\63\64X\31d\220\1G\12\64b\247\242\6"
  "i&\5H\11\64b\23\345\230f\0I\7\263b\263bkJ\12\64b\67\63\310 \225\21K\11\64"
  "b\23U\222\251\63L\14\64b\223A\6\31d\220\301\10M\11\64b\23\307\21\315\0N\11\64b\23"
  "\327Xg\0O\11\64b\247\242\63)\0P\12\64b\263\242\34)\203\14Q\11<^\247\242\134n\24"
  "R\12\64b\263\242\34)\312\0S\12\64b\247b\312\250L\12T\10\263b\263b\27\0U\10\64b"
  "\23=\223\2V\11\64b\23\235I*\0W\11\64b\23\315q\304\0X\12\64b\23e\222*\312\0"
  "Y\15\65b\223\201\6\251\6\31e\24\1Z\12\64b\63\62\210m\60\2[\7\263b\63bs\134\14"
  "\64b\223AF\31e\224A\0]\7\263b\63\233#^\6\223r\327\0_\6\14^\63\2`\6\222"
  "r\23\3a\10$b\67\242L\3b\13\64b\223A\6+\312\221\0c\7\243b\67\63\20d\12\64"
  "b\17\62H#\312\64e\11$b\247\322\310@\1f\11\64b[\225\63\203\10g\12,^\247b\332"
  " )\0h\12\64b\223A\6+\232\1i\10\263b\227\221\254\6j\11\273^\233a\251*\0k\13"
  "\64b\223A\6q\244(\3l\7\263b#\273\6m\11%b\243Z*\251\2n\7$b\263\242\31"
  "o\10$b\247\242L\12p\12,^\263\342H\31d\0q\12,^\67b\332 \203\0r\11$b"
  "\223\222\15\62\0s\10\243b\67\62X\0t\13\64b\227A\234\31\244\230\0u\7$b\23\315\64v"
  "\7\243b\223\254\12w\11%b\223\201J\252\13x\10$b\23\223T\61y\12,^\23e\32\61)"
  "\0z\10$b\63b\71\2{\12,f\33\325H\32$\0|\5\61cs}\12,f\227\201\32I"
  "F\0~\13<b\17\62\210\251\63\203\10\0\0\0\4\377\377\4\20\12\64b\247\242\34S\6\4\21\13"
  "\64b\263\62XQ\216\4\4\22\13\64b\263\342HQ\216\4\4\23\14\64b\63\244\6\31d\220\1\4"
  "\24\12<^\247\372k\310\0\4\25\14\64b\63\64X\31d\60\2\4\26\14\65b\223\222\252V\245\222"
  "*\4\27\13\64b\247b\215dR\0\4\30\12\64b\23\255\221\244\14\4\31\14<b\23\223\212j$"
  ")\3\4\32\13\64b\23U\222\251\224\1\4\33\11\64b\253\372%\3\4\34\12\64b\23\307\21\315\0"
  "\4\35\12\64b\23\345\230f\0\4\36\12\64b\247\242\63)\0\4\37\11\64b\63\244g\0\4 \13"
  "\64b\263\242\34)\203\14\4!\14\64b\247\242\6\31\304\244\0\4\42\11\263b\263b\27\0\4#\13"
  "\64b\23\315\264\301H\0\4$\14\65b\233\301\252T\265A\4\4%\12\263b\223T\231\222\12\4&"
  "\12<^\223\372\327\310 \4'\11\263b\223\254$\13\4(\15\65b\223\222J*\251\244\322\30\4)"
  "\16=^\223\222J*\251\244\322\330(\4*\15\65b\263QF\32\244\230\242\2\4+\15\65b\223\201"
  "\6S%\225f\0\4,\12\263b\23\243Jj\1\4-\14\64b\263Q\332 \203\221\0\4.\15\65"
  "b\23S\65\222J*\311\4\4/\13\64b\67\242L#\312\0\4\60\11$b\67\242L\3\4\61\13"
  "\64b\247\62XQ&\5\4\62\11$b\263V\34\11\4\63\10\243b\63b\11\4\64\11,^\267\222"
  "\34\63\4\65\12$b\247\322\310@\1\4\66\12%b\223\252U\251\2\4\67\10\243b\263\344\2\4\70"
  "\11$b\23\325H\62\4\71\13\64b\23\223\212j$\31\4:\11$b\23Gj\6\4;\10$b"
  "\253Z\62\4<\12%b\223\301ZI\25\4=\11$b\23\307\224\1\4>\11$b\247\242L\12\4"
  "\77\10$b\63\244\31\4@\12,^\263\242\34)\3\4A\10\243b\67\63\20\4B\10\243b\263b"
  "\5\4C\12,^\23e*K\0\4D\13-^\233\301jm\20\1\4E\11$b\23\223T\61\4"
  "F\12,^\23\315\241A\0\4G\11$b\23e\332 \4H\13%b\223\222J*\215\1\4I\14"
  "-^\223\222J*\215\215\2\4J\12$b\243\201LI\1\4K\11$b\23\227\32)\4L\12$"
  "b\223\301\212#\1\4M\10\243b#\345\2\4N\12$b\223\222Z*\1\4O\10\243b\267\322\12"
  "\0";

#endif
