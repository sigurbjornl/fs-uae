#define FSEMU_INTERNAL
#include "fsemu-hud.h"

#include "fsemu-control.h"
#include "fsemu-fontcache.h"
#include "fsemu-glib.h"
#include "fsemu-gui.h"
#include "fsemu-image.h"
#include "fsemu-keyboard.h"
#include "fsemu-module.h"
#include "fsemu-option.h"
#include "fsemu-options.h"
#include "fsemu-thread.h"
#include "fsemu-time.h"
#include "fsemu-titlebar.h"
#include "fsemu-types.h"
#include "fsemu-util.h"
#include "fsemu-video.h"

typedef struct {
    fsemu_hud_id_t id;
    char *icon;
    char *title;
    char *subtitle;
    fsemu_image_t *icon_image;
    fsemu_image_t *title_image;
    fsemu_image_t *subtitle_image;

    fsemu_gui_item_t background_item;
    fsemu_gui_item_t icon_item;
    fsemu_gui_item_t title_item;
    fsemu_gui_item_t subtitle_item;

    bool visible;
    int64_t visible_after;
    int64_t visible_until;

    fsemu_point_t position;
    fsemu_point_t want_position;
} fsemu_hud_notice_t;

static struct {
    bool initialized;
    fsemu_font_t *notice_title_font;
    fsemu_font_t *notice_subtitle_font;
    uint32_t notice_background_color;
    uint32_t notice_title_color;
    uint32_t notice_subtitle_color;
    GList *notices;
    fsemu_hud_notice_t quitkey_notice;
    fsemu_hud_notice_t vsync_notice;
    fsemu_hud_notice_t cursor_notice;
    fsemu_hud_notice_t warp_notice;
    fsemu_hud_notice_t pause_notice;
    bool mouse_was_captured;
    int vsync_refresh_mismatch;

    // GList *notifications;
} fsemu_hud;

static void fsemu_hud_init_notice(fsemu_hud_notice_t *notice)
{
    int notice_x = 20;
    // 440 is a nice size because the notice is symmetrically positioned on the
    // 4:3 video edge on a 16:9 display. But a bit short.
    // int notice_width = 440;
    int notice_width = 512;
    int notice_height = 120;

    fsemu_gui_item_t *item;
    item = &notice->background_item;
    fsemu_gui_rectangle(item,
                        notice_x,
                        0,
                        notice_width,
                        notice_height,
                        fsemu_hud.notice_background_color);
    item->coordinates = FSEMU_COORD_1080P_LEFT;
    item->z_index = 5000;
    fsemu_gui_add_item(item);

    item = &notice->icon_item;
    item->coordinates = FSEMU_COORD_1080P_LEFT;
    item->z_index = 5001;
    fsemu_gui_add_item(item);

    item = &notice->title_item;
    item->coordinates = FSEMU_COORD_1080P_LEFT;
    item->z_index = 5001;
    fsemu_gui_add_item(item);

    item = &notice->subtitle_item;
    item->coordinates = FSEMU_COORD_1080P_LEFT;
    item->z_index = 5001;
    fsemu_gui_add_item(item);
}

static void fsemu_hud_update_notice(fsemu_hud_notice_t *notice)
{
    fsemu_gui_item_t *item;
    item = &notice->icon_item;
    if (notice->icon_image == NULL) {
        notice->icon_image = fsemu_image_load(notice->icon);
        fsemu_gui_image(item,
                        20,
                        0,
                        notice->icon_image->width,
                        notice->icon_image->height,
                        notice->icon_image);
    }

    item = &notice->title_item;
    if (notice->title_image == NULL) {
        notice->title_image =
            fsemu_font_render_text_to_image(fsemu_hud.notice_title_font,
                                            notice->title,
                                            fsemu_hud.notice_title_color);
        fsemu_gui_image(item,
                        20,
                        0,
                        notice->title_image->width,
                        notice->title_image->height,
                        notice->title_image);
    }

    item = &notice->subtitle_item;
    if (notice->subtitle_image == NULL) {
        notice->subtitle_image =
            fsemu_font_render_text_to_image(fsemu_hud.notice_subtitle_font,
                                            notice->subtitle,
                                            fsemu_hud.notice_subtitle_color);
        fsemu_gui_image(item,
                        20,
                        0,
                        notice->subtitle_image->width,
                        notice->subtitle_image->height,
                        notice->subtitle_image);
    }
}

