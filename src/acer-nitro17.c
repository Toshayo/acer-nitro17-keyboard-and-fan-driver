#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/acpi.h>

#define DRIVER_NAME "Acer Nitro 17 AN17-71 Fan & Keyboard Driver"
#define WMID_ACER_GUID "7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56"

#define ACER_NITRO17_FAN_CONFIG_LENGTH 3
#define ACER_NITRO17_KBD_CONFIG_LENGTH 7
#define ACER_NITRO17_KBD_INTERNAL_CONFIG_LENGTH 16
#define WMI_SET_GAMING_FAN_BEHAVIOR_ID 14
#define WMI_SET_GAMING_FAN_SPEED_ID 16
#define WMI_SET_GAMING_KBD_BACKLIGHT_ID 20

MODULE_AUTHOR("Toshayo");
MODULE_LICENSE("GPL");

static int initial_color = 0xFFFFFF;
module_param(initial_color, int, 0);
MODULE_PARM_DESC(initial_color, "Initial keyboard color.");

/**
 * Generates unsigned long config to pass to SetGamingFanBehavior.
 * @param fanMode - 1 if auto mode, 2 is turbo and 3 is manual.
 * @return - Config to pass to SetGamingFanBehavior.
 * @author https://github.com/JafarAkhondali
 */
static u64 acer_nitro17_get_fan_config(u8 fanMode) {
    if (fanMode > 3)
        return 0;
    u64 gpu_fan_config1 = 0, gpu_fan_config2 = 0;
    int i;

    gpu_fan_config2 |= 1;
    for (i = 0; i < 2; ++i)
        gpu_fan_config2 |= 1 << (i + 1);
    gpu_fan_config2 |= 1 << 3;
    gpu_fan_config1 |= fanMode;
    for (i = 0; i < 2; ++i)
        gpu_fan_config1 |= fanMode << (2 * i + 2);
    gpu_fan_config1 |= fanMode << 6;

    return gpu_fan_config2 | gpu_fan_config1 << 16;
}

/**
 * Generates unsigned long config to pass to SetGamingFanSpeed.
 * @param fanIndex - Fan index (0-1).
 * @param speed - Speed percentage (0-100).
 * @return - Config to pass to SetGamingFanSpeed.
 */
static u64 acer_nitro17_get_fan_speed_config(u8 fanIndex, u8 speed) {
    if (fanIndex > 1 || speed > 100)
        return 0;
    return (speed << 8) + (fanIndex == 0 ? 1 : 0b100);
}

static bool acer_nitro17_execute_wmi(u32 methodId, struct acpi_buffer input) {
    struct acpi_buffer output = {ACPI_ALLOCATE_BUFFER, NULL};
    acpi_status status = wmi_evaluate_method(WMID_ACER_GUID, 0, methodId, &input, &output);
    if (ACPI_FAILURE(status)) {
        printk(KERN_INFO "%s: acpi failure", DRIVER_NAME);
        return false;
    }
    return true;
}

static bool acer_nitro17_set_keyboard_config(uint8_t backlight_mode, uint8_t brightness, uint8_t speed, uint8_t direction, uint32_t color) {
    /* Mode 0-7.
     * Here are mode descriptions:
     *     0 - Static.     Speed is 0.  Direction is 0.  Color required.
     *     1 - Breathing.  Speed 1-9.   Direction is 1.  Color required.
     *     2 - Neon.       Speed 1-9.   Direction is 1.  No color.
     *     3 - Wave.       Speed 1-9.   Direction 1-2.   No color.
     *     4 - Shifting.   Speed 1-9.   Direction 1-2.   Color required.
     *     5 - Zoom.       Speed 1-9.   Direction is 1.  Color required.
     *     6 - Meteor.     Speed 1-9.   Direction is 1.  Color required.
     *     7 - Twinkling.  Speed 1-9.   Direction is 1.  Color required.
     */
    if(backlight_mode > 7) {
        return false;
    }

    // Brightness 0-100. 0 is off and 100 is max.
    if(brightness > 100) {
        return false;
    }

    if (backlight_mode == 0) {
        // Speed is 0 when in static mode.
        if (speed != 0) {
            return false;
        }

        // Direction is 0 when in static mode
        if (direction != 0) {
            return false;
        }
    } else {
        // Speed 1-9. 1 is slowest, 9 fastest.
        if (speed < 1 || speed > 9) {
            return false;
        }

        if (backlight_mode == 3 || backlight_mode == 4) {
            // Direction 1-2. 1 is left-to-right and 2 is right-to-left.
            if (direction != 1 && direction != 2) {
                return false;
            }
        } else {
            // Direction should be 1.
            if (direction != 1) {
                return false;
            }
        }
    }

    if(backlight_mode == 0) {
        u8 wmiInputs[] = {
                7, // Backlight mode
                1, // Speed
                1, // Brightness
                0, // Unused/unknown
                direction, // Direction
                (color >> 16) & 0xFF, // Red component
                (color >> 8) & 0xFF, // Green component
                color & 0xFF, // Blue component
                0, // Unknown, was 3 when I used GetGamingKBBacklight on windows
                255, // Unknown, should be > 0 or else nothing changes
                0, 0, 0, 0, 0, 0 // Unknown/unused
        };
        struct acpi_buffer input = {(acpi_size) ACER_NITRO17_KBD_INTERNAL_CONFIG_LENGTH, (void *) (&wmiInputs)};
        if (!acer_nitro17_execute_wmi(WMI_SET_GAMING_KBD_BACKLIGHT_ID, input)) {
            return false;
        }
    }
    u8 wmiInputs[] = {
            backlight_mode,
            speed,
            brightness,
            0, // Unused/unknown
            direction, // Direction
            (color >> 16) & 0xFF, // Red component
            (color >> 8) & 0xFF, // Green component
            color & 0xFF, // Blue component
            0, // Unknown, was 3 when I used GetGamingKBBacklight on windows
            255, // Unknown, should be > 0 or else nothing changes
            0, 0, 0, 0, 0, 0 // Unknown/unused
    };
    struct acpi_buffer input = {(acpi_size) ACER_NITRO17_KBD_INTERNAL_CONFIG_LENGTH, (void *) (&wmiInputs)};
    if (!acer_nitro17_execute_wmi(WMI_SET_GAMING_KBD_BACKLIGHT_ID, input)) {
        return false;
    }
    return true;
}

