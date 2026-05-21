#pragma once

#include <purple.h>

#include "bridge.h"

#define GOWHATSAPP_NAME "whatsmeow"  // name to refer to this plug-in (in logs)
#define GOWHATSAPP_PRPL_ID "prpl-hehoe-whatsmeow"

#define GOWHATSAPP_STATUS_STR_AVAILABLE "available" // this must match whatsmeow's types.PresenceAvailable
#define GOWHATSAPP_STATUS_STR_AWAY      "unavailable" // this must match whatsmeow's types.PresenceUnavailable
#define GOWHATSAPP_STATUS_STR_OFFLINE   "offline"
#define GOWHATSAPP_STATUS_STR_MOBILE    "mobile"

// per-connection protocol state (lives on PurpleGowhatsappConnection)
typedef struct {
    // when this connection was (re-)established; used to discard old messages
    time_t connected_at_timestamp;
} WhatsappProtocolData;

// options
PurpleAccountSettings *gowhatsapp_get_default_account_settings(void);

// login
void gowhatsapp_store_credentials(PurpleAccount *account, char *credentials);

// qrcode
void gowhatsapp_handle_qrcode(PurpleConnection *pc, gowhatsapp_message_t *gwamsg);
void gowhatsapp_close_qrcode(PurpleAccount *account);

// process_message
void gowhatsapp_process_message(gowhatsapp_message_t *gwamsg);

// display_message
void gowhatsapp_display_text_message(PurpleAccount *account,
                                     const gchar *senderJid,
                                     const gchar *remoteJid,
                                     const gchar *text,
                                     const time_t timestamp,
                                     const gboolean isGroup,
                                     const gboolean isOutgoing,
                                     const gchar *name,
                                     const gchar *messageId,
                                     const gboolean escape);

// groups
PurpleConversation *gowhatsapp_enter_group_chat(PurpleConnection *pc, const char *remoteJid, char **participants);
void                gowhatsapp_chat_set_participants(PurpleConversation *conversation, char **participants);
void                gowhatsapp_handle_group(PurpleConnection *pc, gowhatsapp_message_t *gwamsg);
PurpleConversation *gowhatsapp_find_blist_chat(PurpleAccount *account, const char *jid);
PurpleConversation *gowhatsapp_ensure_group_chat_in_blist(PurpleAccount *account, const char *remoteJid, const char *topic);

// blist
PurpleContactInfo *gowhatsapp_ensure_buddy_in_blist(PurpleAccount *account, const char *remoteJid, const char *display_name);
void               gowhatsapp_for_all_buddies(PurpleAccount *account, void(*func)(PurpleAccount *, PurpleContactInfo *));
const char        *gowhatsapp_blist_get_alias(PurpleAccount *account, const char *who);

// send_message (PurpleProtocolConversation interface)
void     gowhatsapp_send_message_async(PurpleProtocolConversation *protocol,
                                       PurpleConversation *conversation,
                                       PurpleMessage *message,
                                       GCancellable *cancellable,
                                       GAsyncReadyCallback callback,
                                       gpointer data);
gboolean gowhatsapp_send_message_finish(PurpleProtocolConversation *protocol,
                                        GAsyncResult *result,
                                        GError **error);

// handle_attachment
void  gowhatsapp_handle_attachment(gowhatsapp_message_t *gwamsg);
char *gowhatsapp_attachment_fill_template(const char *template,
                                          time_t timestamp,
                                          const char *hash,
                                          const char *filename,
                                          const char *extension,
                                          const char *remote,
                                          const char *sender,
                                          const char *chat_alias,
                                          const char *buddy_alias,
                                          const char *messageid);

// send_file (PurpleProtocolFileTransfer interface)
void     gowhatsapp_send_async(PurpleProtocolFileTransfer *protocol,
                               PurpleFileTransfer *transfer,
                               GAsyncReadyCallback callback,
                               gpointer data);
gboolean gowhatsapp_send_finish(PurpleProtocolFileTransfer *protocol,
                                GAsyncResult *result,
                                GError **error);

// presence
void gowhatsapp_handle_presence(PurpleAccount *account, char *remoteJid, char available, time_t last_seen);
void gowhatsapp_subscribe_presence_updates(PurpleAccount *account, PurpleContactInfo *contact);

// profile pictures
void gowhatsapp_request_profile_picture(PurpleAccount *account, PurpleContactInfo *contact);
void gowhatsapp_handle_profile_picture(gowhatsapp_message_t *gwamsg);

// receipts
void gowhatsapp_receipts_init(PurpleConnection *pc);

// commands
enum gowhatsapp_command {
    GOWHATSAPP_COMMAND_NONE = 0,
    GOWHATSAPP_COMMAND_VERSIONS,
    GOWHATSAPP_COMMAND_CONTACTS,
    GOWHATSAPP_COMMAND_PARTICIPANTS,
    GOWHATSAPP_COMMAND_PRESENCE,
    GOWHATSAPP_COMMAND_LOGOUT
};
enum gowhatsapp_command is_command(const char *message);
int  execute_command(PurpleConnection *pc, const gchar *message, const gchar *who, PurpleConversation *conv);
