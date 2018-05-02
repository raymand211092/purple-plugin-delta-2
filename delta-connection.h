#ifndef DELTA_CONNECTION_H
#define DELTA_CONNECTION_H

#include <glib.h>

struct _PurpleConnection;
struct _mrmailbox;

typedef struct _DeltaConnectionData {
	struct _PurpleConnection *pc;
	struct _mrmailbox *mailbox;
} DeltaConnectionData;

#define MAX_DELTA_CONFIGURE 901

void delta_connection_new(struct _PurpleConnection *pc);
void delta_connection_free(struct _PurpleConnection *pc);

void delta_connection_start_login(PurpleConnection *pc);

int delta_send_im(PurpleConnection *pc, const char *who, const char *message, PurpleMessageFlags flags);

#endif

