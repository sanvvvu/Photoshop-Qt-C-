// локальная бинаризация, каждый пиксель сравнивается с локальным порогом, а не глобальным.
#ifndef NIBLACK_H
#define NIBLACK_H

#include <QImage>
#include <functional>

class Niblack {
public:
    // win — нечётное; k — коэффициент 
    static void binarize(QImage &img, int win = 15, double k = -0.2, std::function<void(int)> progressCallback = nullptr);
};

#endif 