
#include "scheduler.h"

void scheduler_init(Scheduler* s) {
    // TODO: use dynamic array for coros;
    s->current_idx = -1;
    s->total_spawned = 0;
}
void scheduler_destroy(Scheduler* s) {
    // Nothing to cleanup now
}
