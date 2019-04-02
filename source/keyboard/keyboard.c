#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "keyboard.h"


#define KEYB_TOKENISING_CHAR ' '

#ifndef SDL_SCANCODE_A
/**
 *  \brief The SDL keyboard scancode representation.
 *
 *  Values of this type are used to represent keyboard keys, among other places
 *  in the \link SDL_Keysym::scancode key.keysym.scancode \endlink field of the
 *  SDL_Event structure.
 *
 *  The values in this enumeration are based on the USB usage page standard:
 *  https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
 */
typedef enum
{
	SDL_SCANCODE_UNKNOWN = 0,

	/**
	 *  \name Usage page 0x07
	 *
	 *  These values are from usage page 0x07 (USB keyboard page).
	 */
	/* @{ */

	SDL_SCANCODE_A = 4,
	SDL_SCANCODE_B = 5,
	SDL_SCANCODE_C = 6,
	SDL_SCANCODE_D = 7,
	SDL_SCANCODE_E = 8,
	SDL_SCANCODE_F = 9,
	SDL_SCANCODE_G = 10,
	SDL_SCANCODE_H = 11,
	SDL_SCANCODE_I = 12,
	SDL_SCANCODE_J = 13,
	SDL_SCANCODE_K = 14,
	SDL_SCANCODE_L = 15,
	SDL_SCANCODE_M = 16,
	SDL_SCANCODE_N = 17,
	SDL_SCANCODE_O = 18,
	SDL_SCANCODE_P = 19,
	SDL_SCANCODE_Q = 20,
	SDL_SCANCODE_R = 21,
	SDL_SCANCODE_S = 22,
	SDL_SCANCODE_T = 23,
	SDL_SCANCODE_U = 24,
	SDL_SCANCODE_V = 25,
	SDL_SCANCODE_W = 26,
	SDL_SCANCODE_X = 27,
	SDL_SCANCODE_Y = 28,
	SDL_SCANCODE_Z = 29,

	SDL_SCANCODE_1 = 30,
	SDL_SCANCODE_2 = 31,
	SDL_SCANCODE_3 = 32,
	SDL_SCANCODE_4 = 33,
	SDL_SCANCODE_5 = 34,
	SDL_SCANCODE_6 = 35,
	SDL_SCANCODE_7 = 36,
	SDL_SCANCODE_8 = 37,
	SDL_SCANCODE_9 = 38,
	SDL_SCANCODE_0 = 39,

	SDL_SCANCODE_RETURN = 40,
	SDL_SCANCODE_ESCAPE = 41,
	SDL_SCANCODE_BACKSPACE = 42,
	SDL_SCANCODE_TAB = 43,
	SDL_SCANCODE_SPACE = 44,

	SDL_SCANCODE_MINUS = 45,
	SDL_SCANCODE_EQUALS = 46,
	SDL_SCANCODE_LEFTBRACKET = 47,
	SDL_SCANCODE_RIGHTBRACKET = 48,
	SDL_SCANCODE_BACKSLASH = 49, /**< Located at the lower left of the return
											*   key on ISO keyboards and at the right end
											*   of the QWERTY row on ANSI keyboards.
											*   Produces REVERSE SOLIDUS (backslash) and
											*   VERTICAL LINE in a US layout, REVERSE
											*   SOLIDUS and VERTICAL LINE in a UK Mac
											*   layout, NUMBER SIGN and TILDE in a UK
											*   Windows layout, DOLLAR SIGN and POUND SIGN
											*   in a Swiss German layout, NUMBER SIGN and
											*   APOSTROPHE in a German layout, GRAVE
											*   ACCENT and POUND SIGN in a French Mac
											*   layout, and ASTERISK and MICRO SIGN in a
											*   French Windows layout.
											*/
	SDL_SCANCODE_NONUSHASH = 50, /**< ISO USB keyboards actually use this code
											*   instead of 49 for the same key, but all
											*   OSes I've seen treat the two codes
											*   identically. So, as an implementor, unless
											*   your keyboard generates both of those
											*   codes and your OS treats them differently,
											*   you should generate SDL_SCANCODE_BACKSLASH
											*   instead of this code. As a user, you
											*   should not rely on this code because SDL
											*   will never generate it with most (all?)
											*   keyboards.
											*/
	SDL_SCANCODE_SEMICOLON = 51,
	SDL_SCANCODE_APOSTROPHE = 52,
	SDL_SCANCODE_GRAVE = 53, /**< Located in the top left corner (on both ANSI
									  *   and ISO keyboards). Produces GRAVE ACCENT and
									  *   TILDE in a US Windows layout and in US and UK
									  *   Mac layouts on ANSI keyboards, GRAVE ACCENT
									  *   and NOT SIGN in a UK Windows layout, SECTION
									  *   SIGN and PLUS-MINUS SIGN in US and UK Mac
									  *   layouts on ISO keyboards, SECTION SIGN and
									  *   DEGREE SIGN in a Swiss German layout (Mac:
									  *   only on ISO keyboards), CIRCUMFLEX ACCENT and
									  *   DEGREE SIGN in a German layout (Mac: only on
									  *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
									  *   French Windows layout, COMMERCIAL AT and
									  *   NUMBER SIGN in a French Mac layout on ISO
									  *   keyboards, and LESS-THAN SIGN and GREATER-THAN
									  *   SIGN in a Swiss German, German, or French Mac
									  *   layout on ANSI keyboards.
									  */
	SDL_SCANCODE_COMMA = 54,
	SDL_SCANCODE_PERIOD = 55,
	SDL_SCANCODE_SLASH = 56,

	SDL_SCANCODE_CAPSLOCK = 57,

	SDL_SCANCODE_F1 = 58,
	SDL_SCANCODE_F2 = 59,
	SDL_SCANCODE_F3 = 60,
	SDL_SCANCODE_F4 = 61,
	SDL_SCANCODE_F5 = 62,
	SDL_SCANCODE_F6 = 63,
	SDL_SCANCODE_F7 = 64,
	SDL_SCANCODE_F8 = 65,
	SDL_SCANCODE_F9 = 66,
	SDL_SCANCODE_F10 = 67,
	SDL_SCANCODE_F11 = 68,
	SDL_SCANCODE_F12 = 69,

	SDL_SCANCODE_PRINTSCREEN = 70,
	SDL_SCANCODE_SCROLLLOCK = 71,
	SDL_SCANCODE_PAUSE = 72,
	SDL_SCANCODE_INSERT = 73, /**< insert on PC, help on some Mac keyboards (but
										 does send code 73, not 117) */
	SDL_SCANCODE_HOME = 74,
	SDL_SCANCODE_PAGEUP = 75,
	SDL_SCANCODE_DELETE = 76,
	SDL_SCANCODE_END = 77,
	SDL_SCANCODE_PAGEDOWN = 78,
	SDL_SCANCODE_RIGHT = 79,
	SDL_SCANCODE_LEFT = 80,
	SDL_SCANCODE_DOWN = 81,
	SDL_SCANCODE_UP = 82,

	SDL_SCANCODE_NUMLOCKCLEAR = 83, /**< num lock on PC, clear on Mac keyboards
	*/
	SDL_SCANCODE_KP_DIVIDE = 84,
	SDL_SCANCODE_KP_MULTIPLY = 85,
	SDL_SCANCODE_KP_MINUS = 86,
	SDL_SCANCODE_KP_PLUS = 87,
	SDL_SCANCODE_KP_ENTER = 88,
	SDL_SCANCODE_KP_1 = 89,
	SDL_SCANCODE_KP_2 = 90,
	SDL_SCANCODE_KP_3 = 91,
	SDL_SCANCODE_KP_4 = 92,
	SDL_SCANCODE_KP_5 = 93,
	SDL_SCANCODE_KP_6 = 94,
	SDL_SCANCODE_KP_7 = 95,
	SDL_SCANCODE_KP_8 = 96,
	SDL_SCANCODE_KP_9 = 97,
	SDL_SCANCODE_KP_0 = 98,
	SDL_SCANCODE_KP_PERIOD = 99,

	SDL_SCANCODE_NONUSBACKSLASH = 100, /**< This is the additional key that ISO
													*   keyboards have over ANSI ones,
													*   located between left shift and Y.
													*   Produces GRAVE ACCENT and TILDE in a
													*   US or UK Mac layout, REVERSE SOLIDUS
													*   (backslash) and VERTICAL LINE in a
													*   US or UK Windows layout, and
													*   LESS-THAN SIGN and GREATER-THAN SIGN
													*   in a Swiss German, German, or French
													*   layout. */
	SDL_SCANCODE_APPLICATION = 101, /**< windows contextual menu, compose */
	SDL_SCANCODE_POWER = 102, /**< The USB document says this is a status flag,
										*   not a physical key - but some Mac keyboards
										*   do have a power key. */
	SDL_SCANCODE_KP_EQUALS = 103,
	SDL_SCANCODE_F13 = 104,
	SDL_SCANCODE_F14 = 105,
	SDL_SCANCODE_F15 = 106,
	SDL_SCANCODE_F16 = 107,
	SDL_SCANCODE_F17 = 108,
	SDL_SCANCODE_F18 = 109,
	SDL_SCANCODE_F19 = 110,
	SDL_SCANCODE_F20 = 111,
	SDL_SCANCODE_F21 = 112,
	SDL_SCANCODE_F22 = 113,
	SDL_SCANCODE_F23 = 114,
	SDL_SCANCODE_F24 = 115,
	SDL_SCANCODE_EXECUTE = 116,
	SDL_SCANCODE_HELP = 117,
	SDL_SCANCODE_MENU = 118,
	SDL_SCANCODE_SELECT = 119,
	SDL_SCANCODE_STOP = 120,
	SDL_SCANCODE_AGAIN = 121,   /**< redo */
	SDL_SCANCODE_UNDO = 122,
	SDL_SCANCODE_CUT = 123,
	SDL_SCANCODE_COPY = 124,
	SDL_SCANCODE_PASTE = 125,
	SDL_SCANCODE_FIND = 126,
	SDL_SCANCODE_MUTE = 127,
	SDL_SCANCODE_VOLUMEUP = 128,
	SDL_SCANCODE_VOLUMEDOWN = 129,
	/* not sure whether there's a reason to enable these */
	/*     SDL_SCANCODE_LOCKINGCAPSLOCK = 130,  */
	/*     SDL_SCANCODE_LOCKINGNUMLOCK = 131, */
	/*     SDL_SCANCODE_LOCKINGSCROLLLOCK = 132, */
	SDL_SCANCODE_KP_COMMA = 133,
	SDL_SCANCODE_KP_EQUALSAS400 = 134,

	SDL_SCANCODE_INTERNATIONAL1 = 135, /**< used on Asian keyboards, see
													 footnotes in USB doc */
	SDL_SCANCODE_INTERNATIONAL2 = 136,
	SDL_SCANCODE_INTERNATIONAL3 = 137, /**< Yen */
	SDL_SCANCODE_INTERNATIONAL4 = 138,
	SDL_SCANCODE_INTERNATIONAL5 = 139,
	SDL_SCANCODE_INTERNATIONAL6 = 140,
	SDL_SCANCODE_INTERNATIONAL7 = 141,
	SDL_SCANCODE_INTERNATIONAL8 = 142,
	SDL_SCANCODE_INTERNATIONAL9 = 143,
	SDL_SCANCODE_LANG1 = 144, /**< Hangul/English toggle */
	SDL_SCANCODE_LANG2 = 145, /**< Hanja conversion */
	SDL_SCANCODE_LANG3 = 146, /**< Katakana */
	SDL_SCANCODE_LANG4 = 147, /**< Hiragana */
	SDL_SCANCODE_LANG5 = 148, /**< Zenkaku/Hankaku */
	SDL_SCANCODE_LANG6 = 149, /**< reserved */
	SDL_SCANCODE_LANG7 = 150, /**< reserved */
	SDL_SCANCODE_LANG8 = 151, /**< reserved */
	SDL_SCANCODE_LANG9 = 152, /**< reserved */

	SDL_SCANCODE_ALTERASE = 153, /**< Erase-Eaze */
	SDL_SCANCODE_SYSREQ = 154,
	SDL_SCANCODE_CANCEL = 155,
	SDL_SCANCODE_CLEAR = 156,
	SDL_SCANCODE_PRIOR = 157,
	SDL_SCANCODE_RETURN2 = 158,
	SDL_SCANCODE_SEPARATOR = 159,
	SDL_SCANCODE_OUT = 160,
	SDL_SCANCODE_OPER = 161,
	SDL_SCANCODE_CLEARAGAIN = 162,
	SDL_SCANCODE_CRSEL = 163,
	SDL_SCANCODE_EXSEL = 164,

	SDL_SCANCODE_KP_00 = 176,
	SDL_SCANCODE_KP_000 = 177,
	SDL_SCANCODE_THOUSANDSSEPARATOR = 178,
	SDL_SCANCODE_DECIMALSEPARATOR = 179,
	SDL_SCANCODE_CURRENCYUNIT = 180,
	SDL_SCANCODE_CURRENCYSUBUNIT = 181,
	SDL_SCANCODE_KP_LEFTPAREN = 182,
	SDL_SCANCODE_KP_RIGHTPAREN = 183,
	SDL_SCANCODE_KP_LEFTBRACE = 184,
	SDL_SCANCODE_KP_RIGHTBRACE = 185,
	SDL_SCANCODE_KP_TAB = 186,
	SDL_SCANCODE_KP_BACKSPACE = 187,
	SDL_SCANCODE_KP_A = 188,
	SDL_SCANCODE_KP_B = 189,
	SDL_SCANCODE_KP_C = 190,
	SDL_SCANCODE_KP_D = 191,
	SDL_SCANCODE_KP_E = 192,
	SDL_SCANCODE_KP_F = 193,
	SDL_SCANCODE_KP_XOR = 194,
	SDL_SCANCODE_KP_POWER = 195,
	SDL_SCANCODE_KP_PERCENT = 196,
	SDL_SCANCODE_KP_LESS = 197,
	SDL_SCANCODE_KP_GREATER = 198,
	SDL_SCANCODE_KP_AMPERSAND = 199,
	SDL_SCANCODE_KP_DBLAMPERSAND = 200,
	SDL_SCANCODE_KP_VERTICALBAR = 201,
	SDL_SCANCODE_KP_DBLVERTICALBAR = 202,
	SDL_SCANCODE_KP_COLON = 203,
	SDL_SCANCODE_KP_HASH = 204,
	SDL_SCANCODE_KP_SPACE = 205,
	SDL_SCANCODE_KP_AT = 206,
	SDL_SCANCODE_KP_EXCLAM = 207,
	SDL_SCANCODE_KP_MEMSTORE = 208,
	SDL_SCANCODE_KP_MEMRECALL = 209,
	SDL_SCANCODE_KP_MEMCLEAR = 210,
	SDL_SCANCODE_KP_MEMADD = 211,
	SDL_SCANCODE_KP_MEMSUBTRACT = 212,
	SDL_SCANCODE_KP_MEMMULTIPLY = 213,
	SDL_SCANCODE_KP_MEMDIVIDE = 214,
	SDL_SCANCODE_KP_PLUSMINUS = 215,
	SDL_SCANCODE_KP_CLEAR = 216,
	SDL_SCANCODE_KP_CLEARENTRY = 217,
	SDL_SCANCODE_KP_BINARY = 218,
	SDL_SCANCODE_KP_OCTAL = 219,
	SDL_SCANCODE_KP_DECIMAL = 220,
	SDL_SCANCODE_KP_HEXADECIMAL = 221,

	SDL_SCANCODE_LCTRL = 224,
	SDL_SCANCODE_LSHIFT = 225,
	SDL_SCANCODE_LALT = 226, /**< alt, option */
	SDL_SCANCODE_LGUI = 227, /**< windows, command (apple), meta */
	SDL_SCANCODE_RCTRL = 228,
	SDL_SCANCODE_RSHIFT = 229,
	SDL_SCANCODE_RALT = 230, /**< alt gr, option */
	SDL_SCANCODE_RGUI = 231, /**< windows, command (apple), meta */

	SDL_SCANCODE_MODE = 257,    /**< I'm not sure if this is really not covered
										  *   by any of the above, but since there's a
										  *   special KMOD_MODE for it I'm adding it here
										  */

	/* @} *//* Usage page 0x07 */

	/**
	 *  \name Usage page 0x0C
	 *
	 *  These values are mapped from usage page 0x0C (USB consumer page).
	 */
	/* @{ */

	SDL_SCANCODE_AUDIONEXT = 258,
	SDL_SCANCODE_AUDIOPREV = 259,
	SDL_SCANCODE_AUDIOSTOP = 260,
	SDL_SCANCODE_AUDIOPLAY = 261,
	SDL_SCANCODE_AUDIOMUTE = 262,
	SDL_SCANCODE_MEDIASELECT = 263,
	SDL_SCANCODE_WWW = 264,
	SDL_SCANCODE_MAIL = 265,
	SDL_SCANCODE_CALCULATOR = 266,
	SDL_SCANCODE_COMPUTER = 267,
	SDL_SCANCODE_AC_SEARCH = 268,
	SDL_SCANCODE_AC_HOME = 269,
	SDL_SCANCODE_AC_BACK = 270,
	SDL_SCANCODE_AC_FORWARD = 271,
	SDL_SCANCODE_AC_STOP = 272,
	SDL_SCANCODE_AC_REFRESH = 273,
	SDL_SCANCODE_AC_BOOKMARKS = 274,

	/* @} *//* Usage page 0x0C */

	/**
	 *  \name Walther keys
	 *
	 *  These are values that Christian Walther added (for mac keyboard?).
	 */
	/* @{ */

	SDL_SCANCODE_BRIGHTNESSDOWN = 275,
	SDL_SCANCODE_BRIGHTNESSUP = 276,
	SDL_SCANCODE_DISPLAYSWITCH = 277, /**< display mirroring/dual display
													switch, video mode switch */
	SDL_SCANCODE_KBDILLUMTOGGLE = 278,
	SDL_SCANCODE_KBDILLUMDOWN = 279,
	SDL_SCANCODE_KBDILLUMUP = 280,
	SDL_SCANCODE_EJECT = 281,
	SDL_SCANCODE_SLEEP = 282,

	SDL_SCANCODE_APP1 = 283,
	SDL_SCANCODE_APP2 = 284,

	/* @} *//* Walther keys */

	/**
	 *  \name Usage page 0x0C (additional media keys)
	 *
	 *  These values are mapped from usage page 0x0C (USB consumer page).
	 */
	/* @{ */

	SDL_SCANCODE_AUDIOREWIND = 285,
	SDL_SCANCODE_AUDIOFASTFORWARD = 286,

	/* @} *//* Usage page 0x0C (additional media keys) */

	/* Add any other keys here. */

	SDL_NUM_SCANCODES = 512 /**< not a key, just marks the number of scancodes
									  for array bounds */
} SDL_Scancode;
#endif

