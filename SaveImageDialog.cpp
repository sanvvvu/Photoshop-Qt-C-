#include "SaveImageDialog.h"
#include "HelpDialog.h"
#include "Compression.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <tiffio.h>
#include <lzma.h>
#include <QDebug>

SaveImageDialog::SaveImageDialog(const QImage &img, QWidget *parent)
    : QDialog(parent), m_image(img)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Сохранить изображение"));
    setModal(true);
    resize(560, 160);

    auto mainL = new QVBoxLayout(this);

    // формат
    auto fmtL = new QHBoxLayout;
    fmtL->addWidget(new QLabel(tr("Формат:")));
    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem("PNG");
    m_formatCombo->addItem("TIFF");
    fmtL->addStretch(1);
    fmtL->addWidget(m_formatCombo);
    mainL->addLayout(fmtL);

    // файл
    auto fileL = new QHBoxLayout;
    fileL->addWidget(new QLabel(tr("Файл:")));
    m_fileEdit = new QLineEdit(this);
    m_browseBtn = new QPushButton(tr("Обзор..."), this);
    m_helpBtn = new QPushButton(tr("Справка"), this);
    fileL->addWidget(m_fileEdit, 1);
    fileL->addWidget(m_browseBtn);
    fileL->addWidget(m_helpBtn);
    mainL->addLayout(fileL);

    // сжатие
    auto compL = new QHBoxLayout;
    compL->addWidget(new QLabel(tr("Сжатие (для TIFF):")));
    m_compressionCombo = new QComboBox(this);
    compL->addWidget(m_compressionCombo);

    m_levelLabel = new QLabel(tr("Параметр:"), this);
    m_levelSpin = new QSpinBox(this);
    m_levelSpin->setRange(0, 100);
    m_levelSpin->setValue(5);
    compL->addWidget(m_levelLabel);
    compL->addWidget(m_levelSpin);
    compL->addStretch(1);
    mainL->addLayout(compL);

    // кнопки
    auto btnL = new QHBoxLayout;
    m_saveBtn = new QPushButton(tr("Сохранить"), this);
    m_cancelBtn = new QPushButton(tr("Отмена"), this);
    btnL->addStretch(1);
    btnL->addWidget(m_saveBtn);
    btnL->addWidget(m_cancelBtn);
    mainL->addLayout(btnL);

    // события
    connect(m_formatCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &SaveImageDialog::onFormatChanged);
    connect(m_compressionCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &SaveImageDialog::onCompressionChanged);
    connect(m_browseBtn, &QPushButton::clicked, this, &SaveImageDialog::onChooseFile);
    connect(m_saveBtn, &QPushButton::clicked, this, &SaveImageDialog::onSave);
    connect(m_cancelBtn, &QPushButton::clicked, this, &SaveImageDialog::reject);
    connect(m_helpBtn, &QPushButton::clicked, this, &SaveImageDialog::onHelpRequested);

    // дефолтное имя
    QString defDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (defDir.isEmpty()) defDir = QDir::homePath();
    QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString defName = defDir + QDir::separator() + QString("result_%1.png").arg(ts);
    m_fileEdit->setText(defName);
    m_filename = defName;

    populateCompressionOptions();
    updateUIState();
    onCompressionChanged(m_compressionCombo->currentIndex());
}

void SaveImageDialog::populateCompressionOptions() {
    m_compressionCombo->clear();
    bool isEffectivelyMono = imageIsEffectivelyMonochrome(m_image);
    bool isGray8 = imageIsGrayscale(m_image);

    m_compressionCombo->addItem(tr("Без сжатия"));
    m_compressionCombo->addItem(tr("Deflate"));
    m_compressionCombo->addItem(tr("LZMA"));
    m_compressionCombo->addItem(tr("PackBits"));
    m_compressionCombo->addItem(tr("LZW"));
    if (isEffectivelyMono || isGray8) {
        m_compressionCombo->addItem(tr("CCITT Group 3"));
        m_compressionCombo->addItem(tr("CCITT Group 4"));
    }
    m_compressionCombo->addItem(tr("JPEG"));
}

