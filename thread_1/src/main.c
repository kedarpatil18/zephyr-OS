// ============================
// Zephyr LED Multithread Demo
// ============================
//
// This program demonstrates multithreading in Zephyr OS by blinking
// three LEDs (led0, led1, led2) with different patterns using 3 threads.
// Each LED is controlled by a dedicated thread, and the program logs
// the behavior with structured printk debug output.
//
// Output is synchronized to print one complete cycle of T1, T2, and T3
// followed by a blank line for clarity.
// ============================


// -------- Header Files --------

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/atomic.h>

// -------- Devicetree Aliases for LEDs --------

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)

// -------- Thread Configuration --------

#define STACK_SIZE 512
#define PRIORITY 5

// -------- Thread Stacks and Thread Data --------

K_THREAD_STACK_DEFINE(led0_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(led1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(led2_stack, STACK_SIZE);

struct k_thread led0_thread_data;
struct k_thread led1_thread_data;
struct k_thread led2_thread_data;

// -------- Global LED Device & Pin Variables --------

const struct device *led0_dev;
const struct device *led1_dev;
const struct device *led2_dev;
int led0_pin, led1_pin, led2_pin;

// -------- Synchronization --------

atomic_t sync_counter = ATOMIC_INIT(0);
struct k_mutex sync_mutex;

void sync_log_complete_cycle(void) {
    atomic_inc(&sync_counter);
    if (atomic_get(&sync_counter) >= 3) {
        k_mutex_lock(&sync_mutex, K_FOREVER);
        atomic_set(&sync_counter, 0);
        printk("\n");
        k_mutex_unlock(&sync_mutex);
    }
}

// -------- LED Initialization Function --------

int init_led(const struct device **dev, int *pin, const struct gpio_dt_spec *led_spec) {
    if (!device_is_ready(led_spec->port)) {
        printk("[INIT] Error: LED device not ready\n");
        return -1;
    }

    *dev = led_spec->port;
    *pin = led_spec->pin;

    int ret = gpio_pin_configure(*dev, *pin, GPIO_OUTPUT_INACTIVE | led_spec->dt_flags);
    if (ret < 0) {
        printk("[INIT] Error configuring pin %d\n", *pin);
        return ret;
    }

    gpio_pin_set(*dev, *pin, 0);
    return 0;
}

// -------- Thread 1: LED0 Pattern --------

void led0_thread(void *arg1, void *arg2, void *arg3) {
    while (1) {
        gpio_pin_toggle(led0_dev, led0_pin);
        bool state = gpio_pin_get(led0_dev, led0_pin);
        printk("[T1] LED0 is now %s | [T1] sleep 500ms\n", state ? "ON" : "OFF");
        sync_log_complete_cycle();
        k_msleep(500);
    }
}

// -------- Thread 2: LED1 Pattern --------

void led1_thread(void *arg1, void *arg2, void *arg3) {
    while (1) {
        gpio_pin_toggle(led1_dev, led1_pin);
        bool state = gpio_pin_get(led1_dev, led1_pin);
        printk("[T2] LED1 is now %s | [T2] sleep 200ms\n", state ? "ON" : "OFF");
        sync_log_complete_cycle();
        k_msleep(200);
    }
}

// -------- Thread 3: LED2 Burst Pattern --------

void led2_thread(void *arg1, void *arg2, void *arg3) {
    while (1) {
        gpio_pin_set(led2_dev, led2_pin, 1);
        printk("[T3] Blink 1: LED2 ON | ");
        k_msleep(100);

        gpio_pin_set(led2_dev, led2_pin, 0);
        printk("[T3] Blink 2: LED2 OFF | [T3] sleep 800ms\n");
        sync_log_complete_cycle();
        k_msleep(800);
    }
}



// -------- Main Function --------


int main(void) {
    printk("[MAIN] Starting LED thread demo\n");

    static const struct gpio_dt_spec led0_spec = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
    static const struct gpio_dt_spec led1_spec = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
    static const struct gpio_dt_spec led2_spec = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

    if (init_led(&led0_dev, &led0_pin, &led0_spec) < 0 ||
        init_led(&led1_dev, &led1_pin, &led1_spec) < 0 ||
        init_led(&led2_dev, &led2_pin, &led2_spec) < 0) {
        printk("[MAIN] LED initialization failed\n");
        return 0;
    }

    k_mutex_init(&sync_mutex);

    printk("[LED0] Initialized on pin %d\n", led0_pin);
    printk("[LED1] Initialized on pin %d\n", led1_pin);
    printk("[LED2] Initialized on pin %d\n\n", led2_pin);

    printk("[T1] LED0 thread started (500ms toggle)\n");
    k_thread_create(&led0_thread_data, led0_stack, STACK_SIZE,
                    led0_thread, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    printk("[T2] LED1 thread started (200ms toggle)\n");
    k_thread_create(&led1_thread_data, led1_stack, STACK_SIZE,
                    led1_thread, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    printk("[T3] LED2 thread started (burst 2x then 800ms rest)\n\n");
    k_thread_create(&led2_thread_data, led2_stack, STACK_SIZE,
                    led2_thread, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);
	return 0;
}
