#include <Arduino.h>
#include <lvgl.h>

// LVGL custom memory management for ESP32 family boards - use PSRAM instead of DRAM.

void lv_mem_init(void) {
    return;
}

void lv_mem_deinit(void) {
    return;
}

lv_mem_pool_t lv_mem_add_pool(void * mem, size_t bytes) {
    return NULL;
}

void lv_mem_remove_pool(lv_mem_pool_t pool) {
    return;
}

void * lv_malloc_core(size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
}

void * lv_realloc_core(void * p, size_t new_size) {
    return heap_caps_realloc(p, new_size, MALLOC_CAP_8BIT);
}

void lv_free_core(void * p){
    return heap_caps_free(p);
}

void lv_mem_monitor_core(lv_mem_monitor_t * mon_p) {
    return;
}

lv_result_t lv_mem_test_core(void) {
    return LV_RESULT_OK;
}
