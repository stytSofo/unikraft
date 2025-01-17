#include <uk/mutex.h>
#include <flexos/isolation.h>

void uk_mutex_init(struct uk_mutex *m)
{
	m->lock_count = 0;
	m->owner = NULL;
	flexos_gate(libuksched, uk_waitq_init, &m->wait);
}
