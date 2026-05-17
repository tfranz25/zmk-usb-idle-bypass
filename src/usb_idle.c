#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zmk/activity.h>
#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/rgb_underglow.h>

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
            return zmk_rgb_underglow_on();
        } else {
            return zmk_rgb_underglow_off();
        }
    } else {
        bool current_rgb_state = false;
        zmk_rgb_underglow_get_state(&current_rgb_state);
        sleep_state.rgb_state_before_sleeping = current_rgb_state;
        return zmk_rgb_underglow_off();
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
