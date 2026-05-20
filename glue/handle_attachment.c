#include "gowhatsapp.h"
#include "libwhatsmeow.h"
#include "constants.h"
#include "pixbuf.h"

#include "glib/gstdio.h"

/*
 * Inbound attachment handling. libpurple 2 had three rendering paths:
 *
 *   1. Inline the image via purple_imgstore_add_with_id + an <img id="N"/>
 *      tag injected into a chat message.
 *   2. Download to a user-configured local path template, then post a
 *      link in the chat.
 *   3. Ask the user via PurpleXfer (with init / start / end / cancel
 *      callbacks) where to save the file.
 *
 * libpurple 3 broke all three: PurpleImageStore + the inline <img> tag
 * representation is gone (images now travel through PurpleMessage's
 * attachment / image properties), PurpleXfer is replaced by
 * PurpleFileTransfer + PurpleProtocolFileTransfer.receive_async, and
 * the libpurple 2 buddy/chat alias lookups have all moved over to
 * PurpleContactInfo accessors.
 *
 * The port below keeps the templated-download path working (this is the
 * variant that actually persists files to disk and is what most users
 * are after) and reduces the inline / PurpleXfer paths to TODOs.
 */

static const char *
display_alias_for(PurpleAccount *account, const char *jid)
{
    if (account == NULL || jid == NULL || *jid == '\0') {
        return jid;
    }
    PurpleContactManager *manager = purple_core_get_contact_manager(purple_core_get_default());
    PurpleContact *contact = purple_contact_manager_find_with_id(manager, account, jid);
    if (contact == NULL) {
        return jid;
    }
    const char *alias = purple_contact_info_get_alias(PURPLE_CONTACT_INFO(contact));
    if (alias == NULL || *alias == '\0' || strchr(alias, '/') != NULL) {
        return jid;
    }
    return alias;
}

static void
replace_placeholder(gpointer key, gpointer value, gpointer user_data)
{
    char **text = user_data;
    /* GLib < 2.68 has no g_string_replace, so use the old token-by-token
     * scan that the libpurple 2 build used. */
    GString *gs = g_string_new(*text);
    g_string_replace(gs, key, value, 0);
    g_free(*text);
    *text = g_string_free(gs, FALSE);
}

char *
gowhatsapp_attachment_fill_template(const char *template,
                                    time_t timestamp,
                                    const char *hash,
                                    const char *filename,
                                    const char *extension,
                                    const char *remote,
                                    const char *sender,
                                    const char *chat_alias,
                                    const char *buddy_alias,
                                    const char *messageid,
                                    G_GNUC_UNUSED PurpleMessageFlags flags)
{
    if (g_strcmp0(remote, sender) == 0) {
        sender = "";
    }
    const char *direction = "";
    /* libpurple 3 dropped PurpleMessageFlags; direction is no longer
     * encoded as flags. Leave the placeholder for now and let the
     * caller pass an empty string until the new attribute model lands. */

    GHashTable *replacements = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    g_hash_table_insert(replacements, "$home",      (char *)g_get_home_dir());
    g_hash_table_insert(replacements, "$purple",    (char *)g_get_user_data_dir());
    g_hash_table_insert(replacements, "$hash",      (char *)hash);
    g_hash_table_insert(replacements, "$direction", (char *)direction);
    g_hash_table_insert(replacements, "$remote",    (char *)remote);
    g_hash_table_insert(replacements, "$sender",    (char *)sender);
    g_hash_table_insert(replacements, "$name",      (char *)buddy_alias);
    g_hash_table_insert(replacements, "$title",     (char *)chat_alias);
    g_hash_table_insert(replacements, "$messageid", (char *)messageid);
    g_hash_table_insert(replacements, "$extension", (char *)extension);
    g_hash_table_insert(replacements, "$filename",  (char *)filename);

    /* Format the strftime tokens first, then substitute the rest. */
    char strftime_buf[1024];
    struct tm *tmv = localtime(&timestamp);
    if (tmv != NULL) {
        strftime(strftime_buf, sizeof strftime_buf, template, tmv);
    } else {
        g_strlcpy(strftime_buf, template, sizeof strftime_buf);
    }

    char *replaced = g_strdup(strftime_buf);
    g_hash_table_foreach(replacements, replace_placeholder, &replaced);
    g_hash_table_destroy(replacements);
    return replaced;
}

