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
		nx_display_string ("I am the TVM.");
		nx_display_end_line ();

		nx_usb_read (buffer, NX_USB_PACKET_SIZE);
		while (usb == 0) {
			if ((pos = nx_usb_data_read ()) >= 8) {
				if ((tbc_length = valid_tbc_header (buffer))) {
					tbc_length 	+= 8;
					tbc 		= (BYTE *) mem_pool;
					for (i = 0; i < pos; ++i) {
						tbc[i] = buffer[i];
					}
					usb = tbc_length - pos;
				}
			} else {
				nx_systick_wait_ms (200);
			}
		}

		nx_display_string ("Got header (");
		nx_display_uint (tbc_length);
		nx_display_string (")");
		nx_display_end_line ();
		
		nx_display_cursor_set_pos (0, 2);
		nx_display_uint (usb);
		nx_display_string ("      ");

		while (usb > 0) {
			U32 tmp;

			if (usb >= NX_USB_PACKET_SIZE) {
				nx_usb_read (&(tbc[pos]), NX_USB_PACKET_SIZE);
			} else {
				nx_usb_read (&(tbc[pos]), usb);
			}

			while (!(tmp = nx_usb_data_read ()))
				continue;

			pos += tmp;
			usb -= tmp;
			
			nx_display_cursor_set_pos (0, 2);
			nx_display_uint (usb);
			nx_display_string ("      ");
		}
		
		nx_display_cursor_set_pos (0, 2);
		nx_display_string ("Got TBC.    ");
		nx_display_end_line ();
		nx_systick_wait_ms (200);

		tlsf_init_memory_pool (NX_USERSPACE_SIZE - tbc_length, 
				(void *)(((U8 *) mem_pool) + tbc_length));
		
		tvm_ectx_init (&tvm, &context);
		context.mem_pool = mem_pool;
		context.get_time = nxt_get_time;
		context.modify_sync_flags = nxt_modify_sync_flags;
		//context.sffi_table = sffi_table;
		//context.sffi_table_length = sffi_table_length;
		
		nx_display_string ("Loading TBC...");
		nx_display_end_line ();
		nx_systick_wait_ms (1000);
		
		if (load_context_with_tbc (&context, tbc, tbc_length) != 0) {
			nx_display_string ("Unable to decode TBC!");
			nx_systick_wait_ms (3000);
			continue;
		}

		nx_display_string ("Running...");
		nx_display_end_line ();
		nx_systick_wait_ms (1000);
		
		while (running) {
			int ret = tvm_run (&context);
			
			nx_display_uint (ret);
			nx_display_end_line ();
			nx_systick_wait_ms (1000);
			
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
			
			nx_display_end_line ();
			nx_display_string ("Finished       ");
			nx_display_end_line ();
			nx_systick_wait_ms (3000);
		}
	}

	/* NOTREACHED */
}
