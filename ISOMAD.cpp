#include "ISOMAD.h"
#include <vector>
#include <algorithm>
#include <QtGlobal>
#include <QtConcurrent>
#include <atomic>
#include <cmath>

// вычисление медианы
static int getMedian(std::vector<uchar> &buf) {
    size_t n = buf.size();
    if (n == 0) return 0;
    size_t mid = n/2;
    std::nth_element(buf.begin(), buf.begin() + mid, buf.end());
    int med = buf[mid];
    if (n % 2 == 0) {
        // tсли количество элементов чётное, берём среднее двух центральных (среднее медианы и максимума нижней половины)
        auto it = std::max_element(buf.begin(), buf.begin() + mid);
        return (med + *it) / 2;
    }
    return med;
}

void ISOMAD::binarize(QImage &img, int win, std::function<void(int)> progressCallback) {
    if (img.isNull()) return;
    if (win % 2 == 0) win++;    // размер окна win должен быть нечётным и ≥3, чтобы центр окна был точно в пикселе
    if (win < 3) win = 3;

    QImage gray = img;
    int w = gray.width();
    int h = gray.height();
    QImage dst(gray.size(), QImage::Format_ARGB32);

    int half = win / 2; // half — половина окна (для координат вокруг центра)

    std::vector<int> rows(h);
    for (int i = 0; i < h; ++i) rows[i] = i;
    std::atomic<int> counter(0);

    QtConcurrent::blockingMap(rows.begin(), rows.end(), [&](int y){
        std::vector<uchar> buf;
        buf.reserve(win * win);
        for (int x = 0; x < w; ++x) {

            buf.clear();
            int x0 = std::max(0, x - half);
            int x1 = std::min(w - 1, x + half);
            int y0 = std::max(0, y - half);
            int y1 = std::min(h - 1, y + half);
            
            for (int yy = y0; yy <= y1; ++yy) {
                const uchar *line = gray.constScanLine(yy);
                for (int xx = x0; xx <= x1; ++xx) buf.push_back(line[xx]);
            }

            int med = getMedian(buf); // медиана яркости окна
            for (auto &v : buf) v = uchar(std::abs(int(v) - med));
            int mad = getMedian(buf);   // медиана абсолютного отклонения от медианы

            // порог
            double thresh = med + 0.5 * mad;
            int v = gray.constScanLine(y)[x];
            reinterpret_cast<QRgb*>(dst.scanLine(y))[x] = (v >= thresh) ? qRgba(255,255,255,255) : qRgba(0,0,0,255);
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
