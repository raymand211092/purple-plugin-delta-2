#define PURPLE_PLUGINS

#include "libdelta.h"

#include <glib.h>

// All from libpurple
#include <accountopt.h>
#include <connection.h>
#include <notify.h>
#include <plugin.h>
#include <prpl.h>
#include <version.h>

#include "delta-connection.h"
#include "util.h"

static GList *
delta_status_types(PurpleAccount *acct)
{
	UNUSED(acct);

	GList *types = NULL;

	types = g_list_append(types, purple_status_type_new(PURPLE_STATUS_OFFLINE, "Offline", NULL, TRUE));
	types = g_list_append(types, purple_status_type_new(PURPLE_STATUS_AVAILABLE, "Online", NULL, TRUE));

	return types;
}

static void
delta_login(PurpleAccount *acct)
{
	PurpleConnection *pc = purple_account_get_connection(acct);


	delta_connection_new(pc);
	delta_connection_start_login(pc);

	pc->flags |= PURPLE_CONNECTION_NO_BGCOLOR;
}

static void
delta_close(PurpleConnection *pc)
{
	// TODO: actually disconnect!
	purple_connection_set_state(pc, PURPLE_DISCONNECTED);
	delta_connection_free(pc);
}

static const char *
delta_list_icon(PurpleAccount *acct, PurpleBuddy *buddy)
{
	UNUSED(acct);
	UNUSED(buddy);

	return "delta";
}

static PurpleAccountOption *
str_opt(const char *text, const char *name, const char *def)
{
	return purple_account_option_string_new(text, name, def);
}

static PurpleAccountOption *
pwd_opt(const char *text, const char *name, const char *def)
{
	PurpleAccountOption* option = str_opt(text, name, def);
	purple_account_option_set_masked(option, TRUE);

	return option;
}

static PurpleAccountOption *
int_opt(const char *text, const char *name, int def)
{
	return purple_account_option_int_new(text, name, def);
}

static void
delta_init_plugin(PurplePlugin *plugin)
{
	PurplePluginProtocolInfo *extra = (PurplePluginProtocolInfo *)plugin->info->extra_info;
	GList *opts = NULL;

	debug("Starting up\n");

	opts = g_list_prepend(opts, str_opt("Display Name", PLUGIN_ACCOUNT_OPT_DISPLAY_NAME, NULL));

	opts = g_list_prepend(opts, str_opt("IMAP Server Host", PLUGIN_ACCOUNT_OPT_IMAP_SERVER_HOST, NULL));
	opts = g_list_prepend(opts, int_opt("IMAP Server Port", PLUGIN_ACCOUNT_OPT_IMAP_SERVER_PORT, DEFAULT_IMAP_PORT));
	opts = g_list_prepend(opts, str_opt("IMAP Username", PLUGIN_ACCOUNT_OPT_IMAP_USER, NULL));


	// These are pidgin's built-in username & password options
	// FIXME: it's not super-obvious or pleasant :/
	// opts = g_list_prepend(opts, str_opt("Email Address", PLUGIN_ACCOUNT_OPT_EMAIL_ADDRESS, ""));
	// opts = g_list_prepend(opts, pwd_opt("IMAP Password", PLUGIN_ACCOUNT_OPT_IMAP_PASS, ""));

	opts = g_list_prepend(opts, str_opt("SMTP Server Host", PLUGIN_ACCOUNT_OPT_SMTP_SERVER_HOST, NULL));
	opts = g_list_prepend(opts, int_opt("SMTP Server Port", PLUGIN_ACCOUNT_OPT_SMTP_SERVER_PORT, DEFAULT_SMTP_PORT));
	opts = g_list_prepend(opts, str_opt("SMTP Username", PLUGIN_ACCOUNT_OPT_SMTP_USER, NULL));
	opts = g_list_prepend(opts, pwd_opt("SMTP Password", PLUGIN_ACCOUNT_OPT_SMTP_PASS, NULL));

	// Not exposed: server_flags, selfstatus, e2ee_enabled
	// https://deltachat.github.io/api/classmrmailbox__t.html

	extra->protocol_options = g_list_reverse(opts);
}

static void
delta_destroy_plugin(PurplePlugin *plugin) {
	UNUSED(plugin);

	debug("Shutting down\n");
}

static gboolean
delta_offline_message(const PurpleBuddy *buddy)
{
	UNUSED(buddy);

	return TRUE;
}

