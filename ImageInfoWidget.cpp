#include "ImageInfoWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QString>
#include <algorithm> // для std::min/std::max

ImageInfoWidget::ImageInfoWidget(QWidget *parent)
    : QWidget(parent), m_label(new QLabel(this))
{
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_label);
    m_label->setText(QObject::tr("No image"));
}

void ImageInfoWidget::setImage(const QImage &image) {
    updateInfo(image);
}

void ImageInfoWidget::updateInfo(const QImage &image) {
    if (image.isNull()) {
        m_label->setText(QObject::tr("No image"));
        return;
    }

    int w = image.width();
    int h = image.height();
    QString fmt = image.format() == QImage::Format_ARGB32 ? "ARGB32" : QString::number(image.format());
    
    // преобразуем в ARGB32 для простоты работы
    QImage tmp = image.convertToFormat(QImage::Format_ARGB32);

    long long sumR = 0, sumG = 0, sumB = 0;
    int minR = 255, minG = 255, minB = 255;
    int maxR = 0, maxG = 0, maxB = 0;

    for (int y = 0; y < tmp.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(tmp.constScanLine(y));
        for (int x = 0; x < tmp.width(); ++x) {
            QRgb p = line[x];
            int r = qRed(p), g = qGreen(p), b = qBlue(p);
            sumR += r; sumG += g; sumB += b;
            minR = std::min(minR, r); minG = std::min(minG, g); minB = std::min(minB, b);
            maxR = std::max(maxR, r); maxG = std::max(maxG, g); maxB = std::max(maxB, b);
        }
    }

    long long pixels = static_cast<long long>(w) * static_cast<long long>(h);
    double avgR = double(sumR) / pixels;
    double avgG = double(sumG) / pixels;
    double avgB = double(sumB) / pixels;
    
    QString info = QString(QObject::tr(
        "Размер: %1 x %2\n"
        "Формат: %3\n"
        "Среднее R,G,B: %4, %5, %6\n"
        "Min R,G,B: %7, %8, %9\n"
        "Max R,G,B: %10, %11, %12"))
        .arg(w).arg(h)
        .arg(fmt)
        .arg(avgR, 0, 'f', 1).arg(avgG, 0, 'f', 1).arg(avgB, 0, 'f', 1)
        .arg(minR).arg(minG).arg(minB)
        .arg(maxR).arg(maxG).arg(maxB);
    m_label->setText(info);
}