static int acer_nitro17_device_file_open(struct inode *file_ptr, struct file *user_buffer) {
    try_module_get(THIS_MODULE);
    return 0;
}

static int acer_nitro17_device_file_close(struct inode *file_ptr, struct file *user_buffer) {
    module_put(THIS_MODULE);
    return 0;
}

static ssize_t acer_nitro17_fan_device_file_read(struct file *file_ptr, char __user *user_buffer, size_t count, loff_t *position) {
    return 0;
}

static ssize_t acer_nitro17_fan_device_file_write(struct file *file_ptr, const char *user_buffer, size_t count, loff_t *offset) {
    if (count != ACER_NITRO17_FAN_CONFIG_LENGTH) {
        printk(KERN_ERR "%s: Invalid data written to fan character device!", DRIVER_NAME);
        return count >= SSIZE_MAX ? SSIZE_MAX : (ssize_t) count;
    }

    u8 config_buf[ACER_NITRO17_FAN_CONFIG_LENGTH];
    unsigned long err = copy_from_user(config_buf, user_buffer, ACER_NITRO17_FAN_CONFIG_LENGTH);
    if (err > 0) {
        printk(KERN_ERR "%s: Copying data from userspace failed with code: %lu", DRIVER_NAME, err);
        return 0;
    }

    u64 wmiInputs;
    struct acpi_buffer input = {(acpi_size) sizeof(u64), (void *) (&wmiInputs)};
    switch (config_buf[0]) {
        case 0:
        case 1:
            wmiInputs = acer_nitro17_get_fan_config(config_buf[0] + 1);
            if (!acer_nitro17_execute_wmi(WMI_SET_GAMING_FAN_BEHAVIOR_ID, input)) {
                return ACER_NITRO17_FAN_CONFIG_LENGTH;
            }
            break;
        case 2:
            // Fan index 0-1, speed 0-100
            if (config_buf[1] > 1 || config_buf[2] > 100) {
                goto error;
            }
            wmiInputs = acer_nitro17_get_fan_config(3);
            if (!acer_nitro17_execute_wmi(WMI_SET_GAMING_FAN_BEHAVIOR_ID, input)) {
                return ACER_NITRO17_FAN_CONFIG_LENGTH;
            }
            wmiInputs = acer_nitro17_get_fan_speed_config(config_buf[1], config_buf[2]);
            if (!acer_nitro17_execute_wmi(WMI_SET_GAMING_FAN_SPEED_ID, input)) {
                return ACER_NITRO17_FAN_CONFIG_LENGTH;
            }
            break;
        default:
            goto error;
    }

    return ACER_NITRO17_FAN_CONFIG_LENGTH;

    error:
    printk(KERN_ERR "%s: Invalid data written to fan character device!", DRIVER_NAME);
    return ACER_NITRO17_FAN_CONFIG_LENGTH;
}

static ssize_t acer_nitro17_kdb_device_file_read(struct file *file_ptr, char __user *user_buffer, size_t count, loff_t *position) {
    return 0;
}

