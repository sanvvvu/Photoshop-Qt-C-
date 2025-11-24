// локальная бинаризация, рассчитывает локальный порог по медиане и среднеквадратическому отклонению.
#ifndef ISOMAD_H
#define ISOMAD_H

#include <QImage>
#include <functional>

class ISOMAD {
public:
    static void binarize(QImage &img, int win = 15, std::function<void(int)> progressCallback = nullptr);
};

#endif
