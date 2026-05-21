#include "gowhatsapp.h"
#include "constants.h"
#include "libwhatsmeow.h" // for gowhatsapp_go_query_group_participants / _query_groups / _get_display_name

#include "purplegowhatsappconnection.h"

/*
 * libpurple 3 deleted PurpleRoomlist (no replacement; channel discovery is
 * expected to go through PurpleChannelJoinDetails on
 * PurpleProtocolConversation) and the static proto_chat_entry plumbing that
 * hung off PurplePluginProtocolInfo (chat_info / chat_info_defaults /
 * join_chat). Group conversations themselves now live as
 * PURPLE_CONVERSATION_TYPE_CHANNEL entries inside PurpleConversationManager,
 * with participants tracked via PurpleConversationMembers (PurpleConversationMember
 * GObjects backed by PurpleContactInfo).
 */

static PurpleConversation *
find_channel(PurpleAccount *account, const char *remoteJid)
{
    PurpleConversationManager *manager = purple_core_get_conversation_manager(purple_core_get_default());
    return purple_conversation_manager_find(manager, account,
                                            PURPLE_CONVERSATION_TYPE_CHANNEL,
                                            remoteJid);
}

void
gowhatsapp_chat_set_participants(PurpleConversation *conversation, char **participants)
{
    if (conversation == NULL) {
        return;
    }
    PurpleAccount *account = purple_conversation_get_account(conversation);
    if (account == NULL) {
        return;
    }
    PurpleConversationMembers *members = purple_conversation_get_members(conversation);
    PurpleContactManager *cm = purple_core_get_contact_manager(purple_core_get_default());

    /* libpurple 3 keeps members as PurpleConversationMember GObjects rather
     * than a plain char-array. (Re-)add each participant; the manager is
     * idempotent if a member for the same contact info already exists. */
    for (char **p = participants; p != NULL && *p != NULL; p++) {
        gboolean found = FALSE;
        PurpleContact *contact = purple_contact_manager_find_or_create(cm, account, *p, &found);
        if (contact == NULL) {
            continue;
        }
        PurpleContactInfo *info = PURPLE_CONTACT_INFO(contact);
        if (purple_conversation_members_find_member(members, info) == NULL) {
            purple_conversation_members_add_member(members, info, FALSE, NULL);
        }
    }
}

PurpleConversation *
gowhatsapp_enter_group_chat(PurpleConnection *pc, const char *remoteJid, char **participants)
{
    g_return_val_if_fail(pc != NULL, NULL);
    g_return_val_if_fail(remoteJid != NULL, NULL);

    PurpleAccount *account = purple_connection_get_account(pc);
    PurpleConversationManager *manager = purple_core_get_conversation_manager(purple_core_get_default());

    PurpleConversation *conversation = find_channel(account, remoteJid);
    if (conversation == NULL) {
        conversation = purple_conversation_new(account, PURPLE_CONVERSATION_TYPE_CHANNEL, remoteJid);
        purple_conversation_manager_add(manager, conversation);
        g_object_unref(conversation);
    }

    if (participants == NULL) {
        char **fetched = gowhatsapp_go_query_group_participants(account, (char *)remoteJid);
        gowhatsapp_chat_set_participants(conversation, fetched);
        g_strfreev(fetched);
    } else {
        gowhatsapp_chat_set_participants(conversation, participants);
    }
    return conversation;
}

void
gowhatsapp_handle_group(PurpleConnection *pc, gowhatsapp_message_t *gwamsg)
{
    g_return_if_fail(pc != NULL);
    g_return_if_fail(gwamsg != NULL);
    g_return_if_fail(gwamsg->account != NULL);

    if (gwamsg->remoteJid == NULL) {
        /* End-of-list sentinel from whatsmeow. With no roomlist there is
         * nothing to flush here. */
        return;
    }

    PurpleAccountSettings *settings = purple_account_get_settings(gwamsg->account);

    if (purple_account_settings_get_boolean(settings,
                                            GOWHATSAPP_REQUEST_CONTACTS_AFTER_LOGIN_OPTION,
                                            TRUE)) {
        gowhatsapp_ensure_group_chat_in_blist(gwamsg->account, gwamsg->remoteJid, gwamsg->name);
    }

    /* If a conversation is already open, refresh its participant list. */
    PurpleConversation *conversation = find_channel(gwamsg->account, gwamsg->remoteJid);
    if (conversation != NULL) {
        gowhatsapp_chat_set_participants(conversation, gwamsg->participants);
    }

    if (purple_account_settings_get_boolean(settings, GOWHATSAPP_AUTO_JOIN_CHAT_OPTION, FALSE)) {
        gowhatsapp_enter_group_chat(pc, gwamsg->remoteJid, gwamsg->participants);
    }
}