struct keyboard_key_s keyboard_map[512];
char *KEYB_map[512];


int KEYB_is_match( char *a, char *b ) {
	int result;

	result = strcmp(a,b);

	if (result == 0) return 1;
	else return 0;
}
int KEYB_keycode_decode( char *input ) {
	int keyvalue = -1;
	if (KEYB_is_match(input,"A")) keyvalue = SDL_SCANCODE_A;
	if (KEYB_is_match(input,"B")) keyvalue = SDL_SCANCODE_B;
	if (KEYB_is_match(input,"C")) keyvalue = SDL_SCANCODE_C;
	if (KEYB_is_match(input,"D")) keyvalue = SDL_SCANCODE_D;
	if (KEYB_is_match(input,"E")) keyvalue = SDL_SCANCODE_E;
	if (KEYB_is_match(input,"F")) keyvalue = SDL_SCANCODE_F;
	if (KEYB_is_match(input,"G")) keyvalue = SDL_SCANCODE_G;
	if (KEYB_is_match(input,"H")) keyvalue = SDL_SCANCODE_H;
	if (KEYB_is_match(input,"I")) keyvalue = SDL_SCANCODE_I;
	if (KEYB_is_match(input,"J")) keyvalue = SDL_SCANCODE_J;
	if (KEYB_is_match(input,"K")) keyvalue = SDL_SCANCODE_K;
	if (KEYB_is_match(input,"L")) keyvalue = SDL_SCANCODE_L;
	if (KEYB_is_match(input,"M")) keyvalue = SDL_SCANCODE_M;
	if (KEYB_is_match(input,"N")) keyvalue = SDL_SCANCODE_N;
	if (KEYB_is_match(input,"O")) keyvalue = SDL_SCANCODE_O;
	if (KEYB_is_match(input,"P")) keyvalue = SDL_SCANCODE_P;
	if (KEYB_is_match(input,"Q")) keyvalue = SDL_SCANCODE_Q;
	if (KEYB_is_match(input,"R")) keyvalue = SDL_SCANCODE_R;
	if (KEYB_is_match(input,"S")) keyvalue = SDL_SCANCODE_S;
	if (KEYB_is_match(input,"T")) keyvalue = SDL_SCANCODE_T;
	if (KEYB_is_match(input,"U")) keyvalue = SDL_SCANCODE_U;
	if (KEYB_is_match(input,"V")) keyvalue = SDL_SCANCODE_V;
	if (KEYB_is_match(input,"W")) keyvalue = SDL_SCANCODE_W;
	if (KEYB_is_match(input,"X")) keyvalue = SDL_SCANCODE_X;
	if (KEYB_is_match(input,"Y")) keyvalue = SDL_SCANCODE_Y;
	if (KEYB_is_match(input,"Z")) keyvalue = SDL_SCANCODE_Z;

	if (KEYB_is_match(input,"1")) keyvalue = SDL_SCANCODE_1;
	if (KEYB_is_match(input,"2")) keyvalue = SDL_SCANCODE_2;
	if (KEYB_is_match(input,"3")) keyvalue = SDL_SCANCODE_3;
	if (KEYB_is_match(input,"4")) keyvalue = SDL_SCANCODE_4;
	if (KEYB_is_match(input,"5")) keyvalue = SDL_SCANCODE_5;
	if (KEYB_is_match(input,"6")) keyvalue = SDL_SCANCODE_6;
	if (KEYB_is_match(input,"7")) keyvalue = SDL_SCANCODE_7;
	if (KEYB_is_match(input,"8")) keyvalue = SDL_SCANCODE_8;
	if (KEYB_is_match(input,"9")) keyvalue = SDL_SCANCODE_9;
	if (KEYB_is_match(input,"0")) keyvalue = SDL_SCANCODE_0;

	if (KEYB_is_match(input,"RETURN")) keyvalue = SDL_SCANCODE_RETURN;
	if (KEYB_is_match(input,"ESCAPE")) keyvalue = SDL_SCANCODE_ESCAPE;
	if (KEYB_is_match(input,"BACKSPACE")) keyvalue = SDL_SCANCODE_BACKSPACE;
	if (KEYB_is_match(input,"TAB")) keyvalue = SDL_SCANCODE_TAB;
	if (KEYB_is_match(input,"SPACE")) keyvalue = SDL_SCANCODE_SPACE;

	if (KEYB_is_match(input,"MINUS")) keyvalue = SDL_SCANCODE_MINUS;
	if (KEYB_is_match(input,"EQUALS")) keyvalue = SDL_SCANCODE_EQUALS;
	if (KEYB_is_match(input,"LEFTBRACKET")) keyvalue = SDL_SCANCODE_LEFTBRACKET;
	if (KEYB_is_match(input,"RIGHTBRACKET")) keyvalue = SDL_SCANCODE_RIGHTBRACKET;
	if (KEYB_is_match(input,"BACKSLASH")) keyvalue = SDL_SCANCODE_BACKSLASH;
	if (KEYB_is_match(input,"NONUSHASH")) keyvalue = SDL_SCANCODE_NONUSHASH;
	if (KEYB_is_match(input,"SEMICOLON")) keyvalue = SDL_SCANCODE_SEMICOLON;
	if (KEYB_is_match(input,"APOSTROPHE")) keyvalue = SDL_SCANCODE_APOSTROPHE;
	if (KEYB_is_match(input,"GRAVE")) keyvalue = SDL_SCANCODE_GRAVE;
	if (KEYB_is_match(input,"COMMA")) keyvalue = SDL_SCANCODE_COMMA;
	if (KEYB_is_match(input,"PERIOD")) keyvalue = SDL_SCANCODE_PERIOD;
	if (KEYB_is_match(input,"SLASH")) keyvalue = SDL_SCANCODE_SLASH;

	if (KEYB_is_match(input,"CAPSLOCK")) keyvalue = SDL_SCANCODE_CAPSLOCK;

	if (KEYB_is_match(input,"F1")) keyvalue = SDL_SCANCODE_F1;
	if (KEYB_is_match(input,"F2")) keyvalue = SDL_SCANCODE_F2;
	if (KEYB_is_match(input,"F3")) keyvalue = SDL_SCANCODE_F3;
	if (KEYB_is_match(input,"F4")) keyvalue = SDL_SCANCODE_F4;
	if (KEYB_is_match(input,"F5")) keyvalue = SDL_SCANCODE_F5;
	if (KEYB_is_match(input,"F6")) keyvalue = SDL_SCANCODE_F6;
	if (KEYB_is_match(input,"F7")) keyvalue = SDL_SCANCODE_F7;
	if (KEYB_is_match(input,"F8")) keyvalue = SDL_SCANCODE_F8;
	if (KEYB_is_match(input,"F9")) keyvalue = SDL_SCANCODE_F9;
	if (KEYB_is_match(input,"F10")) keyvalue = SDL_SCANCODE_F10;
	if (KEYB_is_match(input,"F11")) keyvalue = SDL_SCANCODE_F11;
	if (KEYB_is_match(input,"F12")) keyvalue = SDL_SCANCODE_F12;
	if (KEYB_is_match(input,"PRINTSCREEN")) keyvalue = SDL_SCANCODE_PRINTSCREEN;
	if (KEYB_is_match(input,"SCROLLLOCK")) keyvalue = SDL_SCANCODE_SCROLLLOCK;
	if (KEYB_is_match(input,"PAUSE")) keyvalue = SDL_SCANCODE_PAUSE;
	if (KEYB_is_match(input,"INSERT")) keyvalue = SDL_SCANCODE_INSERT;
	if (KEYB_is_match(input,"HOME")) keyvalue = SDL_SCANCODE_HOME;
	if (KEYB_is_match(input,"PAGEUP")) keyvalue = SDL_SCANCODE_PAGEUP;
	if (KEYB_is_match(input,"DELETE")) keyvalue = SDL_SCANCODE_DELETE;
	if (KEYB_is_match(input,"END")) keyvalue = SDL_SCANCODE_END;
	if (KEYB_is_match(input,"PAGEDOWN")) keyvalue = SDL_SCANCODE_PAGEDOWN;
	if (KEYB_is_match(input,"RIGHT")) keyvalue = SDL_SCANCODE_RIGHT;
	if (KEYB_is_match(input,"LEFT")) keyvalue = SDL_SCANCODE_LEFT;
	if (KEYB_is_match(input,"DOWN")) keyvalue = SDL_SCANCODE_DOWN;
	if (KEYB_is_match(input,"UP")) keyvalue = SDL_SCANCODE_UP;
	if (KEYB_is_match(input,"NUMLOCKCLEAR")) keyvalue = SDL_SCANCODE_NUMLOCKCLEAR;
	if (KEYB_is_match(input,"KP_DIVIDE")) keyvalue = SDL_SCANCODE_KP_DIVIDE;
	if (KEYB_is_match(input,"KP_MULTIPLY")) keyvalue = SDL_SCANCODE_KP_MULTIPLY;
	if (KEYB_is_match(input,"KP_MINUS")) keyvalue = SDL_SCANCODE_KP_MINUS;
	if (KEYB_is_match(input,"KP_PLUS")) keyvalue = SDL_SCANCODE_KP_PLUS;
	if (KEYB_is_match(input,"KP_ENTER")) keyvalue = SDL_SCANCODE_KP_ENTER;
	if (KEYB_is_match(input,"KP_1")) keyvalue = SDL_SCANCODE_KP_1;
	if (KEYB_is_match(input,"KP_2")) keyvalue = SDL_SCANCODE_KP_2;
	if (KEYB_is_match(input,"KP_3")) keyvalue = SDL_SCANCODE_KP_3;
	if (KEYB_is_match(input,"KP_4")) keyvalue = SDL_SCANCODE_KP_4;
	if (KEYB_is_match(input,"KP_5")) keyvalue = SDL_SCANCODE_KP_5;
	if (KEYB_is_match(input,"KP_6")) keyvalue = SDL_SCANCODE_KP_6;
	if (KEYB_is_match(input,"KP_7")) keyvalue = SDL_SCANCODE_KP_7;
	if (KEYB_is_match(input,"KP_8")) keyvalue = SDL_SCANCODE_KP_8;
	if (KEYB_is_match(input,"KP_9")) keyvalue = SDL_SCANCODE_KP_9;
	if (KEYB_is_match(input,"KP_0")) keyvalue = SDL_SCANCODE_KP_0;
	if (KEYB_is_match(input,"KP_PERIOD")) keyvalue = SDL_SCANCODE_KP_PERIOD;

	if (KEYB_is_match(input,"NONUSBACKSLASH")) keyvalue = SDL_SCANCODE_NONUSBACKSLASH;
	if (KEYB_is_match(input,"APPLICATION")) keyvalue = SDL_SCANCODE_APPLICATION;
	if (KEYB_is_match(input,"POWER")) keyvalue = SDL_SCANCODE_POWER;
	if (KEYB_is_match(input,"KP_EQUALS")) keyvalue = SDL_SCANCODE_KP_EQUALS;
	if (KEYB_is_match(input,"F13")) keyvalue = SDL_SCANCODE_F13;
	if (KEYB_is_match(input,"F14")) keyvalue = SDL_SCANCODE_F14;
	if (KEYB_is_match(input,"F15")) keyvalue = SDL_SCANCODE_F15;
	if (KEYB_is_match(input,"F16")) keyvalue = SDL_SCANCODE_F16;
	if (KEYB_is_match(input,"F17")) keyvalue = SDL_SCANCODE_F17;
	if (KEYB_is_match(input,"F18")) keyvalue = SDL_SCANCODE_F18;
	if (KEYB_is_match(input,"F19")) keyvalue = SDL_SCANCODE_F19;
	if (KEYB_is_match(input,"F20")) keyvalue = SDL_SCANCODE_F20;
	if (KEYB_is_match(input,"F21")) keyvalue = SDL_SCANCODE_F21;
	if (KEYB_is_match(input,"F22")) keyvalue = SDL_SCANCODE_F22;
	if (KEYB_is_match(input,"F23")) keyvalue = SDL_SCANCODE_F23;
	if (KEYB_is_match(input,"F24")) keyvalue = SDL_SCANCODE_F24;
	if (KEYB_is_match(input,"EXECUTE")) keyvalue = SDL_SCANCODE_EXECUTE;
	if (KEYB_is_match(input,"HELP")) keyvalue = SDL_SCANCODE_HELP;
	if (KEYB_is_match(input,"MENU")) keyvalue = SDL_SCANCODE_MENU;
	if (KEYB_is_match(input,"SELECT")) keyvalue = SDL_SCANCODE_SELECT;
	if (KEYB_is_match(input,"STOP")) keyvalue = SDL_SCANCODE_STOP;
	if (KEYB_is_match(input,"AGAIN")) keyvalue = SDL_SCANCODE_AGAIN;
	if (KEYB_is_match(input,"UNDO")) keyvalue = SDL_SCANCODE_UNDO;
	if (KEYB_is_match(input,"CUT")) keyvalue = SDL_SCANCODE_CUT;
	if (KEYB_is_match(input,"COPY")) keyvalue = SDL_SCANCODE_COPY;
	if (KEYB_is_match(input,"PASTE")) keyvalue = SDL_SCANCODE_PASTE;
	if (KEYB_is_match(input,"FIND")) keyvalue = SDL_SCANCODE_FIND;
	if (KEYB_is_match(input,"MUTE")) keyvalue = SDL_SCANCODE_MUTE;
	if (KEYB_is_match(input,"VOLUMEUP")) keyvalue = SDL_SCANCODE_VOLUMEUP;
	if (KEYB_is_match(input,"VOLUMEDOWN")) keyvalue = SDL_SCANCODE_VOLUMEDOWN;
	if (KEYB_is_match(input,"KP_COMMA")) keyvalue = SDL_SCANCODE_KP_COMMA;
	if (KEYB_is_match(input,"KP_EQUALSAS400")) keyvalue = SDL_SCANCODE_KP_EQUALSAS400;

	if (KEYB_is_match(input,"INTERNATIONAL1")) keyvalue = SDL_SCANCODE_INTERNATIONAL1;

	if (KEYB_is_match(input,"INTERNATIONAL2")) keyvalue = SDL_SCANCODE_INTERNATIONAL2;
	if (KEYB_is_match(input,"INTERNATIONAL3")) keyvalue = SDL_SCANCODE_INTERNATIONAL3;
	if (KEYB_is_match(input,"INTERNATIONAL4")) keyvalue = SDL_SCANCODE_INTERNATIONAL4;
	if (KEYB_is_match(input,"INTERNATIONAL5")) keyvalue = SDL_SCANCODE_INTERNATIONAL5;
	if (KEYB_is_match(input,"INTERNATIONAL6")) keyvalue = SDL_SCANCODE_INTERNATIONAL6;
	if (KEYB_is_match(input,"INTERNATIONAL7")) keyvalue = SDL_SCANCODE_INTERNATIONAL7;
	if (KEYB_is_match(input,"INTERNATIONAL8")) keyvalue = SDL_SCANCODE_INTERNATIONAL8;
	if (KEYB_is_match(input,"INTERNATIONAL9")) keyvalue = SDL_SCANCODE_INTERNATIONAL9;
	if (KEYB_is_match(input,"LANG1")) keyvalue = SDL_SCANCODE_LANG1;
	if (KEYB_is_match(input,"LANG2")) keyvalue = SDL_SCANCODE_LANG2;
	if (KEYB_is_match(input,"LANG3")) keyvalue = SDL_SCANCODE_LANG3;
	if (KEYB_is_match(input,"LANG4")) keyvalue = SDL_SCANCODE_LANG4;
	if (KEYB_is_match(input,"LANG5")) keyvalue = SDL_SCANCODE_LANG5;
	if (KEYB_is_match(input,"LANG6")) keyvalue = SDL_SCANCODE_LANG6;
	if (KEYB_is_match(input,"LANG7")) keyvalue = SDL_SCANCODE_LANG7;
	if (KEYB_is_match(input,"LANG8")) keyvalue = SDL_SCANCODE_LANG8;
	if (KEYB_is_match(input,"LANG9")) keyvalue = SDL_SCANCODE_LANG9;

	if (KEYB_is_match(input,"ALTERASE")) keyvalue = SDL_SCANCODE_ALTERASE;
	if (KEYB_is_match(input,"SYSREQ")) keyvalue = SDL_SCANCODE_SYSREQ;
	if (KEYB_is_match(input,"CANCEL")) keyvalue = SDL_SCANCODE_CANCEL;
	if (KEYB_is_match(input,"CLEAR")) keyvalue = SDL_SCANCODE_CLEAR;
	if (KEYB_is_match(input,"PRIOR")) keyvalue = SDL_SCANCODE_PRIOR;
	if (KEYB_is_match(input,"RETURN2")) keyvalue = SDL_SCANCODE_RETURN2;
	if (KEYB_is_match(input,"SEPARATOR")) keyvalue = SDL_SCANCODE_SEPARATOR;
	if (KEYB_is_match(input,"OUT")) keyvalue = SDL_SCANCODE_OUT;
	if (KEYB_is_match(input,"OPER")) keyvalue = SDL_SCANCODE_OPER;
	if (KEYB_is_match(input,"CLEARAGAIN")) keyvalue = SDL_SCANCODE_CLEARAGAIN;
	if (KEYB_is_match(input,"CRSEL")) keyvalue = SDL_SCANCODE_CRSEL;
	if (KEYB_is_match(input,"EXSEL")) keyvalue = SDL_SCANCODE_EXSEL;

	if (KEYB_is_match(input,"KP_00")) keyvalue = SDL_SCANCODE_KP_00;
	if (KEYB_is_match(input,"KP_000")) keyvalue = SDL_SCANCODE_KP_000;
	if (KEYB_is_match(input,"THOUSANDSSEPARATOR")) keyvalue = SDL_SCANCODE_THOUSANDSSEPARATOR;
	if (KEYB_is_match(input,"DECIMALSEPARATOR")) keyvalue = SDL_SCANCODE_DECIMALSEPARATOR;
	if (KEYB_is_match(input,"CURRENCYUNIT")) keyvalue = SDL_SCANCODE_CURRENCYUNIT;
	if (KEYB_is_match(input,"CURRENCYSUBUNIT")) keyvalue = SDL_SCANCODE_CURRENCYSUBUNIT;
	if (KEYB_is_match(input,"KP_LEFTPAREN")) keyvalue = SDL_SCANCODE_KP_LEFTPAREN;
	if (KEYB_is_match(input,"KP_RIGHTPAREN")) keyvalue = SDL_SCANCODE_KP_RIGHTPAREN;
	if (KEYB_is_match(input,"KP_LEFTBRACE")) keyvalue = SDL_SCANCODE_KP_LEFTBRACE;
	if (KEYB_is_match(input,"KP_RIGHTBRACE")) keyvalue = SDL_SCANCODE_KP_RIGHTBRACE;
	if (KEYB_is_match(input,"KP_TAB")) keyvalue = SDL_SCANCODE_KP_TAB;
	if (KEYB_is_match(input,"KP_BACKSPACE")) keyvalue = SDL_SCANCODE_KP_BACKSPACE;
	if (KEYB_is_match(input,"KP_A")) keyvalue = SDL_SCANCODE_KP_A;
	if (KEYB_is_match(input,"KP_B")) keyvalue = SDL_SCANCODE_KP_B;
	if (KEYB_is_match(input,"KP_C")) keyvalue = SDL_SCANCODE_KP_C;
	if (KEYB_is_match(input,"KP_D")) keyvalue = SDL_SCANCODE_KP_D;
	if (KEYB_is_match(input,"KP_E")) keyvalue = SDL_SCANCODE_KP_E;
	if (KEYB_is_match(input,"KP_F")) keyvalue = SDL_SCANCODE_KP_F;
	if (KEYB_is_match(input,"KP_XOR")) keyvalue = SDL_SCANCODE_KP_XOR;
	if (KEYB_is_match(input,"KP_POWER")) keyvalue = SDL_SCANCODE_KP_POWER;
	if (KEYB_is_match(input,"KP_PERCENT")) keyvalue = SDL_SCANCODE_KP_PERCENT;
	if (KEYB_is_match(input,"KP_LESS")) keyvalue = SDL_SCANCODE_KP_LESS;
	if (KEYB_is_match(input,"KP_GREATER")) keyvalue = SDL_SCANCODE_KP_GREATER;
	if (KEYB_is_match(input,"KP_AMPERSAND")) keyvalue = SDL_SCANCODE_KP_AMPERSAND;
	if (KEYB_is_match(input,"KP_DBLAMPERSAND")) keyvalue = SDL_SCANCODE_KP_DBLAMPERSAND;
	if (KEYB_is_match(input,"KP_VERTICALBAR")) keyvalue = SDL_SCANCODE_KP_VERTICALBAR;
	if (KEYB_is_match(input,"KP_DBLVERTICALBAR")) keyvalue = SDL_SCANCODE_KP_DBLVERTICALBAR;
	if (KEYB_is_match(input,"KP_COLON")) keyvalue = SDL_SCANCODE_KP_COLON;
	if (KEYB_is_match(input,"KP_HASH")) keyvalue = SDL_SCANCODE_KP_HASH;
	if (KEYB_is_match(input,"KP_SPACE")) keyvalue = SDL_SCANCODE_KP_SPACE;
	if (KEYB_is_match(input,"KP_AT")) keyvalue = SDL_SCANCODE_KP_AT;
	if (KEYB_is_match(input,"KP_EXCLAM")) keyvalue = SDL_SCANCODE_KP_EXCLAM;
	if (KEYB_is_match(input,"KP_MEMSTORE")) keyvalue = SDL_SCANCODE_KP_MEMSTORE;
	if (KEYB_is_match(input,"KP_MEMRECALL")) keyvalue = SDL_SCANCODE_KP_MEMRECALL;
	if (KEYB_is_match(input,"KP_MEMCLEAR")) keyvalue = SDL_SCANCODE_KP_MEMCLEAR;
	if (KEYB_is_match(input,"KP_MEMADD")) keyvalue = SDL_SCANCODE_KP_MEMADD;
	if (KEYB_is_match(input,"KP_MEMSUBTRACT")) keyvalue = SDL_SCANCODE_KP_MEMSUBTRACT;
	if (KEYB_is_match(input,"KP_MEMMULTIPLY")) keyvalue = SDL_SCANCODE_KP_MEMMULTIPLY;
	if (KEYB_is_match(input,"KP_MEMDIVIDE")) keyvalue = SDL_SCANCODE_KP_MEMDIVIDE;
	if (KEYB_is_match(input,"KP_PLUSMINUS")) keyvalue = SDL_SCANCODE_KP_PLUSMINUS;
	if (KEYB_is_match(input,"KP_CLEAR")) keyvalue = SDL_SCANCODE_KP_CLEAR;
	if (KEYB_is_match(input,"KP_CLEARENTRY")) keyvalue = SDL_SCANCODE_KP_CLEARENTRY;
	if (KEYB_is_match(input,"KP_BINARY")) keyvalue = SDL_SCANCODE_KP_BINARY;
	if (KEYB_is_match(input,"KP_OCTAL")) keyvalue = SDL_SCANCODE_KP_OCTAL;
	if (KEYB_is_match(input,"KP_DECIMAL")) keyvalue = SDL_SCANCODE_KP_DECIMAL;
	if (KEYB_is_match(input,"KP_HEXADECIMAL")) keyvalue = SDL_SCANCODE_KP_HEXADECIMAL;

	if (KEYB_is_match(input,"LCTRL")) keyvalue = SDL_SCANCODE_LCTRL;
	if (KEYB_is_match(input,"LSHIFT")) keyvalue = SDL_SCANCODE_LSHIFT;
	if (KEYB_is_match(input,"LALT")) keyvalue = SDL_SCANCODE_LALT;
	if (KEYB_is_match(input,"LGUI")) keyvalue = SDL_SCANCODE_LGUI;
	if (KEYB_is_match(input,"RCTRL")) keyvalue = SDL_SCANCODE_RCTRL;
	if (KEYB_is_match(input,"RSHIFT")) keyvalue = SDL_SCANCODE_RSHIFT;
	if (KEYB_is_match(input,"RALT")) keyvalue = SDL_SCANCODE_RALT;
	if (KEYB_is_match(input,"RGUI")) keyvalue = SDL_SCANCODE_RGUI;

	if (KEYB_is_match(input,"MODE")) keyvalue = SDL_SCANCODE_MODE;

	if (KEYB_is_match(input,"AUDIONEXT")) keyvalue = SDL_SCANCODE_AUDIONEXT;
	if (KEYB_is_match(input,"AUDIOPREV")) keyvalue = SDL_SCANCODE_AUDIOPREV;
	if (KEYB_is_match(input,"AUDIOSTOP")) keyvalue = SDL_SCANCODE_AUDIOSTOP;
	if (KEYB_is_match(input,"AUDIOPLAY")) keyvalue = SDL_SCANCODE_AUDIOPLAY;
	if (KEYB_is_match(input,"AUDIOMUTE")) keyvalue = SDL_SCANCODE_AUDIOMUTE;
	if (KEYB_is_match(input,"MEDIASELECT")) keyvalue = SDL_SCANCODE_MEDIASELECT;
	if (KEYB_is_match(input,"WWW")) keyvalue = SDL_SCANCODE_WWW;
	if (KEYB_is_match(input,"MAIL")) keyvalue = SDL_SCANCODE_MAIL;
	if (KEYB_is_match(input,"CALCULATOR")) keyvalue = SDL_SCANCODE_CALCULATOR;
	if (KEYB_is_match(input,"COMPUTER")) keyvalue = SDL_SCANCODE_COMPUTER;
	if (KEYB_is_match(input,"AC_SEARCH")) keyvalue = SDL_SCANCODE_AC_SEARCH;
	if (KEYB_is_match(input,"AC_HOME")) keyvalue = SDL_SCANCODE_AC_HOME;
	if (KEYB_is_match(input,"AC_BACK")) keyvalue = SDL_SCANCODE_AC_BACK;
	if (KEYB_is_match(input,"AC_FORWARD")) keyvalue = SDL_SCANCODE_AC_FORWARD;
	if (KEYB_is_match(input,"AC_STOP")) keyvalue = SDL_SCANCODE_AC_STOP;
	if (KEYB_is_match(input,"AC_REFRESH")) keyvalue = SDL_SCANCODE_AC_REFRESH;
	if (KEYB_is_match(input,"AC_BOOKMARKS")) keyvalue = SDL_SCANCODE_AC_BOOKMARKS;

	if (KEYB_is_match(input,"BRIGHTNESSDOWN")) keyvalue = SDL_SCANCODE_BRIGHTNESSDOWN;
	if (KEYB_is_match(input,"BRIGHTNESSUP")) keyvalue = SDL_SCANCODE_BRIGHTNESSUP;
	if (KEYB_is_match(input,"DISPLAYSWITCH")) keyvalue = SDL_SCANCODE_DISPLAYSWITCH;
	if (KEYB_is_match(input,"KBDILLUMTOGGLE")) keyvalue = SDL_SCANCODE_KBDILLUMTOGGLE;
	if (KEYB_is_match(input,"KBDILLUMDOWN")) keyvalue = SDL_SCANCODE_KBDILLUMDOWN;
	if (KEYB_is_match(input,"KBDILLUMUP")) keyvalue = SDL_SCANCODE_KBDILLUMUP;
	if (KEYB_is_match(input,"EJECT")) keyvalue = SDL_SCANCODE_EJECT;
	if (KEYB_is_match(input,"SLEEP")) keyvalue = SDL_SCANCODE_SLEEP;

	if (KEYB_is_match(input,"APP1")) keyvalue = SDL_SCANCODE_APP1;
	if (KEYB_is_match(input,"APP2")) keyvalue = SDL_SCANCODE_APP2;

	return keyvalue;
}

