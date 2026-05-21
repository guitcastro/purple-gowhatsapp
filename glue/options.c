#include "gowhatsapp.h"
#include "constants.h"

static PurpleAccountSetting *
add_string(PurpleAccountSettings *settings, const char *id, const char *label, const char *default_value)
{
    PurpleAccountSetting *setting = purple_account_setting_string_new(id, label, default_value);
    purple_account_settings_add_setting(settings, setting);
    return setting;
}

static PurpleAccountSetting *
add_int(PurpleAccountSettings *settings, const char *id, const char *label, int default_value)
{
    PurpleAccountSetting *setting = purple_account_setting_int_new(id, label, default_value);
    purple_account_settings_add_setting(settings, setting);
    return setting;
}

static PurpleAccountSetting *
add_bool(PurpleAccountSettings *settings, const char *id, const char *label, gboolean default_value)
{
    PurpleAccountSetting *setting = purple_account_setting_boolean_new(id, label, default_value);
    purple_account_settings_add_setting(settings, setting);
    return setting;
}

static void
list_add(PurpleAccountSetting *setting, const char *id, const char *label)
{
    purple_account_setting_string_list_add_item(PURPLE_ACCOUNT_SETTING_STRING_LIST(setting), id, label);
}

PurpleAccountSettings *
gowhatsapp_get_default_account_settings(void)
{
    PurpleAccountSettings *settings = purple_account_settings_new();
    PurpleAccountSetting *setting = NULL;

    /* WhatsApp identity: phone number in international format, digits only
     * (no leading +, no spaces). E.g. 5511987654321 for +55 11 98765-4321. */
    add_string(settings, GOWHATSAPP_PHONE_NUMBER_OPTION,
               "Phone number (international format, digits only)",
               "");

    add_string(settings, GOWHATSAPP_DATABASE_ADDRESS_OPTION,
               "Database address",
               GOWHATSAPP_DATABASE_ADDRESS_DEFAULT);

    {
        char *device_name = g_strdup_printf(GOWHATSAPP_DEVICE_NAME_DEFAULT, g_get_host_name());
        add_string(settings, GOWHATSAPP_DEVICE_NAME_OPTION, "Device name", device_name);
        g_free(device_name);
    }

    setting = purple_account_setting_string_list_new(GOWHATSAPP_SEND_RECEIPT_OPTION, "Send receipts");
    list_add(setting, GOWHATSAPP_SEND_RECEIPT_CHOICE_IMMEDIATELY, "Immediately");
    list_add(setting, GOWHATSAPP_SEND_RECEIPT_CHOICE_ON_INTERACT, "When interacting with conversation");
    list_add(setting, GOWHATSAPP_SEND_RECEIPT_CHOICE_ON_ANSWER, "When sending a reply");
    list_add(setting, GOWHATSAPP_SEND_RECEIPT_CHOICE_NEVER, "Never");
    purple_account_setting_string_list_set_active_item(
        PURPLE_ACCOUNT_SETTING_STRING_LIST(setting),
        GOWHATSAPP_SEND_RECEIPT_CHOICE_IMMEDIATELY);
    purple_account_settings_add_setting(settings, setting);

    setting = purple_account_setting_string_list_new(GOWHATSAPP_ECHO_OPTION, "Echo sent messages");
    list_add(setting, GOWHATSAPP_ECHO_CHOICE_INTERNAL, "Internal");
    list_add(setting, GOWHATSAPP_ECHO_CHOICE_ON_SUCCESS, "On success");
    list_add(setting, GOWHATSAPP_ECHO_CHOICE_IMMEDIATELY, "Immediately");
    list_add(setting, GOWHATSAPP_ECHO_CHOICE_NEVER, "Never");
    purple_account_setting_string_list_set_active_item(
        PURPLE_ACCOUNT_SETTING_STRING_LIST(setting),
        GOWHATSAPP_ECHO_CHOICE_INTERNAL);
    purple_account_settings_add_setting(settings, setting);

    add_int(settings, GOWHATSAPP_EXPIRATION_OPTION,
            "Message duration in days (0 to disable expiration)", 0);
    add_int(settings, GOWHATSAPP_MESSAGE_CACHE_SIZE_OPTION,
            "Number of messages to cache", 0);
    add_int(settings, GOWHATSAPP_QRCODE_SIZE_OPTION,
            "QR code size (pixels)", 256);

    add_string(settings, GOWHATSAPP_ATTACHMENT_PATH_TEMPLATE_OPTION,
               "Attachment file path template",
               GOWHATSAPP_ATTACHMENT_PATH_TEMPLATE_DEFAULT);

    add_string(settings, GOWHATSAPP_ATTACHMENT_URL_TEMPLATE_OPTION,
               "Attachment base url",
               GOWHATSAPP_ATTACHMENT_URL_TEMPLATE_DEFAULT);

    add_int(settings, GOWHATSAPP_EMBED_MAX_FILE_SIZE_OPTION,
            "Maximum linked file-size (MB)", 0);

    add_string(settings, GOWHATSAPP_TRUSTED_URL_REGEX_OPTION,
               "Linked file trusted URL regex",
               GOWHATSAPP_TRUSTED_URL_REGEX_DEFAULT);

    setting = purple_account_setting_string_list_new(GOWHATSAPP_ICONS_OPTION,
                                                     "Download user profile pictures");
    list_add(setting, GOWHATSAPP_ICONS_CHOICE_NO, "no");
    list_add(setting, GOWHATSAPP_ICONS_CHOICE_PREVIEW, "preview");
    list_add(setting, GOWHATSAPP_ICONS_CHOICE_ORIGINAL, "original");
    purple_account_setting_string_list_set_active_item(
        PURPLE_ACCOUNT_SETTING_STRING_LIST(setting),
        GOWHATSAPP_ICONS_CHOICE_NO);
    purple_account_settings_add_setting(settings, setting);

    setting = purple_account_setting_string_list_new(GOWHATSAPP_HANDLE_IMAGES_OPTION,
                                                     "How to handle images");
    list_add(setting, GOWHATSAPP_HANDLE_IMAGES_CHOICE_BOTH,       "download to user-defined location and show");
    list_add(setting, GOWHATSAPP_HANDLE_IMAGES_CHOICE_INLINE,     "download to temporary location and show");
    list_add(setting, GOWHATSAPP_HANDLE_IMAGES_CHOICE_ATTACHMENT, "download to user-defined location only");
    purple_account_setting_string_list_set_active_item(
        PURPLE_ACCOUNT_SETTING_STRING_LIST(setting),
        GOWHATSAPP_HANDLE_IMAGES_CHOICE_BOTH);
    purple_account_settings_add_setting(settings, setting);

    add_bool(settings, GOWHATSAPP_DISCARD_OLD_MESSAGES_OPTION,        "Discard old messages",                                   FALSE);
    add_bool(settings, GOWHATSAPP_GROUP_IS_FILE_ORIGIN_OPTION,        "Treat group as the origin of files",                     TRUE);
    add_bool(settings, GOWHATSAPP_FAKE_ONLINE_OPTION,                 "Display offline contacts as away",                       TRUE);
    add_bool(settings, GOWHATSAPP_FETCH_CONTACTS_AFTER_LINKING_OPTION,"Fetch contacts from main device once after linking",     TRUE);
    add_bool(settings, GOWHATSAPP_REQUEST_CONTACTS_AFTER_LOGIN_OPTION,"Update contacts every time after login",                 TRUE);
    add_bool(settings, GOWHATSAPP_UPDATE_BUDDY_ON_MESSAGE_OPTION,     "Update buddy name when receiving a message",             TRUE);
    add_bool(settings, GOWHATSAPP_AUTO_JOIN_CHAT_OPTION,              "Automatically join all chats",                           FALSE);
    add_bool(settings, GOWHATSAPP_IGNORE_STATUS_BROADCAST_OPTION,     "Ignore status broadcasts",                               TRUE);
    add_bool(settings, GOWHATSAPP_BRIDGE_COMPATIBILITY_OPTION,        "Protocol bridge compatibility mode",                     FALSE);
    add_bool(settings, GOWHATSAPP_DISPLAY_MESSAGE_ID_OPTION,          "Display message ID",                                     FALSE);

    return settings;
}
