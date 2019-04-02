/*
 * Keyboard mapping
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#define KEYB_MOD_CTRL 1
#define KEYB_MOD_ALT 2
#define KEYB_MOD_SHIFT 4
#define KEYB_MOD_OS 8


struct keyboard_key_s {
	char human_string[256]; // the string we're associating with the keyboard combo
	uint32_t key;
	uint32_t mods;
};

extern struct keyboard_key_s keyboard_map[512];
extern char *KEYB_map[512];

void KEYB_init(void);
int KEYB_human_decode( struct keyboard_key_s *kitem, const char *human );
int KEYB_human_encode( char *human, struct keyboard_key_s *kitem );

int KEYB_add_map_item_h( struct keyboard_key_s *kmap, int index, char *human );
int KEYB_add_map_item( struct keyboard_key_s *kmap, int index, int key, int mods );

int KEYB_get_index_h( struct keyboard_key_s *kmap, char *human );
int KEYB_get_index( struct keyboard_key_s *kmap, int key, int mods );

char *KEYB_combo_to_string( char *b, int bmax, struct keyboard_key_s kitem );

#ifdef __cplusplus
}
#endif

