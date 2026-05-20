#include "gowhatsapp.h"
#include "libwhatsmeow.h"

/*
 * libpurple 3 replaced the libpurple 2 PurpleXfer + per-xfer callback set
 * (init / start / end / cancel) with PurpleFileTransfer (a state machine
 * GObject backed by a GFile) and the PurpleProtocolFileTransfer interface
 * implemented on the protocol class. The interface only exposes
 * send_async / send_finish (and receive_async / receive_finish) — the
 * lifecycle is otherwise driven by PurpleFileTransferState transitions.
 */

void
gowhatsapp_send_async(G_GNUC_UNUSED PurpleProtocolFileTransfer *protocol,
                      PurpleFileTransfer *transfer,
                      GAsyncReadyCallback callback,
                      gpointer data)
{
    GTask *task = g_task_new(protocol, NULL, callback, data);

    PurpleAccount *account = purple_file_transfer_get_account(transfer);
    PurpleContactInfo *remote = purple_file_transfer_get_remote(transfer);
    GFile *local = purple_file_transfer_get_local_file(transfer);

    if (account == NULL || remote == NULL || local == NULL) {
        purple_file_transfer_set_state(transfer, PURPLE_FILE_TRANSFER_STATE_FAILED);
        g_task_return_new_error(task, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                                "missing account, remote contact or local file");
        g_clear_object(&task);
        return;
    }

    char *path = g_file_get_path(local);
    if (path == NULL) {
        purple_file_transfer_set_state(transfer, PURPLE_FILE_TRANSFER_STATE_FAILED);
        g_task_return_new_error(task, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                                "non-local file (remote GFile has no path)");
        g_clear_object(&task);
        return;
    }
    const char *who = purple_contact_info_get_id(remote);

    purple_file_transfer_set_state(transfer, PURPLE_FILE_TRANSFER_STATE_STARTED);
    char *error = gowhatsapp_go_send_file(account, (char *)who, path);
    g_free(path);

    if (error != NULL && error[0] != '\0') {
        purple_file_transfer_set_state(transfer, PURPLE_FILE_TRANSFER_STATE_FAILED);
        g_task_return_new_error(task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                "gowhatsapp_go_send_file: %s", error);
    } else {
        purple_file_transfer_set_state(transfer, PURPLE_FILE_TRANSFER_STATE_FINISHED);
        g_task_return_boolean(task, TRUE);
    }
    g_free(error);
    g_clear_object(&task);
}

gboolean
gowhatsapp_send_finish(G_GNUC_UNUSED PurpleProtocolFileTransfer *protocol,
                       GAsyncResult *result,
                       GError **error)
{
    return g_task_propagate_boolean(G_TASK(result), error);
}
