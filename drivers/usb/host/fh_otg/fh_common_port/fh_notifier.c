#ifdef FH_NOTIFYLIB

#include "fh_notifier.h"
#include "fh_list.h"

typedef struct fh_observer {
	void *observer;
	fh_notifier_callback_t callback;
	void *data;
	char *notification;
	FH_CIRCLEQ_ENTRY(fh_observer) list_entry;
} observer_t;

FH_CIRCLEQ_HEAD(observer_queue, fh_observer);

typedef struct fh_notifier {
	void *mem_ctx;
	void *object;
	struct observer_queue observers;
	FH_CIRCLEQ_ENTRY(fh_notifier) list_entry;
} notifier_t;

FH_CIRCLEQ_HEAD(notifier_queue, fh_notifier);

typedef struct manager {
	void *mem_ctx;
	void *wkq_ctx;
	fh_workq_t *wq;
//	fh_mutex_t *mutex;
	struct notifier_queue notifiers;
} manager_t;

static manager_t *manager = NULL;

static int create_manager(void *mem_ctx, void *wkq_ctx)
{
	manager = fh_alloc(mem_ctx, sizeof(manager_t));
	if (!manager) {
		return -FH_E_NO_MEMORY;
	}

	FH_CIRCLEQ_INIT(&manager->notifiers);

	manager->wq = fh_workq_alloc(wkq_ctx, "FH Notification WorkQ");
	if (!manager->wq) {
		return -FH_E_NO_MEMORY;
	}

	return 0;
}

static void free_manager(void)
{
	fh_workq_free(manager->wq);

	/* All notifiers must have unregistered themselves before this module
	 * can be removed.  Hitting this assertion indicates a programmer
	 * error. */
	FH_ASSERT(FH_CIRCLEQ_EMPTY(&manager->notifiers),
		   "Notification manager being freed before all notifiers have been removed");
	fh_free(manager->mem_ctx, manager);
}

#ifdef DEBUG
static void dump_manager(void)
{
	notifier_t *n;
	observer_t *o;

	FH_ASSERT(manager, "Notification manager not found");

	FH_DEBUG("List of all notifiers and observers:\n");
	FH_CIRCLEQ_FOREACH(n, &manager->notifiers, list_entry) {
		FH_DEBUG("Notifier %p has observers:\n", n->object);
		FH_CIRCLEQ_FOREACH(o, &n->observers, list_entry) {
			FH_DEBUG("    %p watching %s\n", o->observer, o->notification);
		}
	}
}
#else
#define dump_manager(...)
#endif

static observer_t *alloc_observer(void *mem_ctx, void *observer, char *notification,
				  fh_notifier_callback_t callback, void *data)
{
	observer_t *new_observer = fh_alloc(mem_ctx, sizeof(observer_t));

	if (!new_observer) {
		return NULL;
	}

	FH_CIRCLEQ_INIT_ENTRY(new_observer, list_entry);
	new_observer->observer = observer;
	new_observer->notification = notification;
	new_observer->callback = callback;
	new_observer->data = data;
	return new_observer;
}

static void free_observer(void *mem_ctx, observer_t *observer)
{
	fh_free(mem_ctx, observer);
}

static notifier_t *alloc_notifier(void *mem_ctx, void *object)
{
	notifier_t *notifier;

	if (!object) {
		return NULL;
	}

	notifier = fh_alloc(mem_ctx, sizeof(notifier_t));
	if (!notifier) {
		return NULL;
	}

	FH_CIRCLEQ_INIT(&notifier->observers);
	FH_CIRCLEQ_INIT_ENTRY(notifier, list_entry);

	notifier->mem_ctx = mem_ctx;
	notifier->object = object;
	return notifier;
}

static void free_notifier(notifier_t *notifier)
{
	observer_t *observer;

	FH_CIRCLEQ_FOREACH(observer, &notifier->observers, list_entry) {
		free_observer(notifier->mem_ctx, observer);
	}

	fh_free(notifier->mem_ctx, notifier);
}

static notifier_t *find_notifier(void *object)
{
	notifier_t *notifier;

	FH_ASSERT(manager, "Notification manager not found");

	if (!object) {
		return NULL;
	}

	FH_CIRCLEQ_FOREACH(notifier, &manager->notifiers, list_entry) {
		if (notifier->object == object) {
			return notifier;
		}
	}

	return NULL;
}

int fh_alloc_notification_manager(void *mem_ctx, void *wkq_ctx)
{
	return create_manager(mem_ctx, wkq_ctx);
}

void fh_free_notification_manager(void)
{
	free_manager();
}

