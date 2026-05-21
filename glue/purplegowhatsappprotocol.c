#include "purplegowhatsappprotocol.h"

#include "constants.h"
#include "gowhatsapp.h"
#include "purplegowhatsappconnection.h"

#define PURPLE_GOWHATSAPP_PROTOCOL_DOMAIN (g_quark_from_static_string("purple-gowhatsapp-protocol"))

struct _PurpleGowhatsappProtocol {
    PurpleProtocol parent;
};

/******************************************************************************
 * PurpleProtocolConversation Implementation
 *****************************************************************************/
static void
purple_gowhatsapp_protocol_conversation_iface_init(PurpleProtocolConversationInterface *iface)
{
    iface->send_message_async  = gowhatsapp_send_message_async;
    iface->send_message_finish = gowhatsapp_send_message_finish;
}

/******************************************************************************
 * PurpleProtocolFileTransfer Implementation
 *****************************************************************************/
static void
purple_gowhatsapp_protocol_file_transfer_iface_init(PurpleProtocolFileTransferInterface *iface)
{
    iface->send_async  = gowhatsapp_send_async;
    iface->send_finish = gowhatsapp_send_finish;
}

G_DEFINE_DYNAMIC_TYPE_EXTENDED(
    PurpleGowhatsappProtocol,
    purple_gowhatsapp_protocol,
    PURPLE_TYPE_PROTOCOL,
    G_TYPE_FLAG_FINAL,
    G_IMPLEMENT_INTERFACE_DYNAMIC(PURPLE_TYPE_PROTOCOL_CONVERSATION,
                                  purple_gowhatsapp_protocol_conversation_iface_init)
    G_IMPLEMENT_INTERFACE_DYNAMIC(PURPLE_TYPE_PROTOCOL_FILE_TRANSFER,
                                  purple_gowhatsapp_protocol_file_transfer_iface_init))

static PurpleConnection *
purple_gowhatsapp_protocol_create_connection(G_GNUC_UNUSED PurpleProtocol *protocol,
                                             PurpleAccount *account,
                                             G_GNUC_UNUSED GError **error)
{
    return g_object_new(
        PURPLE_GOWHATSAPP_TYPE_CONNECTION,
        "account", account,
        NULL);
}

static PurpleAccountSettings *
purple_gowhatsapp_protocol_get_default_account_settings(G_GNUC_UNUSED PurpleProtocol *protocol)
{
    return gowhatsapp_get_default_account_settings();
}

static gboolean
purple_gowhatsapp_protocol_validate_account(G_GNUC_UNUSED PurpleProtocol *protocol,
                                            PurpleAccount *account,
                                            GError **error)
{
    PurpleAccountSettings *settings = purple_account_get_settings(account);
    const char *phone = purple_account_settings_get_string(settings,
                                                           GOWHATSAPP_PHONE_NUMBER_OPTION,
                                                           NULL);
    if (phone == NULL || phone[0] == '\0') {
        g_set_error_literal(error, PURPLE_GOWHATSAPP_PROTOCOL_DOMAIN, 0,
                            "Phone number is required (international format, digits only).");
        return FALSE;
    }
    /* Cheap sanity check: digits only, at least 8 characters. The full
     * validation lives on the whatsmeow side. */
    for (const char *p = phone; *p; p++) {
        if (*p < '0' || *p > '9') {
            g_set_error_literal(error, PURPLE_GOWHATSAPP_PROTOCOL_DOMAIN, 0,
                                "Phone number must contain digits only (no '+', spaces or dashes).");
            return FALSE;
        }
    }
    if (strlen(phone) < 8) {
        g_set_error_literal(error, PURPLE_GOWHATSAPP_PROTOCOL_DOMAIN, 0,
                            "Phone number looks too short. Use the international format, digits only.");
        return FALSE;
    }
    return TRUE;
}

static void
purple_gowhatsapp_protocol_init(G_GNUC_UNUSED PurpleGowhatsappProtocol *protocol)
{
}

static void
purple_gowhatsapp_protocol_class_finalize(G_GNUC_UNUSED PurpleGowhatsappProtocolClass *klass)
{
}

static void
purple_gowhatsapp_protocol_class_init(PurpleGowhatsappProtocolClass *klass)
{
    PurpleProtocolClass *protocol_class = PURPLE_PROTOCOL_CLASS(klass);

    protocol_class->create_connection = purple_gowhatsapp_protocol_create_connection;
    protocol_class->get_default_account_settings =
        purple_gowhatsapp_protocol_get_default_account_settings;
    protocol_class->validate_account = purple_gowhatsapp_protocol_validate_account;
}

void
purple_gowhatsapp_protocol_register(GPluginNativePlugin *plugin)
{
    purple_gowhatsapp_protocol_register_type(G_TYPE_MODULE(plugin));
}

PurpleProtocol *
purple_gowhatsapp_protocol_new(void)
{
    return g_object_new(
        PURPLE_GOWHATSAPP_TYPE_PROTOCOL,
        "id", GOWHATSAPP_PRPL_ID,
        "name", "WhatsApp (whatsmeow)",
        "description", "WhatsApp via whatsmeow",
        "icon-name", "whatsapp",
        NULL);
}
