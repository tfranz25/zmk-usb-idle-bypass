#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zmk/activity.h>
#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>

#if IS_ENABLED(CONFIG_ZMK_RGB_PLUS)
#include <zmk_rgb_plus.h>
#elif IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
#include <zmk/rgb_underglow.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct rgb_idle_state {
    bool is_awake;
    bool rgb_state_before_sleeping;
};

static int handle_idle_state(bool target_wake_state) {
    static struct rgb_idle_state sleep_state = {
        .is_awake = true,
        .rgb_state_before_sleeping = false
    };

    if (target_wake_state == sleep_state.is_awake) {
        return 0;
    }
    sleep_state.is_awake = target_wake_state;

    if (sleep_state.is_awake) {
        if (sleep_state.rgb_state_before_sleeping) {
#if IS_ENABLED(CONFIG_ZMK_RGB_PLUS)
            return zmk_rgb_plus_on();
#elif IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
            return zmk_rgb_underglow_on();
#else
            return 0;
#endif
        } else {
#if IS_ENABLED(CONFIG_ZMK_RGB_PLUS)
            return zmk_rgb_plus_off();
#elif IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
            return zmk_rgb_underglow_off();
#else
            return 0;
#endif
        }
    } else {
        bool current_rgb_state = false;
#if IS_ENABLED(CONFIG_ZMK_RGB_PLUS)
        zmk_rgb_plus_get_state(&current_rgb_state);
#elif IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
        zmk_rgb_underglow_get_state(&current_rgb_state);
#endif
        sleep_state.rgb_state_before_sleeping = current_rgb_state;
#if IS_ENABLED(CONFIG_ZMK_RGB_PLUS)
        return zmk_rgb_plus_off();
#elif IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
        return zmk_rgb_underglow_off();
#else
        return 0;
#endif
    }
}

static int usb_idle_event_listener(const zmk_event_t *eh) {
    bool is_active = zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE;
    bool is_usb = zmk_usb_is_powered();

    bool target_wake_state = is_active || is_usb;

    return handle_idle_state(target_wake_state);
}

ZMK_LISTENER(usb_idle_bypass, usb_idle_event_listener);
ZMK_SUBSCRIPTION(usb_idle_bypass, zmk_activity_state_changed);
ZMK_SUBSCRIPTION(usb_idle_bypass, zmk_usb_conn_state_changed);