void SaveImageDialog::onFormatChanged(int idx) {
    Q_UNUSED(idx);
    QString cur = m_fileEdit->text();
    if (!cur.isEmpty()) {
        QFileInfo fi(cur);
        QString base = fi.path() + QDir::separator() + fi.completeBaseName();
        QString wantedExt = (m_formatCombo->currentText() == "PNG") ? ".png" : ".tif";
        QString newName = base + wantedExt;
        m_fileEdit->setText(newName);
        m_filename = newName;
    }
    populateCompressionOptions();
    updateUIState();
}

void SaveImageDialog::onCompressionChanged(int idx) {
    Q_UNUSED(idx);
    QString comp = m_compressionCombo->currentText();

    if (comp == tr("Deflate") || comp == tr("LZMA")) {
        m_levelSpin->setRange(1, 9);
        m_levelSpin->setValue(5);
        m_levelSpin->setEnabled(true);
        m_levelLabel->setEnabled(true);
    } else if (comp == tr("JPEG")) {
        m_levelSpin->setRange(0, 100);
        m_levelSpin->setValue(75);
        m_levelSpin->setEnabled(true);
        m_levelLabel->setEnabled(true);
    } else {
        m_levelSpin->setEnabled(false);
        m_levelLabel->setEnabled(false);
    }
}

void SaveImageDialog::onChooseFile() {
    QString filter = (m_formatCombo->currentText() == "PNG") ? tr("PNG (*.png)") : tr("TIFF (*.tif *.tiff)");
    QString fn = QFileDialog::getSaveFileName(this, tr("Сохранить изображение"), m_fileEdit->text(), filter);
    if (!fn.isEmpty()) {
        m_fileEdit->setText(fn);
        m_filename = fn;
    }
}

void SaveImageDialog::onHelpRequested() {
    HelpDialog dlg(this);
    dlg.exec();
}

void SaveImageDialog::onSave() {
    QString fn = m_fileEdit->text().trimmed();
    if (fn.isEmpty()) {
        QString defDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        if (defDir.isEmpty()) defDir = QDir::homePath();
        QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        fn = defDir + QDir::separator() + QString("result_%1.png").arg(ts);
    }

    if (m_formatCombo->currentText() == "PNG") {
        if (!fn.endsWith(".png", Qt::CaseInsensitive)) fn += ".png";
        if (!m_image.save(fn, "PNG"))
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось сохранить PNG"));
        else { m_filename = fn; accept(); }
        return;
    }

    if (!fn.endsWith(".tif", Qt::CaseInsensitive) && !fn.endsWith(".tiff", Qt::CaseInsensitive))
        fn += ".tif";
    m_filename = fn;

    if (!writeTIFFWithCompression(m_filename))
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось сохранить TIFF"));
    else
        accept();
}

void SaveImageDialog::updateUIState() {
    bool isTiff = (m_formatCombo->currentText() == "TIFF");
    m_compressionCombo->setEnabled(isTiff);
    m_levelSpin->setEnabled(isTiff && m_levelLabel->isEnabled());
    m_levelLabel->setEnabled(isTiff && m_levelLabel->isEnabled());
    m_browseBtn->setEnabled(true);
    m_fileEdit->setEnabled(true);
    m_helpBtn->setEnabled(true);
}

