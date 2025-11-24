#ifndef FILTER_H
#define FILTER_H

#include <QImage>
#include <cstddef>

// выполняет фильтрацию изображения image
// kernel - указатель на массив размера kWidth * kHeight в строковом порядке (row-major)
// поведение на границах - зеркальное отражение, чтобы не терять размеры
// Результат записывается в image (фильтрация in-place через временную копию)
void filter2D(QImage &image, double *kernel, size_t kWidth, size_t kHeight);
void adjustContrast(QImage &image, double factor); // регулировка контраста

#endif // FILTER_H
