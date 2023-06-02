#ifndef _TIMERS_H_
#define _TIMERS_H_

#if 0
struct ak39_timer_plat_data {
	int which_timer;
	int hz;
	void *data;
	timer_handler handler;
};

#endif

typedef int (*timer_handler) (void *data);
void * ak39_timer_probe(int which_timer);
int ak39_timer_remove(void *priv);
int ak39_timer_start(timer_handler handler, void *data, void *priv, int hz);
int ak39_timer_stop(void *priv);

#endif
