#ifndef GRAYSCALEBT709_H
#define GRAYSCALEBT709_H

#include <QImage>
#include <functional>

class GrayscaleBT709 {
public:
    static void convert(QImage &img, std::function<void(int)> progressCallback = nullptr);
};

#endif
