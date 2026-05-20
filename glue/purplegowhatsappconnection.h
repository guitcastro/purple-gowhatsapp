#pragma once

#include <gplugin.h>
#include <gplugin-native.h>
#include <purple.h>

#include "gowhatsapp.h"

G_BEGIN_DECLS

#define PURPLE_GOWHATSAPP_TYPE_CONNECTION purple_gowhatsapp_connection_get_type()
G_DECLARE_FINAL_TYPE(PurpleGowhatsappConnection,
                     purple_gowhatsapp_connection,
                     PURPLE_GOWHATSAPP,
                     CONNECTION,
                     PurpleConnection)

void                  purple_gowhatsapp_connection_register(GPluginNativePlugin *plugin);
WhatsappProtocolData *purple_gowhatsapp_connection_get_protocol_data(PurpleConnection *connection);

G_END_DECLS
