#include "Otsu.h"
#include "GrayscaleBT601.h"
#include <vector>
#include <numeric>
#include <QtGlobal>
#include <QtConcurrent>
#include <atomic>

void Otsu::binarize(QImage &img, std::function<void(int)> progressCallback) {
    if (img.isNull()) return;

    QImage gray = img;

    int w = gray.width();
    int h = gray.height();

    // строю гистограмму, в итоге получаю распределение яркости по всем пикселям
    std::vector<int> hist(256, 0);
    for (int y = 0; y < h; ++y) {
        const uchar *line = gray.constScanLine(y);
        for (int x = 0; x < w; ++x) hist[line[x]]++;
        if (progressCallback) progressCallback(int((y * 40) / h)); // 0..40%
    }

    int total = w * h;
    if (total == 0) {
        if (progressCallback) progressCallback(100);
        return;
    }

    std::vector<double> prob(256);
    for (int i = 0; i < 256; ++i) prob[i] = double(hist[i]) / total;

    /*  prob[i] — вероятность встретить пиксель с яркостью i.
        omega[t] — накопленная вероятность для класса пикселей [0, t].
        mu[t] — накопленное среднее яркости для класса [0, t].
        mu_t — средняя яркость всего изображения                       */
    std::vector<double> omega(256, 0.0);
    std::vector<double> mu(256, 0.0);    
    omega[0] = prob[0];
    mu[0] = 0.0;
    for (int i = 1; i < 256; ++i) {
        omega[i] = omega[i-1] + prob[i];
        mu[i] = mu[i-1] + i * prob[i];
    }
    double mu_t = mu[255];

    double maxSigma = -1.0;
    int threshold = 0;
    for (int t = 0; t < 256; ++t) {
        if (omega[t] <= 0.0 || omega[t] >= 1.0) continue;
        double mu0 = mu[t] / omega[t];                  // среднее 1-го класса (фон)
        double mu1 = (mu_t - mu[t]) / (1.0 - omega[t]); // среднее 2-го класса (объект)
        double sigmaBetween = omega[t] * (1.0 - omega[t]) * (mu0 - mu1) * (mu0 - mu1);
        if (sigmaBetween > maxSigma) {
            maxSigma = sigmaBetween;
            threshold = t;
        }
        if (progressCallback && (t % 5 == 0)) {
            progressCallback(40 + int((t * 40) / 256));
        }
    }

    // для параллельной обработки строк
    QImage dst(gray.size(), QImage::Format_ARGB32);
    std::vector<int> rows(h);
    for (int i = 0; i < h; ++i) rows[i] = i;
    std::atomic<int> counter(0);

    // применяю порог в параллелильном режиме
    QtConcurrent::blockingMap(rows.begin(), rows.end(), [&](int y){
        const uchar *line = gray.constScanLine(y);
        QRgb *dline = reinterpret_cast<QRgb*>(dst.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int v = line[x];
            dline[x] = (v >= threshold) ? qRgba(255,255,255,255) : qRgba(0,0,0,255);
        }
        if (progressCallback) {
            int cnt = ++counter;
            int perc = 80 + int((cnt * 20) / h);
            if (perc > 100) perc = 100;
            progressCallback(perc);
        }
    });

    if (progressCallback) progressCallback(100);
    img = dst;
}