static void
download_to_templated_destination(gowhatsapp_message_t *gwamsg, const char *local_path_template)
{
    const char *chat_alias  = display_alias_for(gwamsg->account, gwamsg->remoteJid);
    const char *buddy_alias = display_alias_for(gwamsg->account, gwamsg->senderJid);
    if (g_strcmp0(gwamsg->remoteJid, gwamsg->senderJid) == 0) {
        chat_alias = buddy_alias;
    }

    char *local_path = gowhatsapp_attachment_fill_template(
        local_path_template,
        gwamsg->timestamp,
        gwamsg->hash_hex,
        gwamsg->filename,
        gwamsg->extension,
        gwamsg->remoteJid,
        gwamsg->senderJid,
        chat_alias,
        buddy_alias,
        gwamsg->messageId,
        0);

    char *error = gowhatsapp_go_download_attachment(gwamsg->account,
                                                    local_path,
                                                    gwamsg->download_handle);
    gowhatsapp_go_delete_handle(gwamsg->download_handle);

    if (error != NULL && error[0] != '\0') {
        gowhatsapp_display_text_message(gwamsg->account, gwamsg->senderJid,
                                        gwamsg->remoteJid, error,
                                        gwamsg->timestamp, gwamsg->isGroup,
                                        gwamsg->isOutgoing, gwamsg->name, 0,
                                        gwamsg->messageId, TRUE);
    } else {
        PurpleAccountSettings *settings = purple_account_get_settings(gwamsg->account);
        const char *url_template = purple_account_settings_get_string(
            settings, GOWHATSAPP_ATTACHMENT_URL_TEMPLATE_OPTION,
            GOWHATSAPP_ATTACHMENT_URL_TEMPLATE_DEFAULT);

        char *url = NULL;
        if (url_template != NULL && url_template[0] != '\0') {
            url = gowhatsapp_attachment_fill_template(url_template,
                                                      gwamsg->timestamp,
                                                      gwamsg->hash_hex,
                                                      gwamsg->filename,
                                                      gwamsg->extension,
                                                      gwamsg->remoteJid,
                                                      gwamsg->senderJid,
                                                      chat_alias, buddy_alias,
                                                      gwamsg->messageId, 0);
        } else {
            url = gowhatsapp_go_url_from_local_path(local_path);
        }

        gowhatsapp_display_text_message(gwamsg->account, gwamsg->senderJid,
                                        gwamsg->remoteJid, url,
                                        gwamsg->timestamp, gwamsg->isGroup,
                                        gwamsg->isOutgoing, gwamsg->name, 0,
                                        gwamsg->messageId, TRUE);
        g_free(url);

        /* Display the caption (the text accompanying the attachment, if any). */
        if (gwamsg->text != NULL && gwamsg->text[0] != '\0') {
            gowhatsapp_display_text_message(gwamsg->account, gwamsg->senderJid,
                                            gwamsg->remoteJid, gwamsg->text,
                                            gwamsg->timestamp, gwamsg->isGroup,
                                            gwamsg->isOutgoing, gwamsg->name, 0,
                                            gwamsg->messageId, TRUE);
        }

        /* TODO(libpurple-3): inline image rendering used to push the PNG
         * bytes through purple_imgstore_add_with_id and embed an <img id="N"/>
         * tag in the chat. libpurple 3 carries images on PurpleMessage's
         * attachment / image properties instead (see PurpleImage +
         * PurpleAttachments), which display_message.c would need to support. */
    }

    g_free(error);
    g_free(local_path);
}

void
gowhatsapp_handle_attachment(gowhatsapp_message_t *gwamsg)
{
    g_return_if_fail(gwamsg != NULL);
    g_return_if_fail(gwamsg->account != NULL);

    PurpleAccountSettings *settings = purple_account_get_settings(gwamsg->account);
    const char *local_path_template = purple_account_settings_get_string(
        settings, GOWHATSAPP_ATTACHMENT_PATH_TEMPLATE_OPTION,
        GOWHATSAPP_ATTACHMENT_PATH_TEMPLATE_DEFAULT);

    if (local_path_template != NULL && local_path_template[0] != '\0') {
        download_to_templated_destination(gwamsg, local_path_template);
        return;
    }

    /* TODO(libpurple-3): if no local path template is configured, the
     * libpurple 2 code asked the user via PurpleXfer (purple_xfer_request).
     * The equivalent now is to drive the inbound side of
     * PurpleProtocolFileTransfer.receive_async — construct a
     * PurpleFileTransfer via purple_file_transfer_new_receive and let
     * libpurple drive the negotiation through the state machine. Until
     * that lands, drop the attachment with a chat message and free the
     * pending download handle so whatsmeow can release its temp file. */
    gowhatsapp_display_text_message(gwamsg->account, gwamsg->senderJid,
                                    gwamsg->remoteJid,
                                    "(attachment received but no download path is configured)",
                                    gwamsg->timestamp, gwamsg->isGroup,
                                    gwamsg->isOutgoing, gwamsg->name, 0,
                                    gwamsg->messageId, TRUE);
    gowhatsapp_go_delete_handle(gwamsg->download_handle);
    gwamsg->download_handle = 0;
}
