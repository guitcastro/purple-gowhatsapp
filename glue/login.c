#include "gowhatsapp.h"
#include "constants.h"

/*
 * The actual login / close logic lives in PurpleGowhatsappConnection
 * (glue/purplegowhatsappconnection.c) which now overrides
 * PurpleConnectionClass::connect and ::disconnect. This file is kept
 * around for credential persistence, which is still called from
 * glue/process_message.c when whatsmeow finishes pairing.
 */
void
gowhatsapp_store_credentials(PurpleAccount *account, char *credentials)
{
    /* TODO(libpurple-3): when the project grows a libsecret / KWallet
     * provider via PurpleCredentialManager we should hand the secret to
     * the manager instead of stashing it in plain text in the account
     * settings. For now this matches the libpurple 2 behavior of
     * purple_account_set_string(GOWHATSAPP_CREDENTIALS_KEY).
     *
     * The bitlbee compatibility branch that mirrored credentials into
     * the password field and emitted "bitlbee-set-account-password" via
     * purple_signal_emit is dropped: libpurple 3 has no signal bus and
     * the password field is now managed asynchronously through
     * PurpleCredentialManager. */
    PurpleAccountSettings *settings = purple_account_get_settings(account);
    purple_account_settings_set_string(settings, GOWHATSAPP_CREDENTIALS_KEY, credentials);
}
