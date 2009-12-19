/*
 * sffi.c - Special FFI functions
 *
 * Copyright (C) 2009  Carl G. Ritson
 *
 */

#include "tvm-nxt.h"
#include <base/display.h>

/* PROC nx.display.clear () */
int _nx_display_clear (ECTX ectx, WORD args[]) {
	nx_display_clear ();
	return SFFI_OK;
}

/* PROC nx.display.cursor.set.pos (VAL INT x, y) */
int _nx_display_cursor_set_pos (ECTX ectx, WORD args[]) {
	nx_display_cursor_set_pos ((U8) args[0], (U8) args[1]);
	return SFFI_OK;
}

/* PROC nx.display.string (VAL []BYTE str) */
int _nx_display_string (ECTX ectx, WORD args[]) {
	BYTEPTR str 	= (BYTEPTR) args[0];
	WORD str_len	= args[1];
	WORD pos 	= 0;
	char buffer[24];

	while (pos < str_len) {
		WORD len;
		for (len = 0; len < 20 && pos < str_len; ++len, ++pos) {
			buffer[len] = read_byte (byteptr_plus (str, pos));
		}
		buffer[len] = '\0';
		nx_display_string (buffer);
	}
	
	return SFFI_OK;
}

SFFI_FUNCTION sffi_table[] = {
	_nx_display_clear,
	_nx_display_cursor_set_pos,
	_nx_display_string
};
const int sffi_table_length = sizeof(sffi_table) / sizeof(SFFI_FUNCTION);
