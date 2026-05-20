#include "gowhatsapp.h"

/*
 * libpurple 2 had a "conversation-updated" signal (raised by
 * purple_signal_connect on the purple_conversations_get_handle() bus)
 * that fired with PURPLE_CONVERSATION_UPDATE_UNSEEN when an inactive
 * conversation accumulated or shed unseen events. This file used that
 * signal to call gowhatsapp_go_mark_read_conversation when the unseen
 * counter dropped from > 0 to 0, i.e. when the user actually looked at
 * the chat.
 *
 * libpurple 3 dropped the bespoke signal bus in favor of GObject
 * signals on the conversation / conversation-manager objects, and the
 * "unseen-count" key/value bag attached to a PurpleConversation is no
 * longer available via purple_conversation_get_data. Re-implementing
 * the read-receipt heuristic against the new API needs:
 *   - subscribing to PurpleConversationManager "added"
 *   - per-conversation notify::active (or equivalent) listeners
 *   - tracking unseen state in the connection GObject
 * which is non-trivial and Pidgin-specific.
 *
 * For now this is a no-op so the connection class can call it
 * unconditionally once it is plugged back in.
 */

void
gowhatsapp_receipts_init(G_GNUC_UNUSED PurpleConnection *pc)
{
    /* TODO(libpurple-3): port the read-receipt heuristic to GObject signals. */
}
