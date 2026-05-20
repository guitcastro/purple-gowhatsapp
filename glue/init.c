/*
 *   gowhatsapp plugin for libpurple
 *   Copyright (C) 2021 Hermann Höhne
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <glib.h>

#include <gplugin.h>
#include <gplugin-native.h>

#include <purple.h>

#include "gowhatsapp.h"
#include "constants.h"
#include "purplegowhatsappconnection.h"
#include "purplegowhatsappprotocol.h"

#ifndef PLUGIN_VERSION
#error Must set PLUGIN_VERSION in build system
#endif
#define MAKE_STR(x) _MAKE_STR(x)
#define _MAKE_STR(x) #x

#define PURPLE_GOWHATSAPP_DOMAIN (g_quark_from_static_string("purple-gowhatsapp"))

static PurpleProtocol *gowhatsapp_protocol = NULL;

static GPluginPluginInfo *
purple_gowhatsapp_query(G_GNUC_UNUSED GError **error)
{
    PurplePluginInfoFlags flags = PURPLE_PLUGIN_INFO_FLAGS_AUTO_LOAD;
    const char * const authors[] = {
        "Hermann Hoehne <hoehermann@gmx.de>",
        NULL,
    };

    return purple_plugin_info_new(
        "id",          GOWHATSAPP_PRPL_ID,
        "name",        "WhatsApp (whatsmeow)",
        "authors",     authors,
        "version",     MAKE_STR(PLUGIN_VERSION),
        "category",    "Protocol",
        "summary",     "WhatsApp protocol plugin",
        "description", "WhatsApp protocol plugin backed by whatsmeow.",
        "website",     "https://github.com/hoehermann/purple-gowhatsapp",
        "abi-version", PURPLE_ABI_VERSION,
        "flags",       flags,
        NULL);
}

static gboolean
purple_gowhatsapp_load(GPluginPlugin *plugin, GError **error)
{
    PurpleProtocolManager *manager = NULL;

    if (PURPLE_IS_PROTOCOL(gowhatsapp_protocol)) {
        g_set_error_literal(error, PURPLE_GOWHATSAPP_DOMAIN, 0,
                            "plugin was not cleaned up properly");
        return FALSE;
    }

    purple_gowhatsapp_connection_register(GPLUGIN_NATIVE_PLUGIN(plugin));
    purple_gowhatsapp_protocol_register(GPLUGIN_NATIVE_PLUGIN(plugin));

    gowhatsapp_protocol = purple_gowhatsapp_protocol_new();

    manager = purple_core_get_protocol_manager(purple_core_get_default());
    if (PURPLE_IS_PROTOCOL_MANAGER(manager)) {
        if (!purple_protocol_manager_add(manager, gowhatsapp_protocol, error)) {
            g_clear_object(&gowhatsapp_protocol);
            return FALSE;
        }
    }

    return TRUE;
}

static gboolean
purple_gowhatsapp_unload(G_GNUC_UNUSED GPluginPlugin *plugin,
                        G_GNUC_UNUSED gboolean shutdown,
                        GError **error)
{
    PurpleProtocolManager *manager = NULL;

    if (!PURPLE_IS_PROTOCOL(gowhatsapp_protocol)) {
        g_set_error_literal(error, PURPLE_GOWHATSAPP_DOMAIN, 0,
                            "plugin was not setup properly");
        return FALSE;
    }

    manager = purple_core_get_protocol_manager(purple_core_get_default());
    if (PURPLE_IS_PROTOCOL_MANAGER(manager)) {
        if (!purple_protocol_manager_remove(manager, gowhatsapp_protocol, error)) {
            return FALSE;
        }
    }

    g_clear_object(&gowhatsapp_protocol);

    return TRUE;
}

/*
 * Stubs for unported glue/ files.
 *
 * These satisfy link-time references from already-ported files (notably
 * glue/bridge.c) while the rest of the glue layer is still being ported
 * to libpurple 3. Each TODO marker below should be removed once the
 * corresponding .c file is rewritten and re-added to glue/CMakeLists.txt.
 */

GPLUGIN_NATIVE_PLUGIN_DECLARE(purple_gowhatsapp)
