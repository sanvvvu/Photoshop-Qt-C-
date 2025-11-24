#include "filter.h"
#include <QImage>
#include <QtGlobal>
#include <cmath>
#include <algorithm>
#include <vector>
#include <QtConcurrent>
#include <atomic>

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

void filter2D(QImage &image, double *kernel, size_t kWidth, size_t kHeight,
              std::function<void(int)> progressCb)
{
    if (image.isNull() || kernel == nullptr || kWidth == 0 || kHeight == 0) {
        if (progressCb) progressCb(100);
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

    std::vector<double> kbuf(kernel, kernel + (kW * kH));

    auto clamp8 = [](double v) -> int {
        if (std::isnan(v)) return 0;
        if (v < 0.0) return 0;
        if (v > 255.0) return 255;
        return static_cast<int>(std::lround(v));
    };

    std::vector<int> rows(h);
    for (int i = 0; i < h; ++i) rows[i] = i;
    std::atomic<int> counter(0);

    QtConcurrent::blockingMap(rows.begin(), rows.end(), [&](int y) {
        QRgb *dstLine = reinterpret_cast<QRgb*>(dst.scanLine(y));
        for (int x = 0; x < w; ++x) {
            double accumR = 0.0, accumG = 0.0, accumB = 0.0;

            for (int ky = 0; ky < kH; ++ky) {
                int yy = reflect_idx(y + ky - kHalfH, h - 1);
                const QRgb *srcLine = reinterpret_cast<const QRgb*>(src.constScanLine(yy));
                for (int kx = 0; kx < kW; ++kx) {
                    int xx = reflect_idx(x + kx - kHalfW, w - 1);
                    double kval = kbuf[ky * kW + kx];
                    QRgb p = srcLine[xx];
                    accumR += qRed(p)   * kval;
                    accumG += qGreen(p) * kval;
                    accumB += qBlue(p)  * kval;
                }
            }

            int r = clamp8(accumR);
            int g = clamp8(accumG);
            int b = clamp8(accumB);
            dstLine[x] = qRgba(r, g, b, qAlpha(src.pixel(x, y)));
        }

        if (progressCb) {
            int percent = static_cast<int>((counter.fetch_add(1) + 1) * 100.0 / h);
            progressCb(percent);
        }
    });

    image = dst;
}

void adjustContrast(QImage &image, double factor,
                    std::function<void(int)> progressCb)
{
    if (image.isNull()) {
        if (progressCb) progressCb(100);
        return;
    }

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

        if (progressCb) {
            int percent = static_cast<int>((y + 1) * 100.0 / h);
            progressCb(percent);
        }
    }

    image = src;
}
