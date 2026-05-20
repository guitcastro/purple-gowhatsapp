#include "gowhatsapp.h"
#include "constants.h"
#include "libwhatsmeow.h"

/*
 * libpurple 3 dropped the tree-shaped buddy list (PurpleGroup → PurpleBuddy
 * / PurpleChat) and replaced it with the flat PurpleContactManager (a
 * GListModel of PurpleContact per account) and PurpleConversationManager
 * (GListModel of PurpleConversation per account). The functions below
 * re-express the gowhatsapp buddy/group helpers against the new model.
 *
 * Things that do not survive the port:
 *  - PurpleGroup ("Whatsapp" parent group): there is no equivalent. We do
 *    not surface a group; the UI groups contacts by tags or by account.
 *  - PurpleBuddy.node arbitrary key/value storage (purple_blist_node_get/
 *    set_string): for now we do not persist a separate server_alias.
 *  - "Fake away" handling for offline contacts. PurplePresence has a new
 *    shape (PurpleSavedPresence + PurplePresencePrimitive) which has to be
 *    ported together with glue/presence.c.
 */

PurpleBuddy *
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

    /* TODO(libpurple-3): once glue/presence.c lands, subscribe to presence
     * updates here (was gowhatsapp_subscribe_presence_updates). */

    /* PurpleBuddy is aliased to PurpleContactInfo in purple_compat.h, and
     * PurpleContact derives from PurpleContactInfo. */
    return PURPLE_CONTACT_INFO(contact);
}

void
gowhatsapp_for_all_buddies(PurpleAccount *account,
                           void (*func)(PurpleAccount *, PurpleBuddy *))
{
    g_return_if_fail(account != NULL);
    g_return_if_fail(func != NULL);

    PurpleContactManager *manager = purple_core_get_contact_manager(purple_core_get_default());
    GListModel *contacts = purple_contact_manager_get_all(manager, account);
    if (contacts == NULL) {
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
 * Called by gowhatsapp_handle_group when the protocol learns about a group
 * chat. In libpurple 2 this registered a PurpleChat in the persistent
 * buddy list. In libpurple 3 the equivalent is registering a
 * PURPLE_CONVERSATION_TYPE_CHANNEL conversation with the conversation
 * manager; persistence across restarts is handled by the manager itself.
 */
PurpleChat *
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

    return conversation; /* PurpleChat is aliased to PurpleConversation in purple_compat.h. */
}

PurpleChat *
gowhatsapp_find_blist_chat(PurpleAccount *account, const char *jid)
{
    if (account == NULL || jid == NULL) {
        return NULL;
    }
    PurpleConversationManager *manager = purple_core_get_conversation_manager(purple_core_get_default());
    return purple_conversation_manager_find(manager, account,
                                            PURPLE_CONVERSATION_TYPE_CHANNEL, jid);
}

/* Stubs kept for the unported callers (presence.c, groups.c). The function
 * signatures still mention PurpleBuddy because the original glue treats
 * PurpleContact ⇔ PurpleBuddy interchangeably during this transitional
 * period (typedef in purple_compat.h). */

void
gowhatsapp_assume_buddy_away(G_GNUC_UNUSED PurpleAccount *account,
                             G_GNUC_UNUSED PurpleBuddy *buddy)
{
    /* TODO(libpurple-3): port to PurplePresence + PurplePresencePrimitive
     * once glue/presence.c is rewritten. */
}

void
gowhatsapp_add_buddy(G_GNUC_UNUSED PurpleConnection *pc,
                     G_GNUC_UNUSED PurpleBuddy *buddy,
                     G_GNUC_UNUSED PurpleGroup *group)
{
    /* TODO(libpurple-3): there is no PurpleProtocolClient.add_buddy hook in
     * libpurple 3. The protocol class learns about new contacts through the
     * PurpleContactManager directly, so this entire entry point will likely
     * be removed once the rest of the buddy code lands. */
}