static void fsemu_hud_set_notice_visible(fsemu_hud_notice_t *notice,
                                         bool visible)
{
    notice->background_item.visible = visible;
    notice->icon_item.visible = visible;
    notice->title_item.visible = visible;
    notice->subtitle_item.visible = visible;
    notice->visible = visible;
}

static void fsemu_hud_add_notice(fsemu_hud_notice_t *notice)
{
    fsemu_hud.notices = g_list_append(fsemu_hud.notices, notice);
}

static void fsemu_hud_init_and_add_notice(fsemu_hud_notice_t *notice)
{
    fsemu_hud_init_notice(notice);
    fsemu_hud_update_notice(notice);
    fsemu_hud_add_notice(notice);
}

static void fsemu_hud_init_standard_notices(void)
{
    fsemu_hud.pause_notice.icon = strdup("NotificationPause.png");
    fsemu_hud.pause_notice.title = strdup("Paused");
    fsemu_hud.pause_notice.subtitle =
        g_strdup_printf("%s+P to resume", FSEMU_KEYBOARD_MOD_NAME);
    fsemu_hud_init_and_add_notice(&fsemu_hud.pause_notice);

    fsemu_hud.warp_notice.icon = strdup("NotificationWarp.png");
    fsemu_hud.warp_notice.title = strdup("Fast forwarding");
    fsemu_hud.warp_notice.subtitle = g_strdup_printf(
        "%s+W to resume normal speed", FSEMU_KEYBOARD_MOD_NAME);
    fsemu_hud_init_and_add_notice(&fsemu_hud.warp_notice);

    fsemu_hud.quitkey_notice.icon = strdup("NotificationInfo.png");
    fsemu_hud.quitkey_notice.title =
        g_strdup_printf("Quit key is %s+Q", FSEMU_KEYBOARD_MOD_NAME);
    // fsemu_hud.quitkey_notice.subtitle = g_strdup_printf(
    //     "%s+F1 to view more shortcuts", FSEMU_KEYBOARD_MOD_NAME);
    fsemu_hud.quitkey_notice.subtitle =
        g_strdup_printf("[FIXME] to view more shortcuts");
    fsemu_hud_init_and_add_notice(&fsemu_hud.quitkey_notice);
    // fsemu_hud.quitkey_notice.visible_until = now + 10 * 1000 * 1000;

    fsemu_option_read_int(FSEMU_OPTION_VSYNC_REFRESH_MISMATCH,
                          &fsemu_hud.vsync_refresh_mismatch);

    fsemu_hud.vsync_notice.icon = strdup("NotificationWarning.png");
    fsemu_hud.vsync_notice.title = strdup("Vsync disabled");
    if (fsemu_hud.vsync_refresh_mismatch) {
        fsemu_hud.vsync_notice.subtitle = g_strdup_printf(
            "Desktop refresh rate is %d Hz", fsemu_hud.vsync_refresh_mismatch);
    } else {
        fsemu_hud.vsync_notice.subtitle = strdup("Vsync support coming later");
    }
    fsemu_hud_init_and_add_notice(&fsemu_hud.vsync_notice);

    fsemu_hud.cursor_notice.icon = strdup("NotificationCursor.png");
    fsemu_hud.cursor_notice.title = strdup("Mouse cursor captured");
    fsemu_hud.cursor_notice.subtitle = g_strdup_printf(
        "Middle click or %s+G to release", FSEMU_KEYBOARD_MOD_NAME);
    fsemu_hud_init_and_add_notice(&fsemu_hud.cursor_notice);

    // fsemu_hud.vsync_notice.visible_until = now + 10 * 1000 * 1000;
}

