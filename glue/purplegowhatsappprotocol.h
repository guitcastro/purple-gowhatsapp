#pragma once

#include <gplugin.h>
#include <gplugin-native.h>
#include <purple.h>

G_BEGIN_DECLS

#define PURPLE_GOWHATSAPP_TYPE_PROTOCOL purple_gowhatsapp_protocol_get_type()
G_DECLARE_FINAL_TYPE(PurpleGowhatsappProtocol,
                     purple_gowhatsapp_protocol,
                     PURPLE_GOWHATSAPP,
                     PROTOCOL,
                     PurpleProtocol)

void            purple_gowhatsapp_protocol_register(GPluginNativePlugin *plugin);
PurpleProtocol *purple_gowhatsapp_protocol_new(void);

G_END_DECLS