static PurplePluginProtocolInfo extra_info =
{
	DELTA_PROTOCOL_OPTS,    /* options */
    NULL,               /* user_splits */
    NULL,               /* protocol_options, initialized in delta_init_plugin() */
    {   /* icon_spec, a PurpleBuddyIconSpec */
        "svg,png,jpg,gif",               /* format */
        0,                               /* min_width */
        0,                               /* min_height */
        128,                             /* max_width */
        128,                             /* max_height */
        10000,                           /* max_filesize */
        PURPLE_ICON_SCALE_DISPLAY,       /* scale_rules */
    },
    delta_list_icon,                       /* list_icon */
    NULL,                                  /* list_emblem */
    NULL,                                  /* status_text */
    NULL,                                  /* tooltip_text */
    delta_status_types,                    /* status_types */
    NULL,                                  /* blist_node_menu */
    NULL,                       /* chat_info */
    NULL,              /* chat_info_defaults */
    delta_login,                           /* login */
    delta_close,                           /* close */
    delta_send_im,                         /* send_im */
    NULL,                                  /* set_info */
    NULL,                                  /* send_typing */
    NULL,                                  /* get_info */
    NULL,                                  /* set_status */
    NULL,                                  /* set_idle */
    NULL,                                  /* change_passwd */
    NULL,                                  /* add_buddy */
    NULL,                                  /* add_buddies */
    NULL,                                  /* remove_buddy */
    NULL,                                  /* remove_buddies */
    NULL,                                  /* add_permit */
    NULL,                                  /* add_deny */
    NULL,                                  /* rem_permit */
    NULL,                                  /* rem_deny */
    NULL,                                  /* set_permit_deny */
    NULL,                  /* join_chat */
    NULL,                /* reject_chat */
    NULL,              /* get_chat_name */
    NULL,                /* chat_invite */
    NULL,                 /* chat_leave */
    NULL,                                  /* chat_whisper */
    NULL,                  /* chat_send */
    NULL,                                  /* keepalive */
    NULL,                                  /* register_user */
    NULL,                                  /* get_cb_info */
    NULL,                                  /* get_cb_away */
    NULL,                                  /* alias_buddy */
    NULL,                                  /* group_buddy */
    NULL,                                  /* rename_group */
    NULL,                                  /* buddy_free */
    NULL,                                  /* convo_closed */
    NULL,                                  /* normalize */
    NULL,                                  /* set_buddy_icon */
    NULL,                                  /* remove_group */
    NULL,           /* get_cb_real_name */
    NULL,                                  /* set_chat_topic */
    NULL,                                  /* find_blist_chat */
    NULL,                                  /* roomlist_get_list */
    NULL,                                  /* roomlist_cancel */
    NULL,                                  /* roomlist_expand_category */
    NULL,                                  /* can_receive_file */
    NULL,                                  /* send_file */
    NULL,                                  /* new_xfer */
    delta_offline_message,                 /* offline_message */
    NULL,                                  /* whiteboard_prpl_ops */
    NULL,                                  /* send_raw */
    NULL,                                  /* roomlist_room_serialize */
    NULL,                                  /* unregister_user */
    NULL,                                  /* send_attention */
    NULL,                                  /* get_attention_types */
    sizeof(PurplePluginProtocolInfo),      /* struct_size */
    NULL,                                  /* get_account_text_table */
    NULL,                                  /* initiate_media */
    NULL,                                  /* get_media_caps */
    NULL,                                  /* get_moods */
    NULL,                                  /* set_public_alias */
    NULL,                                  /* get_public_alias */
    NULL,                                  /* add_buddy_with_invite */
    NULL                                   /* add_buddies_with_invite */
};


static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL,
	NULL, // UI requirements
	0, // flags
	NULL, // dependencies
	PURPLE_PRIORITY_DEFAULT,

	PLUGIN_ID,
	"Delta Chat",
	"0.0.0",

	"Delta Chat is an email-based instant messaging solution",
	"See https://delta.chat for more information",
	"Nick Thomas <delta@ur.gs>",
	"https://delta.chat",

	NULL, // plugin_load
	NULL, // plugin_unload
	delta_destroy_plugin, // plugin_destroy

	NULL, // ui_info
	&extra_info, // extra_info
	NULL, // prefs_info
	NULL, // actions

	NULL, // reserved1
	NULL, // reserved2
	NULL, // reserved3
	NULL  // reserved4
};

PURPLE_INIT_PLUGIN(delta, delta_init_plugin, info)
