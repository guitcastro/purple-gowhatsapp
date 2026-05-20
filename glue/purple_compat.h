#include <purple.h>

#if PURPLE_API_MAJOR_VERSION >= 3
/*
 * libpurple 3 compatibility shims.
 *
 * Many libpurple 2 type names were removed or renamed in libpurple 3.
 * The aliases below make the existing headers parse against libpurple 3
 * so that the source files can be ported one at a time. Call sites that
 * use removed APIs will still fail to compile/link until the
 * corresponding .c file is ported.
 */
typedef PurpleFileTransfer PurpleXfer;
typedef PurpleConversation PurpleConvChat;
typedef PurpleConversation PurpleChat;
typedef PurpleContactInfo  PurpleBuddy;
/* No libpurple 3 equivalents — opaque placeholders just to make the header parse. */
typedef struct _PurpleRoomlist        PurpleRoomlist;
typedef struct _PurpleRoomlistRoom    PurpleRoomlistRoom;
typedef struct _PurpleGroup           PurpleGroup;
typedef struct _PurpleNotifyUserInfo  PurpleNotifyUserInfo;
/* PurpleMessageFlags was a bitmask; libpurple 3 exposes message metadata via
 * PurpleMessage getters/setters instead. Treat the flags as an opaque int
 * during the port so existing prototypes still parse. */
typedef guint PurpleMessageFlags;
/* PurpleStatus was removed; libpurple 3 uses PurplePresence and PurpleSavedPresence.
 * Opaque placeholder while presence.c still references the old type. */
typedef struct _PurpleStatus PurpleStatus;
#endif

#define PURPLE_XFER_TYPE_SEND PURPLE_XFER_SEND
#define PURPLE_IS_CHAT PURPLE_BLIST_NODE_IS_CHAT
#define purple_connection_error purple_connection_error_reason
#define PURPLE_CONNECTION_CONNECTING PURPLE_CONNECTING
#define PURPLE_CONNECTION_CONNECTED PURPLE_CONNECTED
#define PURPLE_CONNECTION_DISCONNECTED PURPLE_DISCONNECTED
#define PurpleIMConversation PurpleConvIm
#define purple_blist_find_group purple_find_group
#define purple_blist_find_buddy purple_find_buddy
#define purple_chat_conversation_add_user purple_conv_chat_add_user
#define purple_im_conversation_new(account, from) PURPLE_CONV_IM(purple_conversation_new(PURPLE_CONV_TYPE_IM, account, from))
#define PURPLE_CONVERSATION(chatorim) ((chatorim) == NULL ? NULL : (chatorim)->conv)
#define purple_serv_got_im serv_got_im
#define purple_serv_got_chat_in serv_got_chat_in
#define purple_serv_got_alias serv_got_alias
#define purple_connection_set_flags(pc, f) ((pc)->flags = (f))
#define purple_connection_get_flags(pc) ((pc)->flags)
#define purple_connection_get_protocol          purple_connection_get_prpl
#define PurpleConversationUpdateType       PurpleConvUpdateType
#define PURPLE_CONVERSATION_UPDATE_UNSEEN  PURPLE_CONV_UPDATE_UNSEEN
#define purple_blist_chat_new purple_chat_new
