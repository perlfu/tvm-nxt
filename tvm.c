#include "tvm-nxt.h"

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
	tvm_init (&tvm);

	for (;;) {
		UWORD tbc_length = 0;
		BYTE *tbc = NULL;
		char buffer[8];
		int running = 1;

		tlsf_init_memory_pool (NX_USERSPACE_SIZE, (void *) mem_pool);

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
