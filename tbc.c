/*
 * tbc.c - TVM bytecode support functions
 *
 * Copyright (C) 2008  Carl G. Ritson
 *
 */

#include "tvm-nxt.h"
#include <tvm_tbc.h>

static tbc_t *decode_tbc (BYTE *data, unsigned int length)
{
	tenc_element_t element;
	tbc_t *tbc = NULL;

	/* CGR FIXME: this function should move to libtvm. */

	if (tenc_decode_element (data, &length, &element))
		return NULL;

	#if TVM_WORD_LENGTH == 2
	if (tvm_memcmp (element.id, "tenc", 4) != 0)
	#else
	if (tvm_memcmp (element.id, "TEnc", 4) != 0)
	#endif
	{
		return NULL;
	}

	data	= element.data.bytes;
	length	= element.length;

	if (tenc_walk_to_element (data, &length, "tbcL", &element) < 0)
		return NULL;

	if (tbc_decode (element.data.bytes, element.length, &tbc))
		return NULL;

	return tbc;
}

int init_context_from_tbc (ECTX ectx, BYTE *data, UWORD length) {
	return -1;
}
