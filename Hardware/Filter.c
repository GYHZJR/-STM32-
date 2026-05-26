#include "Filter.h"

// 滑动平均滤波：循环队列 + 增量求和，O(1) 复杂度
static float buf[FILTER_SIZE];
static uint8_t idx;
static uint8_t count;
static float sum;

void Filter_Init(void) {
    uint8_t i;
    for (i = 0; i < FILTER_SIZE; i++) buf[i] = 0.0f;
    idx = 0;
    count = 0;
    sum = 0.0f;
}

// 新值加入：弹出最旧值，压入新值，返回当前均值
float Filter_Add(float newValue) {
    sum -= buf[idx];
    buf[idx] = newValue;
    sum += buf[idx];
    idx = (idx + 1) % FILTER_SIZE;
    if (count < FILTER_SIZE) count++;  // 未满时用实际数量求平均
    return sum / count;
}

float Filter_Get(void) {
    if (count == 0) return 0.0f;
    return sum / count;
}
