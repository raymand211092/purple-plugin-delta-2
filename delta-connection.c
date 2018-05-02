#include <connection.h>
#include <util.h>

#include <deltachat/mrmailbox.h>
#include <libsoup/soup.h>

#include "delta-connection.h"
#include "libdelta.h"
#include "util.h"

void delta_recv_im(DeltaConnectionData *conn, uint32_t chat_id, uint32_t msg_id);

void
_transpose_config(mrmailbox_t *mailbox, PurpleAccount *acct)
{
	const char *addr = acct->username;
	const char *display = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_DISPLAY_NAME, NULL);

	const char *imap_host = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_IMAP_SERVER_HOST, NULL);
	const char *imap_user = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_IMAP_USER, NULL);
	const char *imap_pass = purple_account_get_password(acct);
	int imap_port = purple_account_get_int(acct, PLUGIN_ACCOUNT_OPT_IMAP_SERVER_PORT, DEFAULT_IMAP_PORT);

	const char *smtp_host = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_SMTP_SERVER_HOST, NULL);
	const char *smtp_user = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_SMTP_USER, NULL);
	const char *smtp_pass = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_SMTP_PASS, NULL);
	int smtp_port = purple_account_get_int(acct, PLUGIN_ACCOUNT_OPT_SMTP_SERVER_PORT, DEFAULT_SMTP_PORT);

	mrmailbox_set_config(mailbox, PLUGIN_ACCOUNT_OPT_ADDR, addr);
	mrmailbox_set_config(mailbox, PLUGIN_ACCOUNT_OPT_DISPLAY_NAME, display);

	mrmailbox_set_config(mailbox, PLUGIN_ACCOUNT_OPT_IMAP_SERVER_HOST, imap_host);
	mrmailbox_set_config(mailbox, PLUGIN_ACCOUNT_OPT_IMAP_USER, imap_user);
	mrmailbox_set_config(mailbox, PLUGIN_ACCOUNT_OPT_IMAP_PASS, imap_pass);
	mrmailbox_set_config_int(mailbox, PLUGIN_ACCOUNT_OPT_IMAP_SERVER_PORT, imap_port);

	mrmailbox_set_config(mailbox, PLUGIN_ACCOUNT_OPT_SMTP_SERVER_HOST, smtp_host);
	mrmailbox_set_config(mailbox, PLUGIN_ACCOUNT_OPT_SMTP_USER, smtp_user);
	mrmailbox_set_config(mailbox, PLUGIN_ACCOUNT_OPT_SMTP_PASS, smtp_pass);
	mrmailbox_set_config_int(mailbox, PLUGIN_ACCOUNT_OPT_SMTP_SERVER_PORT, smtp_port);
}

uintptr_t
_http_get(const char *url)
{
	// FIXME: we could keep a soup session around for more than a single request
	uintptr_t out = 0;
	guint status;
	SoupSession *session = soup_session_new();
	SoupMessage *msg = soup_message_new("GET", url);

	status = soup_session_send_message(session, msg);

	if (status >= 200 && status < 300) {
		out = (uintptr_t)msg->response_body->data;
	}

//	g_free(msg); // FIXME: huge memory leak
//	g_free(session);

	return out;
}

uintptr_t
my_delta_handler(mrmailbox_t* mailbox, int event, uintptr_t data1, uintptr_t data2)
{
	DeltaConnectionData *conn = (DeltaConnectionData *)mrmailbox_get_userdata(mailbox);
	g_assert(conn != NULL);

	uintptr_t out = 0;

	switch (event) {
	case MR_EVENT_INFO:
		printf("INFO: %s\n", (char *)data2);
		break;
	case MR_EVENT_WARNING:
		printf("WARNING: %s\n", (char *)data2);
		break;
	case MR_EVENT_ERROR:
		printf("ERROR: %d: %s\n", (int)data1, (char *)data2);
		break;

	case MR_EVENT_MSGS_CHANGED:
		debug("TODO: received MR_EVENT_MSGS_CHANGED");
		break;

	case MR_EVENT_INCOMING_MSG:
		delta_recv_im(conn, (uint32_t)data1, (uint32_t)data2);
		break;

	// These are all to do with sending & receiving messages. The real meat of
	// the event loop
	case MR_EVENT_MSG_DELIVERED:
	case MR_EVENT_MSG_READ:
	case MR_EVENT_CHAT_MODIFIED:
	case MR_EVENT_CONTACTS_CHANGED:
		debug("TODO!\n");
		break;

	case MR_EVENT_CONFIGURE_PROGRESS:
		purple_connection_update_progress(conn->pc, "Connecting...", (int)data1, MAX_DELTA_CONFIGURE);
		break;
	case MR_EVENT_HTTP_GET:
		printf("HTTP GET requested: %s\n", (char *)data1);
		out = _http_get((char *)data1);
		break;
	case MR_EVENT_IS_OFFLINE:
		debug("TODO: MR_EVENT_IS_OFFLINE handling. Returning online for now\n");
		break;
	case MR_EVENT_GET_STRING:
	case MR_EVENT_GET_QUANTITY_STRING:
	case MR_EVENT_WAKE_LOCK:
		break;
	default:
		printf("Unknown event: %d\n", event);
	}

	return out;
}

