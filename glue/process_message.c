#include "gowhatsapp.h"
#include "constants.h"
#include "libwhatsmeow.h" // for gowhatsapp_go_get_contacts
#include "purplegowhatsappconnection.h"

#define PURPLE_GOWHATSAPP_DOMAIN (g_quark_from_static_string("purple-gowhatsapp"))

static const char *gowhatsapp_message_type_string[] = {
    FOREACH_MESSAGE_TYPE(GENERATE_STRING)
};

static gboolean
gowhatsapp_message_is_old(gowhatsapp_message_t *gwamsg)
{
    PurpleConnection *pc = purple_account_get_connection(gwamsg->account);
    WhatsappProtocolData *wpd = purple_gowhatsapp_connection_get_protocol_data(pc);

    if (wpd != NULL && wpd->connected_at_timestamp > gwamsg->timestamp) {
        PurpleAccountSettings *settings = purple_account_get_settings(gwamsg->account);
        gboolean discard = purple_account_settings_get_boolean(settings,
                                                               GOWHATSAPP_DISCARD_OLD_MESSAGES_OPTION,
                                                               FALSE);
        if (discard) {
            g_info("%s: dropping message older than connection timestamp.", GOWHATSAPP_NAME);
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * Tells the front-end the account is now ready (connected).
 *
 * In libpurple 2 this also walked the buddy list to mark everyone as
 * away, refresh profile pictures, and so on. Those side effects depend
 * on glue/blist.c and glue/presence.c which still need to be ported.
 */
static void
gowhatsapp_connection_set_online(PurpleConnection *connection)
{
    PurpleAccount *account = purple_connection_get_account(connection);
    purple_account_ready(account);

    /* TODO(libpurple-3): once glue/blist.c and glue/presence.c land, also:
     *   - mark all buddies as away (gowhatsapp_for_all_buddies + assume_buddy_away)
     *   - publish our own presence (gowhatsapp_set_presence)
     *   - request profile pictures (gowhatsapp_for_all_buddies + request_profile_picture)
     */
}

/*
 * Interprets a message received from whatsmeow. Handles login success and failure. Forwards errors.
 */
void
gowhatsapp_process_message(gowhatsapp_message_t *gwamsg)
{
    if (gwamsg->msgtype < 0 || gwamsg->msgtype >= gowhatsapp_message_type_max) {
        g_warning("%s: received invalid message type %d", GOWHATSAPP_NAME, gwamsg->msgtype);
        return;
    }

    g_info(
        "%s: received %s (subtype %d) account=%p remote=%s isGroup=%d sender=%s alias=%s isOutgoing=%d ts=%ld text=%s",
        GOWHATSAPP_NAME,
        gowhatsapp_message_type_string[gwamsg->msgtype],
        gwamsg->subtype,
        gwamsg->account,
        gwamsg->remoteJid,
        gwamsg->isGroup,
        gwamsg->senderJid,
        gwamsg->name,
        gwamsg->isOutgoing,
        gwamsg->timestamp,
        gwamsg->text);

    PurpleConnection *pc = purple_account_get_connection(gwamsg->account);
    PurpleAccountSettings *settings = purple_account_get_settings(gwamsg->account);

    if (!gwamsg->timestamp) {
        gwamsg->timestamp = time(NULL);
    }

    switch (gwamsg->msgtype) {
        case gowhatsapp_message_type_error:
            purple_account_disconnect_with_new_error(gwamsg->account,
                                                    gwamsg->text,
                                                    PURPLE_GOWHATSAPP_DOMAIN,
                                                    gwamsg->subtype,
                                                    "%s",
                                                    gwamsg->text);
            gowhatsapp_close_qrcode(gwamsg->account);
            break;
        case gowhatsapp_message_type_login:
            gowhatsapp_handle_qrcode(pc, gwamsg);
            break;
        case gowhatsapp_message_type_pairing_succeeded:
            gowhatsapp_close_qrcode(gwamsg->account);
            break;
        case gowhatsapp_message_type_credentials:
            gowhatsapp_store_credentials(gwamsg->account, gwamsg->text);
            break;
        case gowhatsapp_message_type_connected:
            gowhatsapp_close_qrcode(gwamsg->account);
            if (purple_account_settings_get_boolean(settings,
                                                    GOWHATSAPP_REQUEST_CONTACTS_AFTER_LOGIN_OPTION,
                                                    TRUE)) {
                gowhatsapp_go_get_contacts(gwamsg->account, FALSE);
            } else {
                gowhatsapp_connection_set_online(pc);
            }
            break;
        case gowhatsapp_message_type_name:
            if (gwamsg->remoteJid == NULL) {
                gowhatsapp_connection_set_online(pc);
                gowhatsapp_roomlist_get_list(pc);
            } else {
                gowhatsapp_ensure_buddy_in_blist(gwamsg->account, gwamsg->remoteJid, gwamsg->name);
            }
            break;
        case gowhatsapp_message_type_disconnected:
            purple_account_disconnect(gwamsg->account, gwamsg->text);
            gowhatsapp_close_qrcode(gwamsg->account);
            break;
        case gowhatsapp_message_type_text:
            if (!gowhatsapp_message_is_old(gwamsg)) {
                gowhatsapp_display_text_message(gwamsg->account, gwamsg->senderJid, gwamsg->remoteJid,
                                                gwamsg->text, gwamsg->timestamp, gwamsg->isGroup,
                                                gwamsg->isOutgoing, gwamsg->name, 0,
                                                gwamsg->messageId, TRUE);
            }
            break;
        case gowhatsapp_message_type_system:
            /* TODO(libpurple-3): PurpleMessageFlags is gone; system-message marking
             * needs to be expressed via the new PurpleMessage attribute API once
             * display_message.c is ported. */
            gowhatsapp_display_text_message(gwamsg->account, gwamsg->senderJid, gwamsg->remoteJid,
                                            gwamsg->text, gwamsg->timestamp, gwamsg->isGroup,
                                            gwamsg->isOutgoing, gwamsg->name, 0,
                                            gwamsg->messageId, TRUE);
            break;
        case gowhatsapp_message_type_typing:
        case gowhatsapp_message_type_typing_stopped:
            /* TODO(libpurple-3): serv_got_typing[_stopped] is gone. Typing state
             * now lives on PurpleConversation; look the conversation up via
             * PurpleConversationManager (handled when conversation/IM glue lands). */
            break;
        case gowhatsapp_message_type_presence:
            gowhatsapp_handle_presence(gwamsg->account, gwamsg->remoteJid, gwamsg->subtype, gwamsg->timestamp);
            break;
        case gowhatsapp_message_type_attachment:
            if (!gowhatsapp_message_is_old(gwamsg)) {
                gowhatsapp_handle_attachment(gwamsg);
            }
            break;
        case gowhatsapp_message_type_profile_picture:
            gowhatsapp_handle_profile_picture(gwamsg);
            break;
        case gowhatsapp_message_type_group:
            gowhatsapp_handle_group(pc, gwamsg);
            break;
        default:
            g_info("%s: handling for this message type is not implemented.", GOWHATSAPP_NAME);
            g_free(gwamsg->blob);
    }
}
