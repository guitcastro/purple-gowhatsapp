#include "purplegowhatsappprotocol.h"

#include "gowhatsapp.h"
#include "purplegowhatsappconnection.h"

struct _PurpleGowhatsappProtocol {
    PurpleProtocol parent;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(
    PurpleGowhatsappProtocol,
    purple_gowhatsapp_protocol,
    PURPLE_TYPE_PROTOCOL,
    G_TYPE_FLAG_FINAL,
    {})

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