void
delta_connection_new(PurpleConnection *pc)
{
	DeltaConnectionData *conn = NULL;

	g_assert(purple_connection_get_protocol_data(pc) == NULL);

	conn = g_new0(DeltaConnectionData, 1);
	conn->pc = pc;
	purple_connection_set_protocol_data(pc, conn);
}

void
delta_connection_free(PurpleConnection *pc)
{
	DeltaConnectionData *conn = purple_connection_get_protocol_data(pc);

	g_assert(conn != NULL);

	purple_connection_set_protocol_data(pc, NULL);

	if (conn->mailbox != NULL) {
		mrmailbox_stop_ongoing_process(conn->mailbox);
		mrmailbox_disconnect(conn->mailbox);
		mrmailbox_close(conn->mailbox);
		mrmailbox_unref(conn->mailbox);
	}

	// TODO: free resources as they are added to DeltaConnectionData
	conn->pc = NULL;
	conn->mailbox = NULL;

	g_free(conn);
}

void
delta_connection_start_login(PurpleConnection *pc)
{
	char dbname[1024];
	PurpleAccount *acct = pc->account;
	DeltaConnectionData *conn = purple_connection_get_protocol_data(pc);
	mrmailbox_t *mailbox = mrmailbox_new(my_delta_handler, conn, NULL);

	g_snprintf(
		dbname, 1024, "%s%sdelta_db-%s",
		purple_user_dir(), G_DIR_SEPARATOR_S, acct->username
	);


	if (!mrmailbox_open(mailbox, dbname, NULL)) {
		debug("mrmailbox_open returned false...?");
	}

	conn->mailbox = mailbox;

	_transpose_config(mailbox, acct);

	purple_connection_set_state(pc, PURPLE_CONNECTING);
	purple_connection_update_progress(pc, "Connecting...", 1, MAX_DELTA_CONFIGURE);

	if (mrmailbox_is_configured(mailbox)) {
		mrmailbox_connect(mailbox);
	} else if (!mrmailbox_configure_and_connect(mailbox)) {
		char *info = mrmailbox_get_info(mailbox);
		debug(info);
		g_free(info);

		purple_connection_error(pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR);
		purple_connection_set_state(pc, PURPLE_DISCONNECTED);
		return;
	}

	purple_connection_set_state(pc, PURPLE_CONNECTED);
	return;
}

int
delta_send_im(PurpleConnection *pc, const char *who, const char *message, PurpleMessageFlags flags)
{
	UNUSED(flags);

	DeltaConnectionData *conn = (DeltaConnectionData *)purple_connection_get_protocol_data(pc);
	g_assert(conn != NULL);

	mrmailbox_t *mailbox = conn->mailbox;
	g_assert(mailbox != NULL);

	uint32_t contact_id = mrmailbox_create_contact(mailbox, NULL, who);
	uint32_t chat_id = mrmailbox_create_chat_by_contact_id(mailbox, contact_id);

	mrmailbox_send_text_msg(mailbox, chat_id, message);
	return 1; // success; echo the message to the chat window
}

void
delta_recv_im(DeltaConnectionData *conn, uint32_t chat_id, uint32_t msg_id)
{
	mrmailbox_t *mailbox = conn->mailbox;
	g_assert(mailbox != NULL);

	PurpleConnection *pc = conn->pc;
	g_assert(pc != NULL);

	mrmsg_t* msg = mrmailbox_get_msg(mailbox, msg_id);

	time_t timestamp = mrmsg_get_timestamp(msg);
	char *text = mrmsg_get_text(msg);
	uint32_t contact_id  = mrmsg_get_from_id(msg);

	mrcontact_t *contact = mrmailbox_get_contact(mailbox, contact_id);
	if (contact == NULL) {
		debug("Unknown contact! FIXME!");
		goto out;
	}

	char *who = mrcontact_get_addr(contact);

	// TODO: send this to the IM window instead
	printf("who: %s, text: %s\n", who, text);
	printf("message %d.%d: %s\n", chat_id, msg_id, text);
	serv_got_im(pc, who, text, PURPLE_MESSAGE_RECV | PURPLE_MESSAGE_RAW, timestamp);

	mrmailbox_markseen_msgs(mailbox, &msg_id, 1);
	g_free(who);
out:
	g_free(text);
	mrmsg_unref(msg);
}
