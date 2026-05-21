#include "gowhatsapp.h"
#include "constants.h"
#include "libwhatsmeow.h"

/*
 * libpurple 3 dropped the tree-shaped buddy list (PurpleGroup → PurpleBuddy /
 * PurpleChat) and replaced it with the flat PurpleContactManager (a GListModel
 * of PurpleContact per account). Group chats live as PURPLE_CONVERSATION_TYPE_
 * CHANNEL entries in PurpleConversationManager. The helpers below speak the
 * new model directly.
 */

PurpleContactInfo *
gowhatsapp_ensure_buddy_in_blist(PurpleAccount *account, const char *identifier, const char *name)
{
    g_return_val_if_fail(account != NULL, NULL);
    g_return_val_if_fail(identifier != NULL, NULL);

    if (g_str_has_suffix(identifier, "@lid")) {
        /* TODO(libpurple-3): combine into existing non-hidden buddy. */
        return NULL;
    }

    PurpleContactManager *manager = purple_core_get_contact_manager(purple_core_get_default());
    gboolean found = FALSE;
    PurpleContact *contact = purple_contact_manager_find_or_create(manager, account, identifier, &found);
    PurpleContactInfo *info = PURPLE_CONTACT_INFO(contact);

    if (name != NULL && *name != '\0') {
        const char *current_alias = purple_contact_info_get_alias(info);
        if (current_alias == NULL || *current_alias == '\0') {
            purple_contact_info_set_alias(info, name);
        }
        const char *current_display = purple_contact_info_get_display_name(info);
        if (g_strcmp0(current_display, name) != 0) {
            purple_contact_info_set_display_name(info, name);
        }
    }

    /* TODO(libpurple-3): subscribe to presence updates here once the
     * outbound-presence flow on PurpleConnectionClass::set_presence lands. */

    return info;
}

void
gowhatsapp_for_all_buddies(PurpleAccount *account,
                           void (*func)(PurpleAccount *, PurpleContactInfo *))
{
    g_return_if_fail(account != NULL);
    g_return_if_fail(func != NULL);

    PurpleContactManager *manager = purple_core_get_contact_manager(purple_core_get_default());
    GListModel *contacts = purple_contact_manager_get_all(manager, account);
    if (!G_IS_LIST_MODEL(contacts)) {
        return;
    }

    guint n = g_list_model_get_n_items(contacts);
    for (guint i = 0; i < n; i++) {
        PurpleContact *contact = PURPLE_CONTACT(g_list_model_get_item(contacts, i));
        func(account, PURPLE_CONTACT_INFO(contact));
        g_object_unref(contact);
    }
}

const char *
gowhatsapp_blist_get_alias(PurpleAccount *account, const char *who)
{
    if (account == NULL || who == NULL) {
        return NULL;
    }
    PurpleContactManager *manager = purple_core_get_contact_manager(purple_core_get_default());
    PurpleContact *contact = purple_contact_manager_find_with_id(manager, account, who);
    if (contact == NULL) {
        return NULL;
    }
    return purple_contact_info_get_alias(PURPLE_CONTACT_INFO(contact));
}

/*
 * In libpurple 2 a "group chat in the blist" was a persistent PurpleChat. In
 * libpurple 3 the persistent representation is a PURPLE_CONVERSATION_TYPE_
 * CHANNEL conversation living on the PurpleConversationManager.
 */
PurpleConversation *
gowhatsapp_ensure_group_chat_in_blist(PurpleAccount *account, const char *remoteJid, const char *topic)
{
    g_return_val_if_fail(account != NULL, NULL);
    g_return_val_if_fail(remoteJid != NULL, NULL);

    PurpleConversationManager *manager = purple_core_get_conversation_manager(purple_core_get_default());
    PurpleConversation *conversation = purple_conversation_manager_find(manager, account,
                                                                        PURPLE_CONVERSATION_TYPE_CHANNEL,
                                                                        remoteJid);
    if (conversation == NULL) {
        conversation = purple_conversation_new(account, PURPLE_CONVERSATION_TYPE_CHANNEL, remoteJid);
        purple_conversation_manager_add(manager, conversation);
        g_object_unref(conversation);
    }

    if (topic != NULL) {
        purple_conversation_set_topic(conversation, topic);
    }
    return conversation;
}

PurpleConversation *
gowhatsapp_find_blist_chat(PurpleAccount *account, const char *jid)
{
    if (account == NULL || jid == NULL) {
        return NULL;
    }
    PurpleConversationManager *manager = purple_core_get_conversation_manager(purple_core_get_default());
    return purple_conversation_manager_find(manager, account,
                                            PURPLE_CONVERSATION_TYPE_CHANNEL, jid);
}
