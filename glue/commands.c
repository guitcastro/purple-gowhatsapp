#include "gowhatsapp.h"
#include "libwhatsmeow.h"

/*
 * These functions implement some irc-style commands for advanced use in
 * protocol bridges like Spectrum.
 *
 * This is a custom feature requested by https://github.com/theassemblerguy.
 */

static const char *command_string_presence = "?presence";

enum gowhatsapp_command
is_command(const char *message)
{
    if (message == NULL || message[0] != '?') {
        return GOWHATSAPP_COMMAND_NONE;
    }
    if (g_str_has_prefix(message, "?versions")) {
        return GOWHATSAPP_COMMAND_VERSIONS;
    }
    if (g_str_has_prefix(message, "?contacts")) {
        return GOWHATSAPP_COMMAND_CONTACTS;
    }
    if (g_str_has_prefix(message, "?participants") || g_str_has_prefix(message, "?members")) {
        return GOWHATSAPP_COMMAND_PARTICIPANTS;
    }
    if (g_str_has_prefix(message, command_string_presence)) {
        return GOWHATSAPP_COMMAND_PRESENCE;
    }
    if (g_str_has_prefix(message, "?logout")) {
        return GOWHATSAPP_COMMAND_LOGOUT;
    }
    return GOWHATSAPP_COMMAND_NONE;
}

int
execute_command(PurpleConnection *pc,
                const gchar *message,
                G_GNUC_UNUSED const gchar *who,
                G_GNUC_UNUSED PurpleConversation *conv)
{
    PurpleAccount *account = purple_connection_get_account(pc);

    switch (is_command(message)) {
        case GOWHATSAPP_COMMAND_CONTACTS:
            gowhatsapp_go_get_contacts(account, TRUE);
            return 0;

        case GOWHATSAPP_COMMAND_LOGOUT:
            gowhatsapp_go_logout(account);
            return 0;

        case GOWHATSAPP_COMMAND_VERSIONS:
            /* TODO(libpurple-3): rebuild the "?versions" reply. The libpurple 2
             * implementation reached for purple_plugin_get_version /
             * purple_find_prpl / purple_core_get_ui_info, none of which exist in
             * libpurple 3 (plugins now live under gplugin and UI info is queried
             * differently). Writing the reply back via purple_conversation_write
             * also needs rework against the new PurpleMessage API. */
            g_log(GOWHATSAPP_NAME, G_LOG_LEVEL_MESSAGE,
                  "?versions is not implemented in the libpurple 3 build yet.");
            return -1;

        case GOWHATSAPP_COMMAND_PARTICIPANTS:
            /* TODO(libpurple-3): blocked on glue/groups.c
             * (gowhatsapp_chat_set_participants) being ported to the
             * PurpleConversationMembers API. */
            g_log(GOWHATSAPP_NAME, G_LOG_LEVEL_MESSAGE,
                  "?participants is not implemented in the libpurple 3 build yet.");
            return -1;

        case GOWHATSAPP_COMMAND_PRESENCE:
            /* TODO(libpurple-3): blocked on glue/presence.c being ported to
             * PurplePresence / PurpleSavedPresence. */
            g_log(GOWHATSAPP_NAME, G_LOG_LEVEL_MESSAGE,
                  "?presence is not implemented in the libpurple 3 build yet.");
            return -1;

        case GOWHATSAPP_COMMAND_NONE:
        default:
            return -1;
    }
}