fh_notifier_t *fh_register_notifier(void *mem_ctx, void *object)
{
	notifier_t *notifier;

	FH_ASSERT(manager, "Notification manager not found");

	notifier = find_notifier(object);
	if (notifier) {
		FH_ERROR("Notifier %p is already registered\n", object);
		return NULL;
	}

	notifier = alloc_notifier(mem_ctx, object);
	if (!notifier) {
		return NULL;
	}

	FH_CIRCLEQ_INSERT_TAIL(&manager->notifiers, notifier, list_entry);

	FH_INFO("Notifier %p registered", object);
	dump_manager();

	return notifier;
}

void fh_unregister_notifier(fh_notifier_t *notifier)
{
	FH_ASSERT(manager, "Notification manager not found");

	if (!FH_CIRCLEQ_EMPTY(&notifier->observers)) {
		observer_t *o;

		FH_ERROR("Notifier %p has active observers when removing\n", notifier->object);
		FH_CIRCLEQ_FOREACH(o, &notifier->observers, list_entry) {
			FH_DEBUG("    %p watching %s\n", o->observer, o->notification);
		}

		FH_ASSERT(FH_CIRCLEQ_EMPTY(&notifier->observers),
			   "Notifier %p has active observers when removing", notifier);
	}

	FH_CIRCLEQ_REMOVE_INIT(&manager->notifiers, notifier, list_entry);
	free_notifier(notifier);

	FH_INFO("Notifier unregistered");
	dump_manager();
}

/* Add an observer to observe the notifier for a particular state, event, or notification. */
int fh_add_observer(void *observer, void *object, char *notification,
		     fh_notifier_callback_t callback, void *data)
{
	notifier_t *notifier = find_notifier(object);
	observer_t *new_observer;

	if (!notifier) {
		FH_ERROR("Notifier %p is not found when adding observer\n", object);
		return -FH_E_INVALID;
	}

	new_observer = alloc_observer(notifier->mem_ctx, observer, notification, callback, data);
	if (!new_observer) {
		return -FH_E_NO_MEMORY;
	}

	FH_CIRCLEQ_INSERT_TAIL(&notifier->observers, new_observer, list_entry);

	FH_INFO("Added observer %p to notifier %p observing notification %s, callback=%p, data=%p",
		 observer, object, notification, callback, data);

	dump_manager();
	return 0;
}

int fh_remove_observer(void *observer)
{
	notifier_t *n;

	FH_ASSERT(manager, "Notification manager not found");

	FH_CIRCLEQ_FOREACH(n, &manager->notifiers, list_entry) {
		observer_t *o;
		observer_t *o2;

		FH_CIRCLEQ_FOREACH_SAFE(o, o2, &n->observers, list_entry) {
			if (o->observer == observer) {
				FH_CIRCLEQ_REMOVE_INIT(&n->observers, o, list_entry);
				FH_INFO("Removing observer %p from notifier %p watching notification %s:",
					 o->observer, n->object, o->notification);
				free_observer(n->mem_ctx, o);
			}
		}
	}

	dump_manager();
	return 0;
}

typedef struct callback_data {
	void *mem_ctx;
	fh_notifier_callback_t cb;
	void *observer;
	void *data;
	void *object;
	char *notification;
	void *notification_data;
} cb_data_t;

static void cb_task(void *data)
{
	cb_data_t *cb = (cb_data_t *)data;

	cb->cb(cb->object, cb->notification, cb->observer, cb->notification_data, cb->data);
	fh_free(cb->mem_ctx, cb);
}

void fh_notify(fh_notifier_t *notifier, char *notification, void *notification_data)
{
	observer_t *o;

	FH_ASSERT(manager, "Notification manager not found");

	FH_CIRCLEQ_FOREACH(o, &notifier->observers, list_entry) {
		int len = FH_STRLEN(notification);

		if (FH_STRLEN(o->notification) != len) {
			continue;
		}

		if (FH_STRNCMP(o->notification, notification, len) == 0) {
			cb_data_t *cb_data = fh_alloc(notifier->mem_ctx, sizeof(cb_data_t));

			if (!cb_data) {
				FH_ERROR("Failed to allocate callback data\n");
				return;
			}

			cb_data->mem_ctx = notifier->mem_ctx;
			cb_data->cb = o->callback;
			cb_data->observer = o->observer;
			cb_data->data = o->data;
			cb_data->object = notifier->object;
			cb_data->notification = notification;
			cb_data->notification_data = notification_data;
			FH_DEBUG("Observer found %p for notification %s\n", o->observer, notification);
			FH_WORKQ_SCHEDULE(manager->wq, cb_task, cb_data,
					   "Notify callback from %p for Notification %s, to observer %p",
					   cb_data->object, notification, cb_data->observer);
		}
	}
}

#endif	/* FH_NOTIFYLIB */