static ssize_t acer_nitro17_kbd_device_file_write(struct file *file_ptr, const char *user_buffer, size_t count, loff_t *offset) {
    if (count != ACER_NITRO17_KBD_CONFIG_LENGTH) {
        printk(KERN_ERR "%s: Invalid data written to keyboard character device!", DRIVER_NAME);
        return count >= SSIZE_MAX ? SSIZE_MAX : (ssize_t) count;
    }

    u8 config_buf[ACER_NITRO17_KBD_CONFIG_LENGTH];
    unsigned long err = copy_from_user(config_buf, user_buffer, ACER_NITRO17_KBD_CONFIG_LENGTH);
    if (err > 0) {
        printk(KERN_ERR "%s: Copying data from userspace failed with code: %lu", DRIVER_NAME, err);
        return 0;
    }

    if(!acer_nitro17_set_keyboard_config(config_buf[0], config_buf[2], config_buf[1], config_buf[3], (config_buf[4] << 16) + (config_buf[5] << 8) + config_buf[6])) {
        printk(KERN_ERR "%s: Invalid data written to keyboard character device!", DRIVER_NAME);
    }

    return ACER_NITRO17_KBD_CONFIG_LENGTH;
}

static struct file_operations acer_nitro17_fan_driver_file_options = { // NOLINT(*-interfaces-global-init)
        .owner   = THIS_MODULE,
        .open    = acer_nitro17_device_file_open,
        .release = acer_nitro17_device_file_close,
        .write   = acer_nitro17_fan_device_file_write,
        .read    = acer_nitro17_fan_device_file_read
};

static int acer_nitro17_fan_major_number = 0;
static struct cdev acer_nitro17_fan_cdev;
static struct class *acer_nitro17_fan_cls;

static struct file_operations acer_nitro17_kbd_driver_file_options = { // NOLINT(*-interfaces-global-init)
        .owner   = THIS_MODULE,
        .open    = acer_nitro17_device_file_open,
        .release = acer_nitro17_device_file_close,
        .write   = acer_nitro17_kbd_device_file_write,
        .read    = acer_nitro17_kdb_device_file_read
};

static int acer_nitro17_kbd_major_number = 0;
static struct cdev acer_nitro17_kbd_cdev;
static struct class *acer_nitro17_kbd_cls;

static int __init acer_nitro17_init(void) {
    dev_t dev;

    // Init fan character device
    if (alloc_chrdev_region(&dev, 0, 1, "acer-nitro17_fan") == 0) {
        acer_nitro17_fan_major_number = MAJOR(dev);
        cdev_init(&acer_nitro17_fan_cdev, &acer_nitro17_fan_driver_file_options);

        if (cdev_add(&acer_nitro17_fan_cdev, dev, 1) == 0) {
            acer_nitro17_fan_cls = class_create("acer-nitro17_fan");
            device_create(acer_nitro17_fan_cls, NULL, dev, NULL, "acer-nitro17_fan");
        } else {
            printk(KERN_WARNING "%s: failed to add fan character device", DRIVER_NAME);
            return -1;
        }
    } else {
        printk(KERN_WARNING "%s: failed to allocate fan character device region", DRIVER_NAME);
        return -1;
    }

    // Init keyboard character device
    if (alloc_chrdev_region(&dev, 0, 1, "acer-nitro17_kbd") == 0) {
        acer_nitro17_kbd_major_number = MAJOR(dev);
        cdev_init(&acer_nitro17_kbd_cdev, &acer_nitro17_kbd_driver_file_options);

        if (cdev_add(&acer_nitro17_kbd_cdev, dev, 1) == 0) {
            acer_nitro17_kbd_cls = class_create("acer-nitro17_kbd");
            device_create(acer_nitro17_kbd_cls, NULL, dev, NULL, "acer-nitro17_kbd");
        } else {
            printk(KERN_WARNING "%s: failed to add keyboard character device", DRIVER_NAME);
            return -1;
        }
    } else {
        printk(KERN_WARNING "%s: failed to allocate keyboard character device region", DRIVER_NAME);
        return -1;
    }

    acer_nitro17_set_keyboard_config(0, 100, 0, 0, initial_color);

    printk(KERN_INFO "%s: module loaded", DRIVER_NAME);
    return 0;
}

static void __exit acer_nitro17_exit(void) {
    // Destroy fan character device
    dev_t dev = MKDEV(acer_nitro17_fan_major_number, 0);
    device_destroy(acer_nitro17_fan_cls, dev);
    class_destroy(acer_nitro17_fan_cls);
    cdev_del(&acer_nitro17_fan_cdev);
    unregister_chrdev_region(dev, 1);

    // Destroy keyboard character device
    dev = MKDEV(acer_nitro17_kbd_major_number, 0);
    device_destroy(acer_nitro17_kbd_cls, dev);
    class_destroy(acer_nitro17_kbd_cls);
    cdev_del(&acer_nitro17_kbd_cdev);
    unregister_chrdev_region(dev, 1);

    printk(KERN_INFO "%s: module unloaded", DRIVER_NAME);
}

module_init(acer_nitro17_init)

module_exit(acer_nitro17_exit)
