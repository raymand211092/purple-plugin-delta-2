#ifndef LIBDELTA_H
#define LIBDELTA_H

#define PLUGIN_ID "prpl-delta"

#define PLUGIN_CHAT_INFO_CHAT_ID "chat_id"

#define DELTA_PROTOCOL_OPTS \
	OPT_PROTO_UNIQUE_CHATNAME | \
	OPT_PROTO_CHAT_TOPIC | \
	OPT_PROTO_IM_IMAGE

#define DEFAULT_SMTP_PORT 0
#define DEFAULT_IMAP_PORT 0


// These two will instead be the pidgin "username" and "password" options that
// I can't seem to get rid of.
#define PLUGIN_ACCOUNT_OPT_ADDR             "addr"
#define PLUGIN_ACCOUNT_OPT_IMAP_PASS        "mail_pw"

// Share the remaining keys between purple and delta
#define PLUGIN_ACCOUNT_OPT_DISPLAY_NAME     "displayname"

#define PLUGIN_ACCOUNT_OPT_IMAP_SERVER_HOST "mail_server"
#define PLUGIN_ACCOUNT_OPT_IMAP_SERVER_PORT "mail_port"
#define PLUGIN_ACCOUNT_OPT_IMAP_USER        "mail_user"

#define PLUGIN_ACCOUNT_OPT_SMTP_SERVER_HOST "send_server"
#define PLUGIN_ACCOUNT_OPT_SMTP_SERVER_PORT "send_port"
#define PLUGIN_ACCOUNT_OPT_SMTP_USER        "send_user"
#define PLUGIN_ACCOUNT_OPT_SMTP_PASS        "send_pw"

#endif
