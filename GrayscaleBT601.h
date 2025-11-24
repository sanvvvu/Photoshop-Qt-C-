#ifndef GRAYSCALEBT601_H
#define GRAYSCALEBT601_H

#include <QImage>
#include <functional>

class GrayscaleBT601 {
public:
    static void convert(QImage &img, std::function<void(int)> progressCallback = nullptr);
};

#endif
