#include "bridge.h"
/*
 * These are functions that will be called from the go part.
 */

/*
 * Whether the given pointer actually refers to an existing account.
 */
int gowhatsapp_account_exists(PurpleAccount *account) {
    PurpleCore *core = purple_core_get_default();
    if (core == NULL || !PURPLE_IS_CORE(core)) {
        /* During Pidgin shutdown the core has already been torn down but
         * pending whatsmeow goroutines may still try to deliver messages.
         * Treat those as "account is gone" so they get dropped quietly. */
        return 0;
    }
    PurpleAccountManager *manager = purple_core_get_account_manager(core);
    if (!PURPLE_IS_ACCOUNT_MANAGER(manager)) {
        return 0;
    }
    /* get_connected_accounts() only includes accounts whose connection has
     * already transitioned to CONNECTED; while a freshly enabled account is
     * still in CONNECTING (e.g. waiting for the QR-code scan) it is missing
     * from that list, which would cause us to discard every inbound message.
     * get_enabled() covers both the connecting and the connected states. */
    GListModel *accounts = purple_account_manager_get_enabled(manager);
    if (!G_IS_LIST_MODEL(accounts)) {
        return 0;
    }
    guint n = g_list_model_get_n_items(accounts);
    int account_exists = 0;
    for (guint i = 0; i < n && !account_exists; i++) {
        PurpleAccount *acc = PURPLE_ACCOUNT(g_list_model_get_item(accounts, i));
        if (acc == account) {
            account_exists = 1;
        }
        g_object_unref(acc);
    }
    return account_exists;
}

#if !GLIB_CHECK_VERSION(2, 68, 0)
#define g_memdup2 g_memdup
#endif

/*
 * Handler for a message received by go-whatsapp.
 * Called by go-whatsapp (outside of the GTK eventloop).
 * 
 * Yes, this is indeed neccessary – we checked.
 */
void gowhatsapp_process_message_bridge(gowhatsapp_message_t gwamsg_go) {
    // copying Go-managed struct into heap
    // the strings inside the struct already reside in the heap, according to https://golang.org/cmd/cgo/#hdr-C_references_to_Go
    gowhatsapp_message_t *gwamsg_heap = g_memdup2(&gwamsg_go, sizeof gwamsg_go);
    g_idle_add(
        process_message_bridge, // handle message in main thread
        gwamsg_heap // data to handle in main thread
    );
}
