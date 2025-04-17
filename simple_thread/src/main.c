// -------- Header Files --------
#include <zephyr/kernel.h>          // Core kernel APIs: threads, sleep, etc.
#include <zephyr/sys/printk.h>      // printk() for console logging
#include <zephyr/sys/atomic.h>      // For atomic sync counter
#include <zephyr/kernel_structs.h>  // For thread priority constants

// -------- Thread Configuration --------
#define STACK_SIZE 512
#define THREAD_PRIORITY_1 -1     // Preemptive thread (lower priority)
#define THREAD_PRIORITY_2 -2     // Preemptive thread (higher priority)

// -------- Thread Stack & Thread Structures --------
K_THREAD_STACK_DEFINE(stack1, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack2, STACK_SIZE);

struct k_thread thread1_data;
struct k_thread thread2_data;

// -------- Sync Tools --------
atomic_t sync_counter = ATOMIC_INIT(0);
struct k_mutex sync_mutex;

/**
 * @brief Print a newline when all threads (T1, T2, MAIN) have printed once.
 */
void sync_log_complete_cycle(void) {
    atomic_inc(&sync_counter);
    if (atomic_get(&sync_counter) >= 3) {
        k_mutex_lock(&sync_mutex, K_FOREVER);
        atomic_set(&sync_counter, 0);
        printk("\n");  // Clear visual marker between cycles
        k_mutex_unlock(&sync_mutex);
    }
}

// -------- Thread 1 Function --------
void thread1_fn(void *a, void *b, void *c)
{
    while (1) {
        printk("[T1] Low-priority thread (P:-1) running...\n");
        sync_log_complete_cycle();
        k_sleep(K_MSEC(700)); // Voluntarily yield CPU
    }
}

// -------- Thread 2 Function --------
void thread2_fn(void *a, void *b, void *c)
{
    while (1) {
        printk("[T2] High-priority thread (P:-2) working...\n");
        sync_log_complete_cycle();
        k_sleep(K_MSEC(1200)); // Yield to allow T1 to run
    }
}

// -------- Main Function --------
int main(void)
{
    printk("[MAIN] Starting Preemptive Thread Demo\n");
    k_mutex_init(&sync_mutex); // Initialize the mutex

    // Create Thread 1
    k_thread_create(&thread1_data, stack1,
        K_KERNEL_STACK_SIZEOF(stack1),
        thread1_fn,
        NULL, NULL, NULL,
        THREAD_PRIORITY_1, 0, K_NO_WAIT);
    printk("[MAIN] Thread 1 created (P:-1)\n");

    // Create Thread 2
    k_thread_create(&thread2_data, stack2,
        K_KERNEL_STACK_SIZEOF(stack2),
        thread2_fn,
        NULL, NULL, NULL,
        THREAD_PRIORITY_2, 0, K_NO_WAIT);
    printk("[MAIN] Thread 2 created (P:-2)\n");

    // Optional: Main thread can participate in cycle logging
    while (1) {
        printk("[MAIN] Monitoring system...\n");
        sync_log_complete_cycle();
        k_sleep(K_MSEC(2000));
    }

    return 0;
}