void KEYB_init( void ) {

	KEYB_map[0] = strdup("UNKNOWN");

	KEYB_map[4] = strdup("A");
	KEYB_map[5] = strdup("B");
	KEYB_map[6] = strdup("C");
	KEYB_map[7] = strdup("D");
	KEYB_map[8] = strdup("E");
	KEYB_map[9] = strdup("F");
	KEYB_map[10] = strdup("G");
	KEYB_map[11] = strdup("H");
	KEYB_map[12] = strdup("I");
	KEYB_map[13] = strdup("J");
	KEYB_map[14] = strdup("K");
	KEYB_map[15] = strdup("L");
	KEYB_map[16] = strdup("M");
	KEYB_map[17] = strdup("N");
	KEYB_map[18] = strdup("O");
	KEYB_map[19] = strdup("P");
	KEYB_map[20] = strdup("Q");
	KEYB_map[21] = strdup("R");
	KEYB_map[22] = strdup("S");
	KEYB_map[23] = strdup("T");
	KEYB_map[24] = strdup("U");
	KEYB_map[25] = strdup("V");
	KEYB_map[26] = strdup("W");
	KEYB_map[27] = strdup("X");
	KEYB_map[28] = strdup("Y");
	KEYB_map[29] = strdup("Z");

	KEYB_map[30] = strdup("1");
	KEYB_map[31] = strdup("2");
	KEYB_map[32] = strdup("3");
	KEYB_map[33] = strdup("4");
	KEYB_map[34] = strdup("5");
	KEYB_map[35] = strdup("6");
	KEYB_map[36] = strdup("7");
	KEYB_map[37] = strdup("8");
	KEYB_map[38] = strdup("9");
	KEYB_map[39] = strdup("0");

	KEYB_map[40] = strdup("RETURN");
	KEYB_map[41] = strdup("ESCAPE");
	KEYB_map[42] = strdup("BACKSPACE");
	KEYB_map[43] = strdup("TAB");
	KEYB_map[44] = strdup("SPACE");

	KEYB_map[45] = strdup("MINUS");
	KEYB_map[46] = strdup("EQUALS");
	KEYB_map[47] = strdup("LEFTBRACKET");
	KEYB_map[48] = strdup("RIGHTBRACKET");
	KEYB_map[49] = strdup("BACKSLASH"); 
	KEYB_map[50] = strdup("NONUSHASH"); 
	KEYB_map[51] = strdup("SEMICOLON");
	KEYB_map[52] = strdup("APOSTROPHE");
	KEYB_map[53] = strdup("GRAVE"); 
	KEYB_map[54] = strdup("COMMA");
	KEYB_map[55] = strdup("PERIOD");
	KEYB_map[56] = strdup("SLASH");

	KEYB_map[57] = strdup("CAPSLOCK");

	KEYB_map[58] = strdup("F1");
	KEYB_map[59] = strdup("F2");
	KEYB_map[60] = strdup("F3");
	KEYB_map[61] = strdup("F4");
	KEYB_map[62] = strdup("F5");
	KEYB_map[63] = strdup("F6");
	KEYB_map[64] = strdup("F7");
	KEYB_map[65] = strdup("F8");
	KEYB_map[66] = strdup("F9");
	KEYB_map[67] = strdup("F10");
	KEYB_map[68] = strdup("F11");
	KEYB_map[69] = strdup("F12");

	KEYB_map[70] = strdup("PRINTSCREEN");
	KEYB_map[71] = strdup("SCROLLLOCK");
	KEYB_map[72] = strdup("PAUSE");
	KEYB_map[73] = strdup("INSERT"); 
	KEYB_map[74] = strdup("HOME");
	KEYB_map[75] = strdup("PAGEUP");
	KEYB_map[76] = strdup("DELETE");
	KEYB_map[77] = strdup("END");
	KEYB_map[78] = strdup("PAGEDOWN");
	KEYB_map[79] = strdup("RIGHT");
	KEYB_map[80] = strdup("LEFT");
	KEYB_map[81] = strdup("DOWN");
	KEYB_map[82] = strdup("UP");

	KEYB_map[83] = strdup("NUMLOCKCLEAR"); 
	KEYB_map[84] = strdup("KP_DIVIDE");
	KEYB_map[85] = strdup("KP_MULTIPLY");
	KEYB_map[86] = strdup("KP_MINUS");
	KEYB_map[87] = strdup("KP_PLUS");
	KEYB_map[88] = strdup("KP_ENTER");
	KEYB_map[89] = strdup("KP_1");
	KEYB_map[90] = strdup("KP_2");
	KEYB_map[91] = strdup("KP_3");
	KEYB_map[92] = strdup("KP_4");
	KEYB_map[93] = strdup("KP_5");
	KEYB_map[94] = strdup("KP_6");
	KEYB_map[95] = strdup("KP_7");
	KEYB_map[96] = strdup("KP_8");
	KEYB_map[97] = strdup("KP_9");
	KEYB_map[98] = strdup("KP_0");
	KEYB_map[99] = strdup("KP_PERIOD");

	KEYB_map[100] = strdup("NONUSBACKSLASH"); 
	KEYB_map[101] = strdup("APPLICATION");
	KEYB_map[102] = strdup("POWER"); 
	KEYB_map[103] = strdup("KP_EQUALS");
	KEYB_map[104] = strdup("F13");
	KEYB_map[105] = strdup("F14");
	KEYB_map[106] = strdup("F15");
	KEYB_map[107] = strdup("F16");
	KEYB_map[108] = strdup("F17");
	KEYB_map[109] = strdup("F18");
	KEYB_map[110] = strdup("F19");
	KEYB_map[111] = strdup("F20");
	KEYB_map[112] = strdup("F21");
	KEYB_map[113] = strdup("F22");
	KEYB_map[114] = strdup("F23");
	KEYB_map[115] = strdup("F24");
	KEYB_map[116] = strdup("EXECUTE");
	KEYB_map[117] = strdup("HELP");
	KEYB_map[118] = strdup("MENU");
	KEYB_map[119] = strdup("SELECT");
	KEYB_map[120] = strdup("STOP");
	KEYB_map[121] = strdup("AGAIN");  
	KEYB_map[122] = strdup("UNDO");
	KEYB_map[123] = strdup("CUT");
	KEYB_map[124] = strdup("COPY");
	KEYB_map[125] = strdup("PASTE");
	KEYB_map[126] = strdup("FIND");
	KEYB_map[127] = strdup("MUTE");
	KEYB_map[128] = strdup("VOLUMEUP");
	KEYB_map[129] = strdup("VOLUMEDOWN");
	KEYB_map[133] = strdup("KP_COMMA");
	KEYB_map[134] = strdup("KP_EQUALSAS400");

	KEYB_map[135] = strdup("INTERNATIONAL1");
	KEYB_map[136] = strdup("INTERNATIONAL2");
	KEYB_map[137] = strdup("INTERNATIONAL3"); 
	KEYB_map[138] = strdup("INTERNATIONAL4");
	KEYB_map[139] = strdup("INTERNATIONAL5");
	KEYB_map[140] = strdup("INTERNATIONAL6");
	KEYB_map[141] = strdup("INTERNATIONAL7");
	KEYB_map[142] = strdup("INTERNATIONAL8");
	KEYB_map[143] = strdup("INTERNATIONAL9");
	KEYB_map[144] = strdup("LANG1");
	KEYB_map[145] = strdup("LANG2");
	KEYB_map[146] = strdup("LANG3");
	KEYB_map[147] = strdup("LANG4");
	KEYB_map[148] = strdup("LANG5");
	KEYB_map[149] = strdup("LANG6");
	KEYB_map[150] = strdup("LANG7");
	KEYB_map[151] = strdup("LANG8");
	KEYB_map[152] = strdup("LANG9");

	KEYB_map[153] = strdup("ALTERASE");
	KEYB_map[154] = strdup("SYSREQ");
	KEYB_map[155] = strdup("CANCEL");
	KEYB_map[156] = strdup("CLEAR");
	KEYB_map[157] = strdup("PRIOR");
	KEYB_map[158] = strdup("RETURN2");
	KEYB_map[159] = strdup("SEPARATOR");
	KEYB_map[160] = strdup("OUT");
	KEYB_map[161] = strdup("OPER");
	KEYB_map[162] = strdup("CLEARAGAIN");
	KEYB_map[163] = strdup("CRSEL");
	KEYB_map[164] = strdup("EXSEL");

	KEYB_map[176] = strdup("KP_00");
	KEYB_map[177] = strdup("KP_000");
	KEYB_map[178] = strdup("THOUSANDSSEPARATOR");
	KEYB_map[179] = strdup("DECIMALSEPARATOR");
	KEYB_map[180] = strdup("CURRENCYUNIT");
	KEYB_map[181] = strdup("CURRENCYSUBUNIT");
	KEYB_map[182] = strdup("KP_LEFTPAREN");
	KEYB_map[183] = strdup("KP_RIGHTPAREN");
	KEYB_map[184] = strdup("KP_LEFTBRACE");
	KEYB_map[185] = strdup("KP_RIGHTBRACE");
	KEYB_map[186] = strdup("KP_TAB");
	KEYB_map[187] = strdup("KP_BACKSPACE");
	KEYB_map[188] = strdup("KP_A");
	KEYB_map[189] = strdup("KP_B");
	KEYB_map[190] = strdup("KP_C");
	KEYB_map[191] = strdup("KP_D");
	KEYB_map[192] = strdup("KP_E");
	KEYB_map[193] = strdup("KP_F");
	KEYB_map[194] = strdup("KP_XOR");
	KEYB_map[195] = strdup("KP_POWER");
	KEYB_map[196] = strdup("KP_PERCENT");
	KEYB_map[197] = strdup("KP_LESS");
	KEYB_map[198] = strdup("KP_GREATER");
	KEYB_map[199] = strdup("KP_AMPERSAND");
	KEYB_map[200] = strdup("KP_DBLAMPERSAND");
	KEYB_map[201] = strdup("KP_VERTICALBAR");
	KEYB_map[202] = strdup("KP_DBLVERTICALBAR");
	KEYB_map[203] = strdup("KP_COLON");
	KEYB_map[204] = strdup("KP_HASH");
	KEYB_map[205] = strdup("KP_SPACE");
	KEYB_map[206] = strdup("KP_AT");
	KEYB_map[207] = strdup("KP_EXCLAM");
	KEYB_map[208] = strdup("KP_MEMSTORE");
	KEYB_map[209] = strdup("KP_MEMRECALL");
	KEYB_map[210] = strdup("KP_MEMCLEAR");
	KEYB_map[211] = strdup("KP_MEMADD");
	KEYB_map[212] = strdup("KP_MEMSUBTRACT");
	KEYB_map[213] = strdup("KP_MEMMULTIPLY");
	KEYB_map[214] = strdup("KP_MEMDIVIDE");
	KEYB_map[215] = strdup("KP_PLUSMINUS");
	KEYB_map[216] = strdup("KP_CLEAR");
	KEYB_map[217] = strdup("KP_CLEARENTRY");
	KEYB_map[218] = strdup("KP_BINARY");
	KEYB_map[219] = strdup("KP_OCTAL");
	KEYB_map[220] = strdup("KP_DECIMAL");
	KEYB_map[221] = strdup("KP_HEXADECIMAL");

	KEYB_map[224] = strdup("LCTRL");
	KEYB_map[225] = strdup("LSHIFT");
	KEYB_map[226] = strdup("LALT");
	KEYB_map[227] = strdup("LGUI");
	KEYB_map[228] = strdup("RCTRL");
	KEYB_map[229] = strdup("RSHIFT");
	KEYB_map[230] = strdup("RALT");
	KEYB_map[231] = strdup("RGUI");

	KEYB_map[257] = strdup("MODE");  

	KEYB_map[258] = strdup("AUDIONEXT");
	KEYB_map[259] = strdup("AUDIOPREV");
	KEYB_map[260] = strdup("AUDIOSTOP");
	KEYB_map[261] = strdup("AUDIOPLAY");
	KEYB_map[262] = strdup("AUDIOMUTE");
	KEYB_map[263] = strdup("MEDIASELECT");
	KEYB_map[264] = strdup("WWW");
	KEYB_map[265] = strdup("MAIL");
	KEYB_map[266] = strdup("CALCULATOR");
	KEYB_map[267] = strdup("COMPUTER");
	KEYB_map[268] = strdup("AC_SEARCH");
	KEYB_map[269] = strdup("AC_HOME");
	KEYB_map[270] = strdup("AC_BACK");
	KEYB_map[271] = strdup("AC_FORWARD");
	KEYB_map[272] = strdup("AC_STOP");
	KEYB_map[273] = strdup("AC_REFRESH");
	KEYB_map[274] = strdup("AC_BOOKMARKS");

	KEYB_map[275] = strdup("BRIGHTNESSDOWN");
	KEYB_map[276] = strdup("BRIGHTNESSUP");
	KEYB_map[277] = strdup("DISPLAYSWITCH");
	KEYB_map[278] = strdup("KBDILLUMTOGGLE");
	KEYB_map[279] = strdup("KBDILLUMDOWN");
	KEYB_map[280] = strdup("KBDILLUMUP");
	KEYB_map[281] = strdup("EJECT");
	KEYB_map[282] = strdup("SLEEP");

	KEYB_map[283] = strdup("APP1");
	KEYB_map[284] = strdup("APP2");
}


