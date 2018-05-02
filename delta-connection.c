#include <connection.h>

#include "delta-connection.h"

void delta_connection_new(PurpleConnection *pc)
{
	DeltaConnectionData *conn;

	g_assert(purple_connection_get_protocol_data(pc) == NULL);

	conn = g_new0(DeltaConnectionData, 1);
	conn->pc = pc;

	purple_connection_set_protocol_data(pc, conn);
}


void delta_connection_free(PurpleConnection *pc)
{
	DeltaConnectionData *conn = purple_connection_get_protocol_data(pc);

	g_assert(conn != NULL);

	purple_connection_set_protocol_data(pc, NULL);

	// TODO: free resources as they are added to DeltaConnectionData

	conn->pc = NULL;

	g_free(conn);
}
