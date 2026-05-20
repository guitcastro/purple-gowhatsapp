#include "gowhatsapp.h"
#include "constants.h"
#include "../bridge.h"
#include "bridge.h"

/*
 * This is the C/gtk side of the go → C communication.
 */

/////////////////////////////////////////////////////////////////////
//                                                                 //
//      WELCOME TO THE LAND OF ABANDONMENT OF TYPE AND SAFETY      //
//                        Wanderer, beware.                        //
//                                                                 //
/////////////////////////////////////////////////////////////////////

/*
 * Basic message processing.
 * Log messages are always processed.
 * Queries Pidgin for a list of all accounts.
 * Ignores message if no appropriate connection exists.
 */
static void process_message(gowhatsapp_message_t * gwamsg) {
    if (gwamsg->msgtype == gowhatsapp_message_type_log) {
        // log messages do not need an active connection
        g_log(GOWHATSAPP_NAME, G_LOG_LEVEL_DEBUG, "%s", gwamsg->text);
        return;
    }
    int account_exists = gowhatsapp_account_exists(gwamsg->account);
    if (account_exists == 0) {
        g_log(GOWHATSAPP_NAME, G_LOG_LEVEL_WARNING, "No account %p. Ignoring message.", gwamsg->account);
        return;
    }
    PurpleConnection *connection = purple_account_get_connection(gwamsg->account);
    if (connection == NULL) {
        g_log(GOWHATSAPP_NAME, G_LOG_LEVEL_WARNING, "No active connection for account %p. Ignoring message.", gwamsg->account);
        return;
    }
    gowhatsapp_process_message(gwamsg);
}

/*
 * Handler for a message received by go-whatsapp.
 * Called inside of the GTK eventloop.
 * Releases almost all memory allocated by CGO on heap.
 *
 * @return Whether to execute again. Always FALSE.
 */
gboolean process_message_bridge(gpointer data) {
    gowhatsapp_message_t * gwamsg = (gowhatsapp_message_t *)data;
    process_message(gwamsg);
    gowhatsapp_free_message(gwamsg); // always clean up data in heap
    return FALSE;
}

void gowhatsapp_free_message(gowhatsapp_message_t *gwamsg) {
    g_free(gwamsg->remoteJid);
    g_free(gwamsg->senderJid);
    g_free(gwamsg->text);
    g_free(gwamsg->name);
    // g_free(gwamsg->blob); this is cleared after handling the qrcode PNG or profile picture
    g_free(gwamsg->hash_hex);
    g_free(gwamsg->filename);
    g_free(gwamsg->extension);
    g_free(gwamsg->mimetype);
    g_free(gwamsg->messageId);
    g_strfreev(gwamsg->participants);
    g_free(gwamsg);
}
