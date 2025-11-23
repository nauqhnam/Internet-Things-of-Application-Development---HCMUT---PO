#ifndef __TINYML__
#define __TINYML__

// tinyml.h
#pragma once

#include <Arduino.h>

// Khởi tạo TinyML (nếu bạn muốn gọi riêng, không bắt buộc)
bool tinyml_init();

// FreeRTOS task chạy TinyML liên tục
void tiny_ml_task(void *pvParameters);


#endif