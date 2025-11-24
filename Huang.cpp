// используется энтропия принадлежности пикселей к фон/объект
#include "Huang.h"
#include <vector>
#include <cmath>
#include <QtGlobal>
#include <QtConcurrent>
#include <atomic>

void Huang::binarize(QImage &img, std::function<void(int)> progressCallback) {
    if (img.isNull()) return;

    QImage gray = img;

    const int w = gray.width();
    const int h = gray.height();

    // строю гистрограмму 
    std::vector<int> hist(256, 0);
    for (int y = 0; y < h; ++y) {
        const uchar *line = gray.constScanLine(y);
        for (int x = 0; x < w; ++x) hist[line[x]]++;
        if (progressCallback) progressCallback(int((y * 20) / h));  // 0..20%
    }

    // поиск первого/последнего ненулевого бина
    int first_bin = 0;
    for (int ih = 0; ih < 256; ++ih) {
        if (hist[ih] != 0) { first_bin = ih; break; }
    }
    int last_bin = 255;
    for (int ih = 255; ih >= first_bin; --ih) {
        if (hist[ih] != 0) { last_bin = ih; break; }
    }

    // Avoid degenerate case
    if (last_bin <= first_bin) {
        // trivial threshold
        QImage dst(gray.size(), QImage::Format_ARGB32);
        for (int y = 0; y < h; ++y) {
            const uchar *line = gray.constScanLine(y);
            QRgb *dline = reinterpret_cast<QRgb*>(dst.scanLine(y));
            for (int x = 0; x < w; ++x) {
                int v = line[x];
                dline[x] = (v >= first_bin) ? qRgba(255,255,255,255) : qRgba(0,0,0,255);
            }
            if (progressCallback) progressCallback(50 + int((y * 50) / h));
        }
        if (progressCallback) progressCallback(100);
        img = dst;
        return;
    }

    // term нормализует расстояние между текущим значением пикселя и средним класса
    double term = 1.0 / double(last_bin - first_bin);

    // средняя яркость всех пикселей от first_bin до i (фон)
    std::vector<double> mu0(256, 0.0);
    {
        double sum_pix = 0.0;
        double num_pix = 0.0;
        for (int ih = first_bin; ih < 256; ++ih) {
            sum_pix += double(ih) * hist[ih];
            num_pix += hist[ih];
            // num_pix не ноль ih начинается с first_bin
            mu0[ih] = sum_pix / num_pix;
        }
    }

    // mu1[i] — средняя яркость всех пикселей от last_bin до i+1 (объект)
    std::vector<double> mu1(256, 0.0);
    {
        double sum_pix = 0.0;
        double num_pix = 0.0;
        for (int ih = last_bin; ih > 0; --ih) {
            sum_pix += double(ih) * hist[ih];
            num_pix += hist[ih];
            mu1[ih - 1] = sum_pix / num_pix;
        }
    }

    /*  пиксели ближе к среднему класса будут иметь примерно 1 -> маленькая энтропия
        пиксели дальше от среднего класса будут иметь примерно 0.5 -> большая энтропия
        ищу порог, дающий минимальную общую энтропию принадлежности                     */
    double min_ent = std::numeric_limits<double>::infinity();
    int threshold = 0;

    for (int it = 0; it < 256; ++it) {
        double ent = 0.0;
        for (int ih = 0; ih <= it; ++ih) {
            // membership function - по алгосу Хуанга
            double mu_x = 1.0 / (1.0 + term * std::abs(double(ih) - mu0[it]));  // степень принадлежности к классу
            if (!(mu_x < 1e-06 || mu_x > 0.999999)) {
                ent += double(hist[ih]) * ( -mu_x * std::log(mu_x) - (1.0 - mu_x) * std::log(1.0 - mu_x) );
            }
        }
        // объектный класс
        for (int ih = it + 1; ih < 256; ++ih) {
            double mu_x = 1.0 / (1.0 + term * std::abs(double(ih) - mu1[it]));
            if (!(mu_x < 1e-06 || mu_x > 0.999999)) {
                ent += double(hist[ih]) * ( -mu_x * std::log(mu_x) - (1.0 - mu_x) * std::log(1.0 - mu_x) );
            }
        }

        if (ent < min_ent) {
            min_ent = ent;
            threshold = it;
        }

        if (progressCallback && (it % 8 == 0)) {
            progressCallback(20 + int((it * 30) / 256));
        }
    }

    // применяю порог в параллельном режиме
    QImage dst(gray.size(), QImage::Format_ARGB32);
    std::vector<int> rows(h);
    for (int i = 0; i < h; ++i) rows[i] = i;
    std::atomic<int> counter(0);

    QtConcurrent::blockingMap(rows.begin(), rows.end(), [&](int y){
        const uchar *line = gray.constScanLine(y);
        QRgb *dline = reinterpret_cast<QRgb*>(dst.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int v = line[x];
            dline[x] = (v >= threshold) ? qRgba(255,255,255,255) : qRgba(0,0,0,255);
        }
        if (progressCallback) {
            int cnt = ++counter;
            int perc = 50 + int((cnt * 50) / h);
            if (perc > 100) perc = 100;
            progressCallback(perc);
        }
    });

    if (progressCallback) progressCallback(100);
    img = dst;
}
