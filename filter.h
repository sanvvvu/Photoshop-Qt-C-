#ifndef FILTER_H
#define FILTER_H

#include <QImage>
#include <cstddef>
#include <functional>

// выполняет фильтрацию изображения image
// kernel - указатель на массив размера kWidth * kHeight в строковом порядке (row-major)
// progressCb(percent) — callback для обновления прогресса (0-100), может быть nullptr
// поведение на границах - зеркальное отражение, чтобы не терять размеры
// Результат записывается в image (фильтрация in-place через временную копию)
void filter2D(QImage &image, double *kernel, size_t kWidth, size_t kHeight,
              std::function<void(int)> progressCb = nullptr);

// регулировка контраста с плавным прогрессом
void adjustContrast(QImage &image, double factor,
                    std::function<void(int)> progressCb = nullptr);

#endif
