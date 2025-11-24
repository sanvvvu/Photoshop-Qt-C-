#include "GrayscaleBT709.h"
#include <QtGlobal>
#include <cmath>
#include <QtConcurrent>
#include <atomic>

void GrayscaleBT709::convert(QImage &img, std::function<void(int)> progressCallback) {
    if (img.isNull()) return;

    QImage src = img.convertToFormat(QImage::Format_ARGB32);
    int w = src.width();
    int h = src.height();

    QImage dst(src.size(), QImage::Format_Grayscale8);

    std::vector<int> rows(h);
    for (int i = 0; i < h; ++i) rows[i] = i;
    std::atomic<int> counter(0);

    // BT.709: Y = 0.2126 R + 0.7152 G + 0.0722 B
    QtConcurrent::blockingMap(rows.begin(), rows.end(), [&](int y){
        const QRgb *sline = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        uchar *dline = dst.scanLine(y);
        for (int x = 0; x < w; ++x) {
            QRgb p = sline[x];
            double Y = 0.2126 * qRed(p) + 0.7152 * qGreen(p) + 0.0722 * qBlue(p);
            int y8 = std::clamp(int(std::lround(Y)), 0, 255);
            dline[x] = static_cast<uchar>(y8);
        }
        if (progressCallback) {
            int cnt = ++counter;
            int perc = int((cnt * 100) / h);
            progressCallback(perc);
        }
    });

    if (progressCallback) progressCallback(100);
    img = dst;
}
