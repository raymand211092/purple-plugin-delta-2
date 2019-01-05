#include <connection.h>
#include <eventloop.h>
#include <util.h>

#include <string.h>

#include <curl/curl.h>
#include <curl/easy.h>

#include "delta-connection.h"
#include "libdelta.h"
#include "util.h"

void delta_recv_im(DeltaConnectionData *conn, uint32_t msg_id);

void
_transpose_config(dc_context_t *mailbox, PurpleAccount *acct)
{
	const char *addr = acct->username;
	const char *display = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_DISPLAY_NAME, NULL);

	const char *imap_host = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_IMAP_SERVER_HOST, NULL);
	const char *imap_user = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_IMAP_USER, NULL);
	const char *imap_pass = purple_account_get_password(acct);
	const char *imap_port = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_IMAP_SERVER_PORT, DEFAULT_IMAP_PORT);

	const char *smtp_host = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_SMTP_SERVER_HOST, NULL);
	const char *smtp_user = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_SMTP_USER, NULL);
	const char *smtp_pass = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_SMTP_PASS, NULL);
	const char *smtp_port = purple_account_get_string(acct, PLUGIN_ACCOUNT_OPT_SMTP_SERVER_PORT, DEFAULT_SMTP_PORT);

	dc_set_config(mailbox, PLUGIN_ACCOUNT_OPT_ADDR, addr);
	dc_set_config(mailbox, PLUGIN_ACCOUNT_OPT_DISPLAY_NAME, display);

	dc_set_config(mailbox, PLUGIN_ACCOUNT_OPT_IMAP_SERVER_HOST, imap_host);
	dc_set_config(mailbox, PLUGIN_ACCOUNT_OPT_IMAP_USER, imap_user);
	dc_set_config(mailbox, PLUGIN_ACCOUNT_OPT_IMAP_PASS, imap_pass);
	dc_set_config(mailbox, PLUGIN_ACCOUNT_OPT_IMAP_SERVER_PORT, imap_port);

	dc_set_config(mailbox, PLUGIN_ACCOUNT_OPT_SMTP_SERVER_HOST, smtp_host);
	dc_set_config(mailbox, PLUGIN_ACCOUNT_OPT_SMTP_USER, smtp_user);
	dc_set_config(mailbox, PLUGIN_ACCOUNT_OPT_SMTP_PASS, smtp_pass);
	dc_set_config(mailbox, PLUGIN_ACCOUNT_OPT_SMTP_SERVER_PORT, smtp_port);
}

// This and WriteMemoryCallback are "borrowed" from https://curl.haxx.se/libcurl/c/getinmemory.html
struct MemoryStruct {
	char *memory;
	size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

uintptr_t
_http_get(const char *url)
{
	long status = 0;
	uintptr_t out = 0;
	CURL *curl = curl_easy_init();
	CURLcode res;

	if (curl == NULL) {
		return 0;
	}

	struct MemoryStruct chunk;
	chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("Failed to GET %s: %s\n", url, curl_easy_strerror(res));
		goto err;
	}

	res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
	if (res != CURLE_OK) {
		printf("Failed to read response code for %s: %s\n", url, curl_easy_strerror(res));
		goto err;
	}

	if (status < 200 || status > 299) {
		printf("Non-success HTTP response code for %s: %lu\n", url, status);
		goto err;
	}

	out = (uintptr_t)chunk.memory;

err:
	curl_easy_cleanup(curl);

	// Don't free chunk.memory - that will be done by deltachat-core

	return out;
}

typedef struct {
	DeltaConnectionData *conn;
	uint32_t msg_id;
} ProcessRequest;

gboolean
delta_process(void *data)
{
	ProcessRequest *pr = (ProcessRequest *)data;
	g_assert(pr->conn != NULL);

	delta_recv_im(pr->conn, pr->msg_id);
	free(data);

	return FALSE;
}

void
delta_fresh_messages(DeltaConnectionData *conn, dc_context_t *mailbox)
{
	g_assert(conn != NULL);
	g_assert(mailbox != NULL);

	// Spot any messages received while offline
	dc_array_t *fresh_msgs = dc_get_fresh_msgs(mailbox);
	size_t fresh_count = dc_array_get_cnt(fresh_msgs);

	printf("*** fresh_count: %zu\n", fresh_count);

	for(size_t i = 0; i < fresh_count; i++) {
		ProcessRequest *pr = g_malloc(sizeof(ProcessRequest));
		g_assert(pr != NULL);

		pr->conn = conn;
		pr->msg_id = dc_array_get_id(fresh_msgs, i);

		purple_timeout_add(0, delta_process, pr);
	}

	free(fresh_msgs);

	return;
}

