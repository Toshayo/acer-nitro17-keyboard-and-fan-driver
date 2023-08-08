#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/acpi.h>

#define DRIVER_NAME "Acer Nitro 17 AN17-71 Fan & Keyboard Driver"
#define WMID_ACER_GUID "7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56"

#define ACER_NITRO17_FAN_CONFIG_LENGTH 1
#define ACER_NITRO17_KBD_CONFIG_LENGTH 7
#define ACER_NITRO17_KBD_INTERNAL_CONFIG_LENGTH 16
#define WMI_SET_GAMING_FAN_BEHAVIOR_ID 14
#define WMI_SET_GAMING_KBD_BACKLIGHT_ID 20

MODULE_AUTHOR("Toshayo");
MODULE_LICENSE("GPL");

/**
 * Generates unsigned long config to pass to SetGamingFanBehavior.
 * @param fan_mode - True if turbo mode, false otherwise.
 * @return - Config to pass to SetGamingFanBehavior.
 * @author https://github.com/JafarAkhondali
 */
static u64 get_fan_config(bool isTurbo) {
    u64 gpu_fan_config1 = 0, gpu_fan_config2 = 0;
    int i;

    gpu_fan_config2 |= 1;
    for (i = 0; i < 2; ++i)
        gpu_fan_config2 |= 1 << (i + 1);
    gpu_fan_config2 |= 1 << 3;
    gpu_fan_config1 |= isTurbo ? 2 : 1;
    for (i = 0; i < 2; ++i)
        gpu_fan_config1 |= (isTurbo ? 2 : 1) << (2 * i + 2);
    gpu_fan_config1 |= (isTurbo ? 2 : 1) << 6;

    return gpu_fan_config2 | gpu_fan_config1 << 16;
}

static int acer_nitro17_device_file_open(__attribute__((unused)) struct inode *file_ptr,
                                         __attribute__((unused)) struct file *user_buffer) {
    try_module_get(THIS_MODULE);
    return 0;
}

static int acer_nitro17_device_file_close(__attribute__((unused)) struct inode *file_ptr,
                                          __attribute__((unused)) struct file *user_buffer) {
    module_put(THIS_MODULE);
    return 0;
}

static ssize_t acer_nitro17_fan_device_file_read(__attribute__((unused)) struct file *file_ptr,
                                                 __attribute__((unused)) char __user *user_buffer,
                                                 __attribute__((unused)) size_t count,
                                                 __attribute__((unused)) loff_t *position) {
    return 0;
}

static ssize_t acer_nitro17_fan_device_file_write(__attribute__((unused)) struct file *file_ptr,
                                                  const char *user_buffer, size_t count,
                                                  __attribute__((unused)) loff_t *offset) {
    if (count != ACER_NITRO17_FAN_CONFIG_LENGTH && count != ACER_NITRO17_FAN_CONFIG_LENGTH + 1) {
        printk(KERN_ERR "%s: Invalid data written to fan character device!", DRIVER_NAME);
        return 0;
    }

    u8 config_buf[ACER_NITRO17_FAN_CONFIG_LENGTH];
    unsigned long err = copy_from_user(config_buf, user_buffer, ACER_NITRO17_FAN_CONFIG_LENGTH);
    if (err > 0) {
        printk(KERN_ERR "%s: Copying data from userspace failed with code: %lu", DRIVER_NAME, err);
        return 0;
    }

    u64 wmiInputs = get_fan_config(config_buf[0] == 1 || config_buf[0] == '1');
    struct acpi_buffer input = {(acpi_size) sizeof(u64), (void *) (&wmiInputs)};
    struct acpi_buffer output = {ACPI_ALLOCATE_BUFFER, NULL};
    acpi_status status = wmi_evaluate_method(WMID_ACER_GUID, 0, WMI_SET_GAMING_FAN_BEHAVIOR_ID, &input, &output);
    if (ACPI_FAILURE(status)) {
        printk(KERN_INFO "%s: acpi failure", DRIVER_NAME);
        return 0;
    }

    return ACER_NITRO17_FAN_CONFIG_LENGTH;
}

