#include "gowhatsapp.h"
#include "constants.h"
#include "libwhatsmeow.h"

/*
 * Strip HTML out of an outgoing message body.
 *
 * Pidgin sends formatted text and WhatsApp is a plain-text protocol, so we
 * mirror the original libpurple 2 logic: if the user has the bridge-
 * compatibility option set we pass the raw bytes through, otherwise we
 * route through purple_markup_strip_html (which also turns <br/> into \n).
 */
static char *
build_outgoing_body(PurpleAccount *account, const char *contents)
{
    PurpleAccountSettings *settings = purple_account_get_settings(account);
    gboolean bridge = purple_account_settings_get_boolean(settings,
                                                          GOWHATSAPP_BRIDGE_COMPATIBILITY_OPTION,
                                                          FALSE);
    if (bridge) {
        return g_strdup(contents);
    }
    return purple_markup_strip_html(contents);
}

void
gowhatsapp_send_message_async(G_GNUC_UNUSED PurpleProtocolConversation *protocol,
                              PurpleConversation *conversation,
                              PurpleMessage *message,
                              GCancellable *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer data)
{
    GTask *task = g_task_new(protocol, cancellable, callback, data);

    PurpleAccount *account = purple_conversation_get_account(conversation);
    const char *id = purple_conversation_get_id(conversation);
    const char *contents = purple_message_get_contents(message);
    gboolean isGroup = purple_conversation_get_conversation_type(conversation)
                       == PURPLE_CONVERSATION_TYPE_CHANNEL;

    if (id == NULL || contents == NULL) {
        g_task_return_new_error(task, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                                "missing conversation id or message contents");
        g_clear_object(&task);
        return;
    }

    /* IRC-style ?commands (?contacts, ?logout, …) intercept the message
     * before it is sent to whatsmeow. */
    if (is_command(contents)) {
        PurpleConnection *pc = purple_account_get_connection(account);
        int rc = execute_command(pc, contents, id, conversation);
        if (rc == 0) {
            g_task_return_boolean(task, TRUE);
        } else {
            g_task_return_new_error(task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                    "command failed (rc=%d)", rc);
        }
        g_clear_object(&task);
        return;
    }

    char *body = build_outgoing_body(account, contents);
    int rc = gowhatsapp_go_send_message(account, (char *)id, body, isGroup);
    g_free(body);

    if (rc > 0) {
        purple_message_set_delivered(message, TRUE);
        /* Group conversations need an explicit local echo since whatsmeow
         * does not echo our own messages back to us. DMs already echo via
         * the gowhatsapp_message_type_text branch in process_message.c. */
        if (isGroup) {
            purple_conversation_write_message(conversation, message);
        }
        g_task_return_boolean(task, TRUE);
    } else {
        g_task_return_new_error(task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                "gowhatsapp_go_send_message returned %d", rc);
    }

    g_clear_object(&task);
}

gboolean
gowhatsapp_send_message_finish(G_GNUC_UNUSED PurpleProtocolConversation *protocol,
                               GAsyncResult *result,
                               GError **error)
{
    return g_task_propagate_boolean(G_TASK(result), error);
}
