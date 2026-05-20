#include "gowhatsapp.h"
#include "constants.h"
#include "libwhatsmeow.h" // for gowhatsapp_go_send_presence / _subscribe_presence

/*
 * libpurple 3 dropped PurpleStatus / PurpleStatusType in favor of
 * PurplePresence + PurplePresencePrimitive. Each PurpleContactInfo
 * (and therefore each PurpleAccount and PurpleContact) has an attached
 * PurplePresence; we drive the primitive directly.
 */
static PurplePresencePrimitive
primitive_for_remote(PurpleAccount *account, char online)
{
    if (online != 0) {
        return PURPLE_PRESENCE_PRIMITIVE_AVAILABLE;
    }
    PurpleAccountSettings *settings = purple_account_get_settings(account);
    gboolean fake_online = purple_account_settings_get_boolean(settings,
                                                               GOWHATSAPP_FAKE_ONLINE_OPTION,
                                                               TRUE);
    return fake_online ? PURPLE_PRESENCE_PRIMITIVE_AWAY
                       : PURPLE_PRESENCE_PRIMITIVE_OFFLINE;
}

void
gowhatsapp_handle_presence(PurpleAccount *account,
                           char *remoteJid,
                           char online,
                           G_GNUC_UNUSED time_t last_seen)
{
    g_return_if_fail(account != NULL);
    g_return_if_fail(remoteJid != NULL);

    PurpleContactManager *manager = purple_core_get_contact_manager(purple_core_get_default());
    PurpleContact *contact = purple_contact_manager_find_with_id(manager, account, remoteJid);
    if (contact == NULL) {
        return;
    }

    PurplePresence *presence = purple_contact_info_get_presence(PURPLE_CONTACT_INFO(contact));
    if (presence != NULL) {
        purple_presence_set_primitive(presence, primitive_for_remote(account, online));
    }

    /* TODO(libpurple-3): persist last_seen. libpurple 2 stored it on the
     * buddy node via purple_blist_node_set_int; libpurple 3 has no
     * equivalent generic key/value bag on the contact yet. */
}

void
gowhatsapp_handle_profile_picture(gowhatsapp_message_t *gwamsg)
{
    g_return_if_fail(gwamsg != NULL);
    g_return_if_fail(gwamsg->account != NULL);
    g_return_if_fail(gwamsg->remoteJid != NULL);

    PurpleContactManager *manager = purple_core_get_contact_manager(purple_core_get_default());
    PurpleContact *contact = purple_contact_manager_find_with_id(manager, gwamsg->account, gwamsg->remoteJid);
    if (contact == NULL) {
        g_free(gwamsg->blob);
        gwamsg->blob = NULL;
        return;
    }

    if (gwamsg->blob != NULL && gwamsg->blobsize > 0) {
        PurpleImage *image = purple_image_new_from_data(gwamsg->blob, gwamsg->blobsize);
        purple_contact_info_set_avatar(PURPLE_CONTACT_INFO(contact), image);
        g_object_unref(image);
    }

    /* purple_image_new_from_data copies the bytes, so it is our responsibility
     * to release the blob whatsmeow handed us. */
    g_free(gwamsg->blob);
    gwamsg->blob = NULL;

    /* TODO(libpurple-3): persist picture_id and picture_date so the next
     * gowhatsapp_request_profile_picture can ask whatsmeow to skip
     * re-downloading. libpurple 2 used purple_blist_node_set_string. */
}

void
gowhatsapp_tooltip_text(G_GNUC_UNUSED PurpleBuddy *buddy,
                        G_GNUC_UNUSED PurpleNotifyUserInfo *info,
                        G_GNUC_UNUSED gboolean full)
{
    /* libpurple 3 has no PurpleNotifyUserInfo; the buddy tooltip used to be
     * extended by appending pairs to it. TODO once the replacement landed
     * in libpurple 3 (currently no equivalent — protocols expose extra info
     * via PurpleContactInfo properties surfaced in the UI directly). */
}

void
gowhatsapp_subscribe_presence_updates(PurpleAccount *account, PurpleBuddy *buddy)
{
    g_return_if_fail(account != NULL);
    g_return_if_fail(buddy != NULL);

    PurplePresence *own = purple_contact_info_get_presence(PURPLE_CONTACT_INFO(account));
    if (own == NULL) {
        return;
    }
    /* WhatsApp only delivers presence updates while we are advertising
     * "available" ourselves. */
    if (purple_presence_get_primitive(own) != PURPLE_PRESENCE_PRIMITIVE_AVAILABLE) {
        return;
    }

    const char *id = purple_contact_info_get_id(PURPLE_CONTACT_INFO(buddy));
    if (id != NULL) {
        gowhatsapp_go_subscribe_presence(account, (char *)id);
    }
}

void
gowhatsapp_request_profile_picture(PurpleAccount *account, PurpleBuddy *buddy)
{
    g_return_if_fail(account != NULL);
    g_return_if_fail(buddy != NULL);

    PurpleAccountSettings *settings = purple_account_get_settings(account);
    const char *mode = purple_account_settings_get_string(settings, GOWHATSAPP_ICONS_OPTION,
                                                          GOWHATSAPP_ICONS_CHOICE_NO);
    if (g_strcmp0(mode, GOWHATSAPP_ICONS_CHOICE_NO) == 0) {
        return;
    }

    const char *id = purple_contact_info_get_id(PURPLE_CONTACT_INFO(buddy));
    if (id == NULL) {
        return;
    }
    /* libpurple 3 has no per-contact key/value bag yet, so we cannot pass
     * the previously seen picture id/date to allow whatsmeow to skip
     * re-downloading. Pass empty strings; whatsmeow will always fetch. */
    gowhatsapp_go_request_profile_picture(account, (char *)id, "", "");
}

/*
 * Own outbound presence. In libpurple 2 this was hung off the protocol
 * vtable (.set_status = gowhatsapp_set_presence) and called with a
 * PurpleStatus *. libpurple 3 routes the same change through
 * PurpleConnectionClass::set_presence, which would receive a
 * PurpleSavedPresence instead. The connection class is not yet wiring
 * that hook up, so this function is kept around (with its legacy
 * PurpleStatus parameter, opaqued via purple_compat.h) to satisfy the
 * commands.c / process_message.c TODO call sites once they are
 * re-enabled.
 */
void
gowhatsapp_set_presence(G_GNUC_UNUSED PurpleAccount *account,
                        G_GNUC_UNUSED PurpleStatus *status)
{
    /* TODO(libpurple-3): port to PurpleConnectionClass::set_presence with
     * PurpleSavedPresence + PurplePresencePrimitive. Subscribe to remote
     * presence updates via gowhatsapp_for_all_buddies + the helper above
     * once the new entry point lands. */
}
