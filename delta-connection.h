#ifndef DELTA_CONNECTION_H
#define DELTA_CONNECTION_H

#include <glib.h>

struct _PurpleConnection;

typedef struct _DeltaConnectionData {
	struct _PurpleConnection *pc;
} DeltaConnectionData;

void delta_connection_new(struct _PurpleConnection *pc);
void delta_connection_free(struct _PurpleConnection *pc);

#endif

