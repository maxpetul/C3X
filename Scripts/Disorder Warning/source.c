
#include "C3X.h"

void __fastcall
patch_Main_Screen_Form_m82_handle_key_event (Main_Screen_Form * this, int edx, int virtual_key_code, int is_down)
{
	bool in_game = *p_player_bits != 0;
	if (virtual_key_code == VK_H && in_game && ! is_down) {
		PopupForm * popup = get_popup_form ();
		popup->vtable->set_text_key_and_flags (popup, __, MOD_SCRIPT_PATH, "C3X_INFO", -1, 0, 0, 0);
		PopupForm_add_text (popup, __, "Hello World!", false);
		show_popup (popup, __, 0, 0);
	}
	Main_Screen_Form_m82_handle_key_event (this, __, virtual_key_code, is_down);
}
