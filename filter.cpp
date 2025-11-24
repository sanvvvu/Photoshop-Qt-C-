#include "filter.h"
#include <QImage>
#include <QtGlobal>
#include <cmath>
#include <algorithm>
#include <vector>

static inline int reflect_idx(int idx, int maxIdx) {
    if (maxIdx <= 0) return 0;
    if (idx < 0) idx = -idx - 1;
    if (idx > maxIdx) {
        int d = idx - maxIdx;
        idx = maxIdx - (d - 1);
    }
    if (idx < 0) idx = 0;
    if (idx > maxIdx) idx = maxIdx;
    return idx;
}

void filter2D(QImage &image, double *kernel, size_t kWidth, size_t kHeight) {
    if (image.isNull() || kernel == nullptr || kWidth == 0 || kHeight == 0) {
        return;
    }

    QImage src = image.convertToFormat(QImage::Format_ARGB32);
    QImage dst(src.size(), QImage::Format_ARGB32);

    const int w = src.width();
    const int h = src.height();

    const int kW = static_cast<int>(kWidth);
    const int kH = static_cast<int>(kHeight);
    const int kHalfW = kW / 2;
    const int kHalfH = kH / 2;

    std::vector<double> kbuf;
    kbuf.assign(kernel, kernel + (kW * kH));

    for (int y = 0; y < h; ++y) {
        QRgb *dstLine = reinterpret_cast<QRgb*>(dst.scanLine(y));
        for (int x = 0; x < w; ++x) {
            double accumR = 0.0;
            double accumG = 0.0;
            double accumB = 0.0;

            for (int ky = 0; ky < kH; ++ky) {
                int oy = ky - kHalfH;
                int yy = reflect_idx(y + oy, h - 1);
                const QRgb *srcLine = reinterpret_cast<const QRgb*>(src.constScanLine(yy));

                for (int kx = 0; kx < kW; ++kx) {
                    int ox = kx - kHalfW;
                    int xx = reflect_idx(x + ox, w - 1);

                    QRgb p = srcLine[xx];
                    double kval = kbuf[ky * kW + kx];

                    accumR += qRed(p)   * kval;
                    accumG += qGreen(p) * kval;
                    accumB += qBlue(p)  * kval;
                }
            }

            auto clamp8 = [](double v) -> int {
                if (std::isnan(v)) return 0;
                if (v < 0.0) return 0;
                if (v > 255.0) return 255;
                return static_cast<int>(std::lround(v));
            };

            int r = clamp8(accumR);
            int g = clamp8(accumG);
            int b = clamp8(accumB);

            dstLine[x] = qRgba(r, g, b, qAlpha(src.pixel(x, y)));
        }
    }

    image = dst;
}

void adjustContrast(QImage &image, double factor) {
    if (image.isNull()) return;

    QImage src = image.convertToFormat(QImage::Format_ARGB32);
    int w = src.width();
    int h = src.height();

    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(src.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int r = qRed(line[x]);
            int g = qGreen(line[x]);
            int b = qBlue(line[x]);

            r = std::clamp(int((r - 128) * factor + 128), 0, 255);
            g = std::clamp(int((g - 128) * factor + 128), 0, 255);
            b = std::clamp(int((b - 128) * factor + 128), 0, 255);

            line[x] = qRgba(r, g, b, qAlpha(line[x]));
        }
    }
    
    image = src;
}