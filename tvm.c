#include "tvm-nxt.h"
#include <base/display.h>
#include <base/drivers/_usb.h>

static void *mem_pool = NX_USERSPACE_START; 
static tvm_t tvm;
static tvm_ectx_t context;

static WORD nxt_get_time (ECTX ectx) {
	return nx_systick_get_ms ();
}

static void nxt_modify_sync_flags (ECTX ectx, WORD set, WORD clear) {
	nx_interrupts_disable ();
	ectx->sflags = (ectx->sflags & (~clear)) | set;
	nx_interrupts_enable ();
}

void main (void) {
	UWORD tbc_length = 0;
	BYTE *tbc = NULL;
	
	tvm_init (&tvm);

	for (;;) {
		U8 buffer[NX_USB_PACKET_SIZE];
		WORD usb 	= 0;
		U32 pos		= 0;
		U32 i;
		int running 	= 1;

		nx_display_clear ();
		nx_display_string ("I am the Transterpreter,");
		nx_display_end_line ();
		nx_display_string ("        give me all your bytecodes.");
		nx_display_end_line ();
		
		nx_usb_read (buffer, NX_USB_PACKET_SIZE);
		while (usb == 0) {
			if ((pos = nx_usb_data_read ()) >= 8) {
				if ((usb = valid_tbc_header (buffer))) {
					tbc = (BYTE *) mem_pool;
					tbc_length = pos + usb;
					for (i = 0; i < pos; ++i) {
						tbc[i] = buffer[i];
					}
				}
			} else {
				nx_systick_wait_ms (200);
			}
		}

		nx_display_string ("Got TEncode header.");
		nx_display_end_line ();
		nx_display_uint (usb);
		nx_display_string (" bytes remaining");
		nx_display_end_line ();

		while (usb > 0) {
			U32 tmp;

			if (usb >= NX_USB_PACKET_SIZE) {
				nx_usb_read (buffer, NX_USB_PACKET_SIZE);
			} else {
				nx_usb_read (buffer, usb);
			}

			while (!(tmp = nx_usb_data_read ())) {
				nx_display_uint (usb);
				nx_display_string (" bytes remaining");
				nx_display_end_line ();
				nx_systick_wait_ms (200);
			}

			for (i = 0; i < tmp; ++i) {
				tbc[pos++] = buffer[i];
				usb--;
			}
		}

		tlsf_init_memory_pool (NX_USERSPACE_SIZE, 
				(void *)(((U8 *) mem_pool) + tbc_length));

		tvm_ectx_init (&tvm, &context);
		context.mem_pool = mem_pool;
		context.get_time = nxt_get_time;
		context.modify_sync_flags = nxt_modify_sync_flags;
		//context.sffi_table = sffi_table;
		//context.sffi_table_length = sffi_table_length;

		if (load_context_with_tbc (&context, tbc, tbc_length) != 0) {
			/* FIXME: display program load failed */
			continue;
		}

		while (running) {
			int ret = tvm_run (&context);
			switch (ret) {
				case ECTX_PREEMPT:
				case ECTX_TIME_SLICE: {
					/* Safe to continue. */
					break;
				}
				case ECTX_SLEEP: {
					WORD next = context.tnext;
					WORD now = nxt_get_time (&context);
					while (TIME_AFTER (next, now)) {
						nx_systick_wait_ms (next - now);
						now = nxt_get_time (&context);
					}
					break;
				}
				case ECTX_INTERRUPT: {
					//clear_pending_interrupts ();
					break;
				}
				case ECTX_EMPTY: {
					//if (!waiting_on_interrupts ()) {
						//terminate("deadlock", NULL);
					//}
					break;
				}
				case ECTX_SHUTDOWN: {
					//terminate("end of program", NULL);
					running = 0;
				}
				default: {
					//terminate("error status ", &ret);
					running = 0;
				}
			}
		}
	}

	/* NOTREACHED */
}
