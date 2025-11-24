/*  Находит оптимальный порог, который минимизирует внутриклассовую дисперсию (между фоновыми и объектными пикселями).
    Все пиксели выше порога становятся белыми, ниже — чёрными                                                           */
#ifndef OTSU_H
#define OTSU_H

#include <QImage>
#include <functional>

class Otsu {
public:
    static void binarize(QImage &img, std::function<void(int)> progressCallback = nullptr);
};

#endif
