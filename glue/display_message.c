#include "gowhatsapp.h"
#include "constants.h"

/*
 * Look up the PurpleContact for a remote JID, creating it if necessary,
 * and stamping its display name when we have a friendly one.
 */
static PurpleContact *
find_or_create_contact(PurpleAccount *account, const gchar *jid, const gchar *display_name)
{
    PurpleContactManager *manager = purple_core_get_contact_manager(purple_core_get_default());
    gboolean found = FALSE;
    PurpleContact *contact = purple_contact_manager_find_or_create(manager, account, jid, &found);

    if (display_name != NULL && *display_name != '\0') {
        purple_contact_info_set_display_name(PURPLE_CONTACT_INFO(contact), display_name);
    }

    return contact;
}

/*
 * Find or create the conversation for this exchange and add the author
 * as a conversation member if it isn't one already.
 */
static PurpleConversationMember *
ensure_member(PurpleConversation *conversation, PurpleContact *contact)
{
    PurpleConversationMembers *members = purple_conversation_get_members(conversation);
    PurpleContactInfo *info = PURPLE_CONTACT_INFO(contact);

    PurpleConversationMember *member = purple_conversation_members_find_member(members, info);
    if (member == NULL) {
        member = purple_conversation_members_add_member(members, info, FALSE, NULL);
    }
    return member;
}

static PurpleConversation *
find_or_create_conversation(PurpleAccount *account, const gchar *remoteJid, gboolean isGroup)
{
    PurpleConversationManager *manager = purple_core_get_conversation_manager(purple_core_get_default());
    PurpleConversationType type = isGroup
        ? PURPLE_CONVERSATION_TYPE_CHANNEL
        : PURPLE_CONVERSATION_TYPE_DM;

    PurpleConversation *conversation = purple_conversation_manager_find(manager, account, type, remoteJid);
    if (conversation == NULL) {
        conversation = purple_conversation_new(account, type, remoteJid);
        purple_conversation_manager_add(manager, conversation);
        g_object_unref(conversation);
    }
    return conversation;
}

void
gowhatsapp_display_text_message(
    PurpleAccount *account,
    const gchar *senderJid,
    const gchar *remoteJid,
    const gchar *text,
    const time_t timestamp,
    const gboolean isGroup,
    G_GNUC_UNUSED const gboolean isOutgoing,
    const gchar *name,
    G_GNUC_UNUSED PurpleMessageFlags flags,
    const gchar *messageId,
    const gboolean escape)
{
    g_return_if_fail(account != NULL);
    g_return_if_fail(remoteJid != NULL);

    PurpleAccountSettings *settings = purple_account_get_settings(account);

    /* libpurple 3 no longer uses PURPLE_MESSAGE_SEND/RECV flags. The UI infers
     * direction from the author's contact id — if it matches the account's id
     * the message is rendered as outgoing. The senderJid coming from whatsmeow
     * already encodes that distinction, so we just need a contact for it. */
    const gchar *authorJid = senderJid != NULL ? senderJid : "system";

    PurpleContact *contact = find_or_create_contact(account, authorJid, name);
    PurpleConversation *conversation = find_or_create_conversation(account, remoteJid, isGroup);
    PurpleConversationMember *author = ensure_member(conversation, contact);

    /* Build the visible payload. WhatsApp is plain-text, so HTML-escape unless
     * a caller has already done it, and turn newlines into <br/>. */
    gchar *body = NULL;
    if (escape) {
        gchar *html = g_markup_escape_text(text, -1);
        GString *withbr = g_string_new(NULL);
        for (const gchar *p = html; *p; p++) {
            if (*p == '\n') {
                g_string_append(withbr, "<br/>");
            } else {
                g_string_append_c(withbr, *p);
            }
        }
        g_free(html);
        body = g_string_free(withbr, FALSE);
    } else {
        body = g_strdup(text);
    }

    gchar *body_with_id = NULL;
    if (messageId != NULL &&
        purple_account_settings_get_boolean(settings, GOWHATSAPP_DISPLAY_MESSAGE_ID_OPTION, FALSE)) {
        body_with_id = g_strdup_printf("%s <span lang=\"id\">%s</span>", body, messageId);
        g_free(body);
    } else {
        body_with_id = body;
    }

    PurpleMessage *message = purple_message_new(author, body_with_id);
    g_free(body_with_id);

    if (timestamp > 0) {
        GDateTime *dt = g_date_time_new_from_unix_utc((gint64)timestamp);
        if (dt != NULL) {
            purple_message_set_timestamp(message, dt);
            g_date_time_unref(dt);
        }
    }

    purple_conversation_write_message(conversation, message);
    g_object_unref(message);
}
