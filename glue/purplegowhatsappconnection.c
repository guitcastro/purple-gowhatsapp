#include "purplegowhatsappconnection.h"

#include "libwhatsmeow.h"

#define GOWHATSAPP_CREDENTIALS_KEY "credentials"

struct _PurpleGowhatsappConnection {
    PurpleConnection parent;

    WhatsappProtocolData wpd;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(
    PurpleGowhatsappConnection,
    purple_gowhatsapp_connection,
    PURPLE_TYPE_CONNECTION,
    G_TYPE_FLAG_FINAL,
    {})

/******************************************************************************
 * Helpers
 *****************************************************************************/
WhatsappProtocolData *
purple_gowhatsapp_connection_get_protocol_data(PurpleConnection *connection)
{
    g_return_val_if_fail(PURPLE_GOWHATSAPP_IS_CONNECTION(connection), NULL);
    return &PURPLE_GOWHATSAPP_CONNECTION(connection)->wpd;
}

/******************************************************************************
 * PurpleConnection Implementation
 *****************************************************************************/
static gboolean
purple_gowhatsapp_connection_connect(PurpleConnection *purple_connection,
                                     G_GNUC_UNUSED GError **error)
{
    PurpleGowhatsappConnection *connection = NULL;
    PurpleAccount *account = NULL;
    PurpleAccountSettings *settings = NULL;
    const char *credentials = NULL;
    char *username = NULL;
    char *user_dir = NULL;

    g_return_val_if_fail(PURPLE_GOWHATSAPP_IS_CONNECTION(purple_connection), FALSE);

    connection = PURPLE_GOWHATSAPP_CONNECTION(purple_connection);
    account = purple_connection_get_account(purple_connection);
    settings = purple_account_get_settings(account);

    /* Track when the connection was (requested to be) established so old
     * messages received during sync can be discarded. */
    connection->wpd.connected_at_timestamp = time(NULL) - 1;

    /* TODO(libpurple-3): port the proxy handling. libpurple 3 exposes proxy
     * configuration via GProxyResolver rather than PurpleProxyInfo. */

    /* TODO(libpurple-3): port the bitlbee password fallback once
     * PurpleCredentialManager is wired up. For now credentials must be
     * provided via the account setting. */
    credentials = purple_account_settings_get_string(settings,
                                                     GOWHATSAPP_CREDENTIALS_KEY,
                                                     NULL);

    /* libpurple 3 dropped the dedicated "username" concept; the account's ID
     * (set at account creation) is the stable identifier we hand to whatsmeow. */
    username = (char *)purple_contact_info_get_id(PURPLE_CONTACT_INFO(account));
    user_dir = (char *)g_get_user_data_dir();
    gowhatsapp_go_login(account, user_dir, username, (char *)credentials, NULL);

    /* TODO(libpurple-3): port glue/receipt.c then re-enable:
     *     gowhatsapp_receipts_init(purple_connection);
     */

    return TRUE;
}

static gboolean
purple_gowhatsapp_connection_disconnect(PurpleConnection *purple_connection,
                                        G_GNUC_UNUSED const char *message,
                                        G_GNUC_UNUSED GError **error)
{
    PurpleAccount *account = NULL;
    char *username = NULL;
    char *user_dir = NULL;

    g_return_val_if_fail(PURPLE_GOWHATSAPP_IS_CONNECTION(purple_connection), FALSE);

    account = purple_connection_get_account(purple_connection);
    /* libpurple 3 dropped the dedicated "username" concept; the account's ID
     * (set at account creation) is the stable identifier we hand to whatsmeow. */
    username = (char *)purple_contact_info_get_id(PURPLE_CONTACT_INFO(account));
    user_dir = (char *)g_get_user_data_dir();
    gowhatsapp_go_close(account, user_dir, username);

    return TRUE;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_gowhatsapp_connection_init(G_GNUC_UNUSED PurpleGowhatsappConnection *connection)
{
}

static void
purple_gowhatsapp_connection_class_finalize(G_GNUC_UNUSED PurpleGowhatsappConnectionClass *klass)
{
}

static void
purple_gowhatsapp_connection_class_init(PurpleGowhatsappConnectionClass *klass)
{
    PurpleConnectionClass *connection_class = PURPLE_CONNECTION_CLASS(klass);

    connection_class->connect = purple_gowhatsapp_connection_connect;
    connection_class->disconnect = purple_gowhatsapp_connection_disconnect;
}

void
purple_gowhatsapp_connection_register(GPluginNativePlugin *plugin)
{
    purple_gowhatsapp_connection_register_type(G_TYPE_MODULE(plugin));
}