static void fsemu_hud_set_notice_position(fsemu_hud_notice_t *notice,
                                          int x,
                                          int y)
{
    notice->background_item.rect.x = x;
    notice->background_item.rect.y = y;

    notice->icon_item.rect.x = x + 20;
    notice->icon_item.rect.y = y + 20;

    notice->title_item.rect.x = x + 80 + 20 + 20;
    notice->title_item.rect.y = y + 18;

    notice->subtitle_item.rect.x = x + 80 + 20 + 20;
    notice->subtitle_item.rect.y = y + 58;
}

static void fsemu_hud_update_notice_positions(void)
{
    // FIXME: titlebar height -- not when using custom frame in windowed mode
    // int first_y = 40 + 20;
    int x = 20;
    int y = 20;
    /*
    fsemu_hud_set_notice_position(&fsemu_hud.quitkey_notice, x, y);
    y += 120 + 20;
    fsemu_hud_set_notice_position(&fsemu_hud.cursor_notice, x, y);
    y += 120 + 20;
    fsemu_hud_set_notice_position(&fsemu_hud.vsync_notice, x, y);
    y += 120 + 20;
    */

    GList *list_item = fsemu_hud.notices;
    while (list_item) {
        fsemu_hud_notice_t *notice = (fsemu_hud_notice_t *) list_item->data;
        if (notice->visible) {
            notice->want_position.x = x;
            notice->want_position.y = y;
            if (notice->position.x == 0 && notice->position.y == 0) {
                notice->position.x = notice->want_position.x;
                notice->position.y = notice->want_position.y;
            }

            // Quick hack here, (120 + 20) is a multiple of 4 so we do not have
            // to check for position exceeding target right now.
            if (notice->position.x < notice->want_position.x) {
                notice->position.x += 4;
            } else if (notice->position.x > notice->want_position.x) {
                notice->position.x -= 4;
            }

            // FIXME: Using fixed delta for positions isn't good for warp
            // mode rendering. Animations will be too fast.
            if (notice->position.y < notice->want_position.y) {
                notice->position.y += 20;
            } else if (notice->position.y > notice->want_position.y) {
                notice->position.y -= 20;
            }

            fsemu_hud_set_notice_position(
                notice, notice->position.x, notice->position.y);
            y += 120 + 20;
        }
        list_item = list_item->next;
    }
}

void fsemu_hud_show_notification(fsemu_hud_id_t notification_id,
                                 const char *title,
                                 const char *sub_title,
                                 const char *icon_name,
                                 int64_t duration_us)
{
    printf("[FSEMU] Notification (%016llx) %s\n",
           (long long) notification_id,
           title);
    // FIXME: When re-showing a notification that's already on screen,
    // maybe it could briefly flash? Maybe a parameter (flags?) to
    // fsemu_hud_show_notification? Also maybe a flag to flash a notification
    // the first time it appears as well?

    GList *list_item = fsemu_hud.notices;
    while (list_item) {
        fsemu_hud_notice_t *notice = (fsemu_hud_notice_t *) list_item->data;
        if (notice->id == notification_id) {
            printf("Found existing notification; extending duration\n");
            notice->visible_until = fsemu_time_us() + duration_us;
            return;
        }
        list_item = list_item->next;
    }

    fsemu_hud_notice_t *notice = FSEMU_UTIL_MALLOC0(fsemu_hud_notice_t);
    // FIXME: Ignoring icon_name for now
    notice->id = notification_id;
    notice->icon = strdup("NotificationWarning.png");
    notice->title = strdup(title);
    notice->subtitle = strdup(sub_title);
    notice->visible_until = fsemu_time_us() + duration_us;
    fsemu_hud_init_and_add_notice(notice);

    // fsemu_hud.notifications = g_list_append(fsemu_hud.notifications,
    // notice);
}

