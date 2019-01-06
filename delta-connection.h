#ifndef DELTA_CONNECTION_H
#define DELTA_CONNECTION_H

#include <glib.h>
#include <deltachat/deltachat.h>
#include <pthread.h>

struct _PurpleConnection;

typedef struct _DeltaConnectionData {
	struct _PurpleConnection *pc;
	dc_context_t *mailbox;

	// Set to 0 to convince threads to exit
	int runthreads;

	pthread_t imap_thread;
	pthread_t smtp_thread;
} DeltaConnectionData;

#define MAX_DELTA_CONFIGURE 1000

void delta_connection_new(struct _PurpleConnection *pc);
void delta_connection_free(struct _PurpleConnection *pc);

void delta_connection_start_login(PurpleConnection *pc);

int delta_send_im(PurpleConnection *pc, const char *who, const char *message, PurpleMessageFlags flags);

#endif

