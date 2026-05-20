#include "purplegowhatsappprotocol.h"

#include "gowhatsapp.h"

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
                                             G_GNUC_UNUSED PurpleAccount *account,
                                             GError **error)
{
    /* TODO: define a PurpleGowhatsappConnection GObject (G_DECLARE_FINAL_TYPE
     * deriving from PurpleConnection) and return an instance here. The libpurple 3
     * port of the connection logic in login.c / process_message.c will live there. */
    g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                        "purple-gowhatsapp libpurple 3 port: connection class not yet implemented");
    return NULL;
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
