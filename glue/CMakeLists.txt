set(TARGET_NAME whatsmeow)
# All glue/ translation units now compile against libpurple 3. Individual
# files still carry TODO markers for features that depend on libpurple 3
# subsystems we have not finished wiring up yet (outbound presence on
# PurpleConnectionClass::set_presence, PurpleProtocolFileTransfer.receive_async,
# inline image rendering via PurpleAttachments / PurpleImage, the
# "conversation-updated" read-receipt heuristic, etc).
add_library(${TARGET_NAME} SHARED
    "gowhatsapp.h"
    "init.c"
    "purplegowhatsappconnection.c"
    "purplegowhatsappconnection.h"
    "purplegowhatsappprotocol.c"
    "purplegowhatsappprotocol.h"
    "bridge.c"
    "../bridge.c" # this needs to be compiled again since go's c-archive mode does not include the C object
    "../bridge.h"
    "pixbuf.c"
    "pixbuf.h"
    "process_message.c"
    "options.c"
    "display_message.c"
    "blist.c"
    "send_message.c"
    "qrcode.c"
    "receipt.c"
    "login.c"
    "commands.c"
    "presence.c"
    "groups.c"
    "send_file.c"
    "handle_attachment.c"
)
add_dependencies(${TARGET_NAME} libwhatsmeow)

target_compile_definitions(${TARGET_NAME} PRIVATE PLUGIN_VERSION=${VERSION})
target_include_directories(${TARGET_NAME} PRIVATE ${GO_WHATSAPP_INCLUDE_DIRS} ${PURPLE_INCLUDE_DIRS} ${PIXBUF_INCLUDE_DIRS} ${OPUSFILE_INCLUDE_DIRS})
target_link_directories(${TARGET_NAME} PRIVATE ${PURPLE_LIBRARY_DIRS} ${PIXBUF_LIBRARY_DIRS})
target_link_libraries(${TARGET_NAME} PRIVATE ${GO_WHATSAPP_LIBRARIES} ${PURPLE_LIBRARIES} ${PIXBUF_LIBRARIES} ${OPUSFILE_LINK_LIBRARIES})
set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "lib")
if (WIN32 AND CMAKE_C_COMPILER_ID STREQUAL GNU AND CMAKE_C_COMPILER_VERSION VERSION_GREATER 4.7.2) # might actually work without this up to 7.1.0, but I did not check
    target_link_options(${TARGET_NAME} PRIVATE "-static-libgcc")
endif()
if (APPLE)
    target_link_libraries(${TARGET_NAME} PRIVATE "-framework CoreFoundation" "-framework Security" resolv)
endif()

if (WIN32)
    install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION "${PURPLE_PLUGIN_DIR}")
else()
    install(TARGETS ${TARGET_NAME} DESTINATION "${PURPLE_PLUGIN_DIR}")
endif()
