#include "Niblack.h"
#include <vector>
#include <cmath>
#include <QtGlobal>
#include <QtConcurrent>
#include <atomic>

// быстрый интегральный метод для среднего и дисперсии
static void integralImages(const QImage &gray, std::vector<double> &sum, std::vector<double> &sumSq) {
    int w = gray.width();
    int h = gray.height();
    sum.assign((w+1)*(h+1), 0.0);
    sumSq.assign((w+1)*(h+1), 0.0);

    for (int y = 1; y <= h; ++y) {
        const uchar *line = gray.constScanLine(y-1);
        double rowSum = 0.0, rowSumSq = 0.0;

        for (int x = 1; x <= w; ++x) {
            double v = line[x-1];
            rowSum += v;
            rowSumSq += v * v;
            int idx = y*(w+1) + x;
            sum[idx] = sum[idx - (w+1)] + rowSum;
            sumSq[idx] = sumSq[idx - (w+1)] + rowSumSq;
        }
    }
}

void Niblack::binarize(QImage &img, int win, double k, std::function<void(int)> progressCallback) {
    if (img.isNull()) return;
    if (win % 2 == 0) win++;

    QImage gray = img;
    int w = gray.width();
    int h = gray.height();

    std::vector<double> sum, sumSq;
    integralImages(gray, sum, sumSq);

    QImage dst(gray.size(), QImage::Format_ARGB32);
    int half = win / 2;

    std::vector<int> rows(h);
    for (int i = 0; i < h; ++i) rows[i] = i;
    std::atomic<int> counter(0);

    QtConcurrent::blockingMap(rows.begin(), rows.end(), [&](int y){
        QRgb *dline = reinterpret_cast<QRgb*>(dst.scanLine(y));

        for (int x = 0; x < w; ++x) {
            int x0 = std::max(0, x - half);
            int x1 = std::min(w - 1, x + half);
            int y0 = std::max(0, y - half);
            int y1 = std::min(h - 1, y + half);
            int A = y0 * (w+1) + x0;
            int B = y0 * (w+1) + (x1+1);
            int C = (y1+1) * (w+1) + x0;
            int D = (y1+1) * (w+1) + (x1+1);
            double area = double((y1 - y0 + 1) * (x1 - x0 + 1));
            double s = sum[D] - sum[B] - sum[C] + sum[A];
            double s2 = sumSq[D] - sumSq[B] - sumSq[C] + sumSq[A];
            double mean = s / area;
            double var = (s2 / area) - (mean * mean);
            if (var < 0.0 && var > -1e-12) var = 0.0;
            double stddev = var > 0 ? std::sqrt(var) : 0.0;
            int v = gray.constScanLine(y)[x];
            double thresh = mean + k * stddev;
            dline[x] = (v >= thresh) ? qRgba(255,255,255,255) : qRgba(0,0,0,255);
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
