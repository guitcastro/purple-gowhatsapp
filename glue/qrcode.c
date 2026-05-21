#include "gowhatsapp.h"
#include "constants.h"

#define PURPLE_GOWHATSAPP_QRCODE_DOMAIN (g_quark_from_static_string("purple-gowhatsapp-qrcode"))

static void
null_cb(G_GNUC_UNUSED void *data, G_GNUC_UNUSED PurpleRequestPage *page)
{
}

static void
dismiss_cb(void *data, G_GNUC_UNUSED PurpleRequestPage *page)
{
    PurpleAccount *account = data;
    purple_account_disconnect_with_new_error(account, "QR code was dismissed.",
                                             PURPLE_GOWHATSAPP_QRCODE_DOMAIN, 0,
                                             "%s", "QR code was dismissed.");
}

void
gowhatsapp_close_qrcode(PurpleAccount *account)
{
    /* Close any open request fields attached to this account. */
    purple_request_close_with_handle(account);
}

static void
gowhatsapp_display_qrcode(PurpleAccount *account,
                          const char *pairing_code,
                          const char *qr_data,
                          void *image_data,
                          size_t image_data_len)
{
    g_return_if_fail(account != NULL);

    PurpleRequestPage *page = purple_request_page_new();
    PurpleRequestGroup *group = purple_request_group_new(NULL);
    purple_request_page_add_group(page, group);

    purple_request_group_add_field(group,
        purple_request_field_string_new("pairing_code", "Pairing Code",
                                        pairing_code, FALSE));
    purple_request_group_add_field(group,
        purple_request_field_string_new("qr_data", "QR Code Data",
                                        qr_data, FALSE));
    PurpleRequestField *image_field =
        purple_request_field_image_new("qr_image", "QR Code Image",
                                       image_data, image_data_len);
    /* Pidgin renders image fields at native scale unless we override it; the
     * whatsmeow QR PNG is small (~256px) so push it up enough to be
     * scannable from a phone camera. */
    purple_request_field_image_set_scale(PURPLE_REQUEST_FIELD_IMAGE(image_field),
                                         4, 4);
    purple_request_group_add_field(group, image_field);

    /* purple_account_get_name() returns the user-supplied name (the WhatsApp
     * phone number on this protocol). purple_contact_info_get_id() would
     * return the internal account UUID instead. */
    const char *username = purple_account_get_name(account);
    char *secondary = g_strdup_printf("WhatsApp account %s", username);

    gowhatsapp_close_qrcode(account);
    purple_request_fields(
        account,            /* handle (also reused as user_data for dismiss_cb) */
        "Logon QR Code",
        "Please enter pairing code or scan the QR code",
        secondary,
        page,
        "OK",      G_CALLBACK(null_cb),
        "Dismiss", G_CALLBACK(dismiss_cb),
        NULL,               /* PurpleRequestCommonParameters */
        account);           /* user_data */

    g_free(secondary);
}

void
gowhatsapp_handle_qrcode(PurpleConnection *pc, gowhatsapp_message_t *gwamsg)
{
    PurpleAccount *account = purple_connection_get_account(pc);

    if (gwamsg->blobsize > 0) {
        gowhatsapp_display_qrcode(account, gwamsg->pairing_code,
                                  gwamsg->pairing_qrdata, gwamsg->blob,
                                  gwamsg->blobsize);
    } else {
        /* No image — fall back to the textual QR code (the same one
         * whatsmeow prints to the terminal). The user can paste it into
         * an external QR code generator. */
        g_log(GOWHATSAPP_NAME, G_LOG_LEVEL_MESSAGE,
              "Pairing code: %s\nQR code data: %s\n%s",
              gwamsg->pairing_code, gwamsg->pairing_qrdata,
              gwamsg->pairing_qrterminal ? gwamsg->pairing_qrterminal : "");
    }

    g_free(gwamsg->blob);
    gwamsg->blob = NULL;
}
