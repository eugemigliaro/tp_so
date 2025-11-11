// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <time.h>
#include <interrupts.h>
#include <fonts.h>
#include <cursor.h>
#include <process.h>
#include <scheduler.h>
#include <queueADT.h>
#include <memoryManager.h>

static unsigned long ticks = 0;

typedef struct {
	int32_t pid;
	unsigned long wake_time;
} SleepingProcess;

static queue_t *sleeping_queue = NULL;

static void init_sleeping_queue(void) {
	if (sleeping_queue == NULL) {
		sleeping_queue = queue_create();
	}
}

void timer_handler() {
	ticks++;
	toggleCursor();

	if (sleeping_queue != NULL && !queue_is_empty(sleeping_queue)) {
		queue_iterator_t iter = queue_iter(sleeping_queue);
		
		while (queue_iter_has_next(&iter)) {
			SleepingProcess *process = (SleepingProcess *)queue_iter_next(&iter);
			
			if (process != NULL && process->wake_time <= ticks) {
				process_t *proc = process_lookup(process->pid);
				if (proc != NULL) {
					process_unblock(proc);
				}
			}
		}
		
		iter = queue_iter(sleeping_queue);
		while (queue_iter_has_next(&iter)) {
			SleepingProcess *process = (SleepingProcess *)queue_iter_next(&iter);
			if (process != NULL && process->wake_time <= ticks) {
				queue_remove(sleeping_queue, process);
				mem_free(process);
			}
		}
	}
}

int ticks_elapsed() {
	return ticks;
}

int seconds_elapsed() {
	return ticks / SECONDS_TO_TICKS;
}

void sleepTicks(uint64_t sleep_t) {
	init_sleeping_queue();
	
	process_t *current = scheduler_current();
	if (current == NULL) {
		return; 
	}

	SleepingProcess *process = (SleepingProcess *)mem_alloc(sizeof(SleepingProcess));
	if (process == NULL) {
		return;
	}

	process->pid = current->pid;
	process->wake_time = ticks + sleep_t;
	
	if (!queue_push(sleeping_queue, process)) {
		mem_free(process);
		return;
	}

	process_block(current);
	
	_force_scheduler_interrupt();
}

void sleep(int seconds) {
	sleepTicks(seconds * SECONDS_TO_TICKS);
	return;
}