int KEYB_tokenise_keycodes( uint32_t *keyvalues, int maxvalues, const char *keycodes ) {

	char *p, *po, *hit;
	int i = 0;

	p = po = strdup(keycodes);

	//CCDBG fprintf(stderr,"Decoding %s\n", keycodes);

	do {

		if (!p) break;


		hit = strchr(p,KEYB_TOKENISING_CHAR);
		if ( (hit) || ((!hit) && (*p != '\0')) ) {
			int temp_keyvalue = -1;
			if (hit) *hit = '\0';
			temp_keyvalue = KEYB_keycode_decode( p );
			if (temp_keyvalue == -1) {
				fprintf(stderr,"Unknown keycode '%s'\n", p );
			} else {
				keyvalues[i] = temp_keyvalue;
				//fprintf(stderr," '%s' converted to '%d'\n", p, keyvalues[i]);
				i++;
				if (hit) p = hit+1;
			}
		} // hit

	} while ((i < maxvalues) && (hit) && (p) && (*p != '\0'));

	if (po) free(po);

	return i;
}

int KEYB_human_decode( struct keyboard_key_s *kitem, const char *human ) {

	int result = 0;
	result = KEYB_tokenise_keycodes( &(kitem->key), 1, human );

	return result;


}

char *KEYB_combo_to_string( char *b, int bmax, struct keyboard_key_s kitem ) {
	snprintf(b, bmax, "%s%s%s%s%s"
			, kitem.mods & KEYB_MOD_CTRL ? "CTRL+":""
			, kitem.mods & KEYB_MOD_ALT ? "ALT+":""
			, kitem.mods & KEYB_MOD_OS ? "SUPER+":""
			, kitem.mods & KEYB_MOD_SHIFT ? "SHIFT+":""
			, KEYB_map[kitem.key]
			);

	return b;
}

