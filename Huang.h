// Минимизирует определённую функцию энтропии, улучшая качество при низком контрасте.
#ifndef HUANG_H
#define HUANG_H

#include <QImage>
#include <functional>

class Huang {
public:
    static void binarize(QImage &img, std::function<void(int)> progressCallback = nullptr);
};

#endif // HUANG_H