void fsemu_hud_update(void)
{
    static int64_t first_update_at;
    int64_t now = fsemu_time_us();
    if (first_update_at == 0) {
        first_update_at = now;
    }

    if (fsemu_hud.quitkey_notice.visible_until == 0) {
        // Initially show quit key notice after a small delay
        if (now - first_update_at > 2000 * 1000) {
            fsemu_hud.quitkey_notice.visible_until = now + 8000 * 1000;
        }
    }

    if (fsemu_video_vsync_prevented() || fsemu_hud.vsync_refresh_mismatch) {
        if (fsemu_hud.vsync_notice.visible_until == 0) {
            // Initially show after a small delay
            if (now - first_update_at > 2500 * 1000) {
                fsemu_hud.vsync_notice.visible_until = now + 7500 * 1000;
            }
        }
    }

    if (fsemu_mouse_captured()) {
        // Initially show cursor notice after a small delay
        if (now - first_update_at > 3000 * 1000) {
            if (!fsemu_hud.mouse_was_captured) {
                // FIXME: Shorter delays later
                // FIXME: Only long duration if cursor captured from start
                fsemu_hud.cursor_notice.visible_until = now + 7000 * 1000;
                fsemu_hud.mouse_was_captured = true;
            }
        }
    } else {
        fsemu_hud.cursor_notice.visible_until = 0;
        fsemu_hud.mouse_was_captured = false;
    }

    GList *list_item = fsemu_hud.notices;
    while (list_item) {
        fsemu_hud_notice_t *notice = (fsemu_hud_notice_t *) list_item->data;
        fsemu_hud_set_notice_visible(notice, notice->visible_until > now);

        // FIXME: Delete dynamically allocated notices

        list_item = list_item->next;
    }

    fsemu_hud_set_notice_visible(&fsemu_hud.pause_notice,
                                 fsemu_control_paused());
    if (!fsemu_control_paused()) {
        fsemu_hud_set_notice_visible(&fsemu_hud.warp_notice,
                                     fsemu_control_warp());
    }
    fsemu_hud_update_notice_positions();
}

static void fsemu_hud_quit(void)
{
    fsemu_log("fsemu_hud_quit\n");

    GList *listitem = fsemu_hud.notices;
    while (listitem) {
        fsemu_hud_notice_t *notice = (fsemu_hud_notice_t *) listitem->data;

        if (notice->icon_image) {
            fsemu_image_unref(notice->icon_image);
            notice->icon_image = NULL;
        }
        if (notice->title_image) {
            fsemu_image_unref(notice->title_image);
            notice->title_image = NULL;
        }
        if (notice->subtitle_image) {
            fsemu_image_unref(notice->subtitle_image);
            notice->subtitle_image = NULL;
        }

        // FIXME: Cannot delete notice atm. Some are statically allocated. Fix
        // this (make all dynamically allocated).

        listitem = listitem->next;
    }

    g_list_free(fsemu_hud.notices);
    fsemu_hud.notices = NULL;
}

void fsemu_hud_init(void)
{
    fsemu_thread_assert_main();
    if (fsemu_hud.initialized) {
        return;
    }
    fsemu_hud.initialized = true;
    fsemu_log("Initializing hud module\n");
    fsemu_module_on_quit(fsemu_hud_quit);

    // FIXME: Medium ?
    fsemu_hud.notice_title_font =
        fsemu_fontcache_font("SairaCondensed-SemiBold.ttf", 32);
    // FIXME: Semi-bold ?
    fsemu_hud.notice_subtitle_font =
        fsemu_fontcache_font("SairaCondensed-Medium.ttf", 28);

    // fsemu_hud.notice_background_color = FSEMU_RGBA(0x202020c0);
    fsemu_hud.notice_background_color = FSEMU_RGBA(0x404040c0);
    fsemu_hud.notice_title_color = FSEMU_RGBA(0xffffffff);
    fsemu_hud.notice_subtitle_color = FSEMU_RGBA(0xffffffaa);

    fsemu_hud_init_standard_notices();
}