// Do not call any libpurple functions in here, as it is not thread-safe and
// events may be dispatched from any delta thread. Use
// purple_timeout_add(0, callback, data) to run on the main thread instead
uintptr_t
my_delta_handler(dc_context_t* mailbox, int event, uintptr_t data1, uintptr_t data2)
{
	DeltaConnectionData *conn = (DeltaConnectionData *)dc_get_userdata(mailbox);
	g_assert(conn != NULL);

	ProcessRequest *pr;
	uintptr_t out = 0;

	printf("my_delta_handler(mailbox, %d, %lu, %lu)\n", event, data1, data2);

	switch (event) {
	case DC_EVENT_INFO:
		printf("INFO: %s\n", (char *)data2);
		break;
	case DC_EVENT_WARNING:
		printf("WARNING: %s\n", (char *)data2);
		break;
	case DC_EVENT_ERROR:
		printf("ERROR: %d: %s\n", (int)data1, (char *)data2);
		break;

	case DC_EVENT_MSGS_CHANGED:
		delta_fresh_messages(conn, mailbox);
		break;

	case DC_EVENT_INCOMING_MSG:
		// data1 is chat_id, which we don't seem to need yet.
		// TODO: It may be needed for group chats
		pr = g_malloc(sizeof(ProcessRequest));
		g_assert(pr != NULL);
		pr->conn = conn;
		pr->msg_id = (uint32_t)data2;
		purple_timeout_add(0, delta_process, pr);
		break;

	// These are all to do with sending & receiving messages. The real meat of
	// the event loop
	case DC_EVENT_MSG_DELIVERED:
	case DC_EVENT_MSG_READ:
	case DC_EVENT_CHAT_MODIFIED:
	case DC_EVENT_CONTACTS_CHANGED:
		debug("TODO!\n");
		break;

	case DC_EVENT_CONFIGURE_PROGRESS:
		purple_connection_update_progress(conn->pc, "Connecting...", (int)data1, MAX_DELTA_CONFIGURE);
		break;
	case DC_EVENT_HTTP_GET:
		printf("HTTP GET requested: %s\n", (char *)data1);
		out = _http_get((char *)data1);
		break;
	case DC_EVENT_IS_OFFLINE:
		if ( conn->pc == NULL || !PURPLE_CONNECTION_IS_CONNECTED(conn->pc) ) {
			debug("Telling Delta we are offline\n");
			out = 1;
		} else {
			debug("Telling Delta we are online\n");
		}
		break;
	case DC_EVENT_GET_STRING:
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
		dc_stop_ongoing_process(conn->mailbox);
		dc_close(conn->mailbox);
		dc_context_unref(conn->mailbox);
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
	dc_context_t *mailbox = dc_context_new(my_delta_handler, conn, NULL);

	g_snprintf(
		dbname, 1024, "%s%sdelta_db-%s",
		purple_user_dir(), G_DIR_SEPARATOR_S, acct->username
	);

	if (!dc_open(mailbox, dbname, NULL)) {
		debug("dc_open returned false...?\n");
	}

	conn->mailbox = mailbox;

	_transpose_config(mailbox, acct);

	purple_connection_set_state(pc, PURPLE_CONNECTING);
	purple_connection_update_progress(pc, "Connecting...", 1, MAX_DELTA_CONFIGURE);

	if (!dc_is_configured(mailbox)) {
		dc_configure(mailbox);
	}

// FIXME: ensure this is set by the connection handler
//	purple_connection_set_state(pc, PURPLE_CONNECTED);

	return;
}

int
delta_send_im(PurpleConnection *pc, const char *who, const char *message, PurpleMessageFlags flags)
{
	UNUSED(flags);

	DeltaConnectionData *conn = (DeltaConnectionData *)purple_connection_get_protocol_data(pc);
	g_assert(conn != NULL);

	dc_context_t *mailbox = conn->mailbox;
	g_assert(mailbox != NULL);

	uint32_t contact_id = dc_create_contact(mailbox, NULL, who);
	uint32_t chat_id = dc_create_chat_by_contact_id(mailbox, contact_id);

	dc_send_text_msg(mailbox, chat_id, message);
	return 1; // success; echo the message to the chat window
}

void
delta_recv_im(DeltaConnectionData *conn, uint32_t msg_id)
{
	dc_context_t *mailbox = conn->mailbox;
	g_assert(mailbox != NULL);

	PurpleConnection *pc = conn->pc;
	g_assert(pc != NULL);

	dc_msg_t* msg = dc_get_msg(mailbox, msg_id);

	time_t timestamp = dc_msg_get_timestamp(msg);
	char *text = dc_msg_get_text(msg);
	uint32_t contact_id  = dc_msg_get_from_id(msg);

	dc_contact_t *contact = dc_get_contact(mailbox, contact_id);
	if (contact == NULL) {
		debug("Unknown contact! FIXME!\n");
		goto out;
	}

	char *who = dc_contact_get_addr(contact);

	serv_got_im(pc, who, text, PURPLE_MESSAGE_RECV | PURPLE_MESSAGE_RAW, timestamp);

	dc_markseen_msgs(mailbox, &msg_id, 1);
	g_free(who);
out:
	g_free(text);
	dc_msg_unref(msg);
}