bool SaveImageDialog::writeTIFFWithCompression(const QString &filename) {
    QImage img = m_image;
    uint32_t width = img.width();
    uint32_t height = img.height();

    QString comp = m_compressionCombo->currentText();
    bool useCCITT = (comp == tr("CCITT Group 3") || comp == tr("CCITT Group 4"));

    if (useCCITT) {
        // конвертируем в монохромное 1-bit для CCITT
        img = img.convertToFormat(QImage::Format_Mono);
    } else if (img.format() != QImage::Format_RGB888 && img.format() != QImage::Format_Grayscale8) {
        img = img.convertToFormat(QImage::Format_RGB888);
    }

    bool isGray = (img.format() == QImage::Format_Grayscale8 || useCCITT);

    TIFF *tif = TIFFOpen(filename.toLocal8Bit().constData(), "w");
    if (!tif) return false;

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);

    if (useCCITT) {
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 1);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, (comp == tr("CCITT Group 3")) ? COMPRESSION_CCITTFAX3 : COMPRESSION_CCITTFAX4);
        TIFFSetField(tif, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
    } else {
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, isGray ? 1 : 3);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, isGray ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB);

        if (comp == tr("Без сжатия")) TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        else if (comp == tr("Deflate")) TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
        else if (comp == tr("PackBits")) TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);
        else if (comp == tr("LZW")) { TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW); TIFFSetField(tif, TIFFTAG_PREDICTOR, 2); }
        else if (comp == tr("JPEG")) { TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_JPEG); TIFFSetField(tif, TIFFTAG_JPEGQUALITY, m_levelSpin->value()); }
    }

    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, 0));

    for (uint32_t y = 0; y < height; ++y) {
        const uchar *line = img.constScanLine(y);
        if (TIFFWriteScanline(tif, (tdata_t)line, y, 0) < 0) { TIFFClose(tif); return false; }
    }

    TIFFClose(tif);
    return true;
}
bool SaveImageDialog::saveLZMA(const QString &filename) {
    // Конвертируем изображение в RGB888
    QImage img = m_image;
    if (img.format() != QImage::Format_RGB888)
        img = img.convertToFormat(QImage::Format_RGB888);

    std::vector<uint8_t> raw(img.width() * img.height() * 3);
    for (int y = 0; y < img.height(); ++y) {
        const uchar *line = img.constScanLine(y);
        memcpy(raw.data() + y * img.width() * 3, line, img.width() * 3);
    }

    std::vector<uint8_t> out;
    int level = m_levelSpin->value();
    if (level < 1) level = 5;
    if (level > 9) level = 9;

    lzma_ret r = lzma_easy_buffer_encode(level, LZMA_CHECK_CRC64, nullptr, raw.data(), raw.size(), nullptr, nullptr, 0);
    // выделяем буфер
    size_t outMax = raw.size() + raw.size() / 3 + 128;
    out.resize(outMax);
    size_t destLen = outMax;
    r = lzma_easy_buffer_encode(level, LZMA_CHECK_CRC64, nullptr, raw.data(), raw.size(), out.data(), &destLen, out.size());
    if (r != LZMA_OK) { qWarning() << "lzma_easy_buffer_encode failed" << int(r); return false; }
    out.resize(destLen);

    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(reinterpret_cast<const char*>(out.data()), out.size());
    f.close();
    return true;
}

bool SaveImageDialog::imageIsEffectivelyMonochrome(const QImage &img) const {
    if (img.isNull()) return false;
    if (img.format() == QImage::Format_Mono || img.format() == QImage::Format_MonoLSB) return true;

    int w = img.width(), h = img.height();
    const int MAX_SAMPLES = 200000;
    int total = w * h;
    int step = (total > MAX_SAMPLES) ? (total / MAX_SAMPLES) + 1 : 1;
    bool seenBlack = false, seenWhite = false, seenOther = false;

    QImage tmp = img.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < h && !seenOther; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(tmp.constScanLine(y));
        for (int x = 0; x < w; x += step) {
            QRgb p = line[x];
            int r = qRed(p), g = qGreen(p), b = qBlue(p);
            if (r == 0 && g == 0 && b == 0) seenBlack = true;
            else if (r == 255 && g == 255 && b == 255) seenWhite = true;
            else { seenOther = true; break; }
        }
    }
    return (!seenOther) && (seenBlack || seenWhite);
}

bool SaveImageDialog::imageIsGrayscale(const QImage &img) const {
    if (img.isNull()) return false;
    if (img.format() == QImage::Format_Grayscale8) return true;

    int w = img.width(), h = img.height();
    const int MAX_SAMPLES = 200000;
    int total = w * h;
    int step = (total > MAX_SAMPLES) ? (total / MAX_SAMPLES) + 1 : 1;
    QImage tmp = img.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < h; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(tmp.constScanLine(y));
        for (int x = 0; x < w; x += step) {
            QRgb p = line[x];
            int r = qRed(p), g = qGreen(p), b = qBlue(p);
            if (!(r == g && g == b)) return false;
        }
    }
    return true;
}

bool saveImageWithDialog(const QImage &image, QWidget *parent) {
    SaveImageDialog dlg(image, parent);
    return dlg.exec() == QDialog::Accepted;
}
