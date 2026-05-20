#include "gowhatsapp.h"
#include "constants.h"
#include "libwhatsmeow.h" // for gowhatsapp_go_query_group_participants / _query_groups / _get_display_name

#include "purplegowhatsappconnection.h"

/*
 * libpurple 3 dropped both the PurpleRoomlist family (no replacement; the
 * channel-discovery use case is mostly handled via
 * PurpleChannelJoinDetails on the PurpleProtocolConversation interface)
 * and the static proto_chat_entry / chat_info_defaults plumbing that hung
 * off PurplePluginProtocolInfo. Group conversations themselves now live
 * as PURPLE_CONVERSATION_TYPE_CHANNEL entries in the
 * PurpleConversationManager, with members tracked via
 * PurpleConversationMembers.
 */

/******************************************************************************
 * Roomlist — fully stubbed.
 *
 * libpurple 3 has no PurpleRoomlist. Once the channel-join hook is wired
 * into the protocol GObject we can rebuild the "list of joinable groups"
 * UX through PurpleChannelJoinDetails instead.
 *****************************************************************************/
PurpleRoomlist *
gowhatsapp_roomlist_get_list(G_GNUC_UNUSED PurpleConnection *pc)
{
    /* TODO(libpurple-3): replace with PurpleProtocolConversation.get_channel_join_details
     * + PurpleProtocolConversation.join_channel_async. */
    return NULL;
}

void
gowhatsapp_roomlist_add_room(G_GNUC_UNUSED PurpleConnection *pc,
                             G_GNUC_UNUSED char *remoteJid,
                             G_GNUC_UNUSED char *name)
{
    /* TODO(libpurple-3): no-op until the channel-join replacement lands. */
}

gchar *
gowhatsapp_roomlist_serialize(G_GNUC_UNUSED PurpleRoomlistRoom *room)
{
    return NULL;
}

/******************************************************************************
 * proto_chat_entry-style API — stubbed.
 *
 * In libpurple 2 the user-facing "Join a Chat..." dialog was driven off
 * PurplePluginProtocolInfo.chat_info / chat_info_defaults / join_chat.
 * libpurple 3 expects PurpleProtocolConversation.get_channel_join_details
 * to return a PurpleChannelJoinDetails describing the same fields. The
 * old GList<struct proto_chat_entry *> hand-off has no equivalent.
 *****************************************************************************/
GList *
gowhatsapp_chat_info(G_GNUC_UNUSED PurpleConnection *pc)
{
    /* TODO(libpurple-3): expose via PurpleChannelJoinDetails. */
    return NULL;
}

GHashTable *
gowhatsapp_chat_info_defaults(G_GNUC_UNUSED PurpleConnection *pc,
                              G_GNUC_UNUSED const char *chat_name)
{
    return NULL;
}

void
gowhatsapp_join_chat(G_GNUC_UNUSED PurpleConnection *pc,
                     G_GNUC_UNUSED GHashTable *data)
{
    /* TODO(libpurple-3): the join-channel flow now lives on
     * PurpleProtocolConversation.join_channel_async. */
}

char *
gowhatsapp_get_chat_name(GHashTable *components)
{
    if (components == NULL) {
        return NULL;
    }
    const char *jid = g_hash_table_lookup(components, "name");
    return g_strdup(jid);
}

void
gowhatsapp_set_chat_topic(G_GNUC_UNUSED PurpleConnection *pc,
                          G_GNUC_UNUSED int id,
                          G_GNUC_UNUSED const char *topic)
{
    /* TODO(libpurple-3): there is no per-id lookup any more; the topic
     * setter lives on PurpleConversation directly. Once a UI path calls
     * us with a real PurpleConversation, route to purple_conversation_set_topic. */
}

/******************************************************************************
 * Group conversation helpers
 *****************************************************************************/
static PurpleConversation *
find_channel(PurpleAccount *account, const char *remoteJid)
{
    PurpleConversationManager *manager = purple_core_get_conversation_manager(purple_core_get_default());
    return purple_conversation_manager_find(manager, account,
                                            PURPLE_CONVERSATION_TYPE_CHANNEL,
                                            remoteJid);
}

void
gowhatsapp_chat_set_participants(PurpleConvChat *conv_chat, char **participants)
{
    if (conv_chat == NULL) {
        return;
    }
    /* In our compat shim PurpleConvChat is a typedef for PurpleConversation. */
    PurpleConversation *conversation = (PurpleConversation *)conv_chat;
    PurpleAccount *account = purple_conversation_get_account(conversation);
    if (account == NULL) {
        return;
    }
    PurpleConversationMembers *members = purple_conversation_get_members(conversation);
    PurpleContactManager *cm = purple_core_get_contact_manager(purple_core_get_default());

    /* libpurple 3 keeps members as PurpleConversationMember GObjects rather
     * than the libpurple 2 char-array. We just (re-)add each participant; the
     * manager's add_member is idempotent if a member for the same contact
     * info already exists. */
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
        /* End-of-list sentinel from whatsmeow. With the roomlist gone there
         * is nothing to flush here. */
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

char *
gowhatsapp_get_cb_alias(PurpleConnection *connection,
                        G_GNUC_UNUSED int id,
                        const char *who)
{
    return gowhatsapp_go_get_display_name(purple_connection_get_account(connection), (char *)who);
}

void
gowhatsapp_free_name(G_GNUC_UNUSED PurpleConversation *conv)
{
    /* TODO(libpurple-3): libpurple 2 used to leak the strdup'd "name" key
     * stored in PurpleConversation's data hash table; there is no
     * equivalent storage anymore so there is nothing to release here. */
}