static ssize_t acer_nitro17_kdb_device_file_read(__attribute__((unused)) struct file *file_ptr,
                                                 __attribute__((unused)) char __user *user_buffer,
                                                 __attribute__((unused)) size_t count,
                                                 __attribute__((unused)) loff_t *position) {
    return 0;
}

static ssize_t
acer_nitro17_kbd_device_file_write(__attribute__((unused)) struct file *file_ptr,
                                   const char *user_buffer, size_t count,
                                   __attribute__((unused)) loff_t *offset) {
    if (count != ACER_NITRO17_KBD_CONFIG_LENGTH) {
        printk(KERN_ERR "%s: Invalid data written to keyboard character device!", DRIVER_NAME);
        return 0;
    }

    u8 config_buf[ACER_NITRO17_KBD_CONFIG_LENGTH];
    unsigned long err = copy_from_user(config_buf, user_buffer, ACER_NITRO17_KBD_CONFIG_LENGTH);
    if (err > 0) {
        printk(KERN_ERR "%s: Copying data from userspace failed with code: %lu", DRIVER_NAME, err);
        return 0;
    }

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
    if (config_buf[0] > 7) goto error;

    // Brightness 0-100. 0 is off and 100 is max.
    if (config_buf[2] > 100) goto error;

    if(config_buf[0] == 0) {
        // Speed is 0 when in static mode.
        if (config_buf[1] != 0) goto error;

        // Direction is 0 when in static mode
        if (config_buf[3] != 0) goto error;
    } else {
        // Speed 1-9. 1 is slowest, 9 fastest.
        if (config_buf[1] < 1 || config_buf[1] > 9) goto error;

        if(config_buf[0] == 3 || config_buf[0] == 4) {
            // Direction 1-2. 1 is left-to-right and 2 is right-to-left.
            if (config_buf[3] > 1) goto error;
        } else {
            // Direction should be 1.
            if (config_buf[3] != 1) goto error;
        }
    }

    // 3 color components are 0-255 so no need to test them.

    u8 wmiInputs[] = {
            config_buf[0], // Backlight mode
            config_buf[1], // Speed
            config_buf[2], // Brightness
            0, // Unused/unknown
            config_buf[3] + 1, // Direction
            config_buf[4], // Red component
            config_buf[5], // Green component
            config_buf[6], // Blue component
            0, // Unknown, was 3 when I used GetGamingKBBacklight on windows
            255, // Unknown, should be > 0 or else nothing changes
            0, 0, 0, 0, 0, 0 // Unknown/unused
    };
    struct acpi_buffer input = {(acpi_size) ACER_NITRO17_KBD_INTERNAL_CONFIG_LENGTH, (void *) (&wmiInputs)};
    struct acpi_buffer output = {ACPI_ALLOCATE_BUFFER, NULL};
    acpi_status status;
    status = wmi_evaluate_method(WMID_ACER_GUID, 0, WMI_SET_GAMING_KBD_BACKLIGHT_ID, &input, &output);
    if (ACPI_FAILURE(status)) {
        printk(KERN_INFO "%s: acpi failure", DRIVER_NAME);
        return 0;
    }

    return ACER_NITRO17_KBD_CONFIG_LENGTH;

    error:
    printk(KERN_ERR "%s: Invalid data written to keyboard character device!", DRIVER_NAME);
    return 0;
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
            acer_nitro17_fan_cls = class_create(THIS_MODULE, "acer-nitro17_fan");
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
            acer_nitro17_kbd_cls = class_create(THIS_MODULE, "acer-nitro17_kbd");
            device_create(acer_nitro17_kbd_cls, NULL, dev, NULL, "acer-nitro17_kbd");
        } else {
            printk(KERN_WARNING "%s: failed to add keyboard character device", DRIVER_NAME);
            return -1;
        }
    } else {
        printk(KERN_WARNING "%s: failed to allocate keyboard character device region", DRIVER_NAME);
        return -1;
    }

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
