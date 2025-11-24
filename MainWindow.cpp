#include "MainWindow.h"
#include "ImageInfoWidget.h"
#include "filter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QCheckBox>
#include <QFileDialog>
#include <QPixmap>
#include <QStatusBar>
#include <QProgressBar>
#include <QFont>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QMessageBox>

#include "GrayscaleBT601.h"
#include "GrayscaleBT709.h"
#include "Otsu.h"
#include "Huang.h"
#include "Niblack.h"
#include "ISOMAD.h"

// пресеты
static double box3x3[] = {
    1/9.0, 1/9.0, 1/9.0,
    1/9.0, 1/9.0, 1/9.0,
    1/9.0, 1/9.0, 1/9.0
};

static double sharpen3x3[] = {
     0, -1,  0,
    -1,  5, -1,
     0, -1,  0
};

static QVector<double> getPreset(const QString &name, int &outK) {
    if (name.contains(QObject::tr("Блюр"))) {
        outK = 3;
        return QVector<double>(box3x3, box3x3 + 9);
    } else if (name.contains(QObject::tr("Резкость"))) {
        outK = 3;
        return QVector<double>(sharpen3x3, sharpen3x3 + 9);
    } else {
        outK = 3;
        return QVector<double>();
    }
}

// мэйн виндоу
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    connectSignals();
}

void MainWindow::setupUi() {
    setWindowTitle(tr("Пример фильтрации изображения"));
    setMinimumSize(900, 700);

    // стиль
    QString style = R"(
        QWidget { background: qlineargradient(x1:0 y1:0, x2:0 y2:1, stop:0 #fff8f8, stop:1 #ffffff); }
        QGroupBox { border: 1px solid #e0d6d6; border-radius: 8px; margin-top: 6px; padding: 10px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
        QPushButton {
            border: 1px solid #b33;
            border-radius: 10px;
            padding: 6px 10px;
            font-weight: 600;
            background: qlineargradient(x1:0 y1:0, x2:0 y2:1, stop:0 #fff, stop:1 #fbecec);
        }
        QPushButton:hover { background: #ffdede; }
        QComboBox, QSpinBox, QDoubleSpinBox, QTableWidget { background: #ffffff; }
    )";
    setStyleSheet(style);

    m_central = new QWidget(this);
    setCentralWidget(m_central);

    auto mainLayout = new QHBoxLayout(m_central);

    // слева изображения
    auto imagesBox = new QGroupBox(tr("Изображения"), this);
    auto imagesLayout = new QVBoxLayout(imagesBox);
    m_origLabel = new QLabel();
    m_procLabel = new QLabel();
    m_origLabel->setAlignment(Qt::AlignCenter);
    m_procLabel->setAlignment(Qt::AlignCenter);

    QScrollArea *origScroll = new QScrollArea();
    origScroll->setWidgetResizable(true);
    QWidget *origHolder = new QWidget();
    QVBoxLayout *ohL = new QVBoxLayout(origHolder);
    ohL->addWidget(m_origLabel);
    origScroll->setWidget(origHolder);

    QScrollArea *procScroll = new QScrollArea();
    procScroll->setWidgetResizable(true);
    QWidget *procHolder = new QWidget();
    QVBoxLayout *phL = new QVBoxLayout(procHolder);
    phL->addWidget(m_procLabel);
    procScroll->setWidget(procHolder);

    imagesLayout->addWidget(new QLabel(tr("Исходное изображение:")));
    imagesLayout->addWidget(origScroll, 1);
    imagesLayout->addWidget(new QLabel(tr("Обработанное изображение:")));
    imagesLayout->addWidget(procScroll, 1);

    mainLayout->addWidget(imagesBox, 3);

    // справа контролы
    auto controlsBox = new QGroupBox(tr("Управление"), this);
    auto rightBox = new QVBoxLayout(controlsBox);

    m_loadBtn = new QPushButton(tr("Загрузить изображение..."));
    m_applyBtn = new QPushButton(tr("Применить фильтр"));
    m_saveBtn = new QPushButton(tr("Сохранить результат..."));
    m_resetBtn = new QPushButton(tr("Сброс"));

    m_filterCombo = new QComboBox();
    m_filterCombo->addItem(tr("Нет"));
    m_filterCombo->addItem(tr("Блюр"));
    m_filterCombo->addItem(tr("Резкость"));
    m_filterCombo->addItem(tr("Контраст"));
    m_filterCombo->addItem(tr("Пользовательский"));
    m_filterCombo->addItem(tr("Полутоновый BT.601-7"));
    m_filterCombo->addItem(tr("Полутоновый BT.709-6"));
    m_filterCombo->addItem(tr("Бинаризация Оцу"));
    m_filterCombo->addItem(tr("Бинаризация Хуанга"));
    m_filterCombo->addItem(tr("Бинаризация Ниблака"));
    m_filterCombo->addItem(tr("Бинаризация ИСОМАД"));

    // контролы ядра
    QLabel *kSizeLabel = new QLabel(tr("Размер ядра (нечётное):"));
    m_kSizeSpin = new QSpinBox();
    m_kSizeSpin->setRange(1, 11);
    m_kSizeSpin->setSingleStep(2);
    m_kSizeSpin->setValue(3);

    m_kernelTable = new QTableWidget(3, 3);
    m_kernelTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_kernelTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    m_normalizeBox = new QCheckBox(tr("Нормировать ядро (сумма = 1)"));
    m_normalizeBox->setChecked(true);

    // контраст
    QLabel *contrastLabel = new QLabel(tr("Коэффициент контраста:"));
    m_contrastSpin = new QDoubleSpinBox();
    m_contrastSpin->setRange(0.1, 5.0);
    m_contrastSpin->setSingleStep(0.1);
    m_contrastSpin->setValue(1.2);

    // локальное окно для Niblack/ISOMAD (нечетное надо)
    QLabel *localWindowLabel = new QLabel(tr("Окно (для Niblack/ISOMAD, нечётное):"));
    m_localWindowSpin = new QSpinBox();
    m_localWindowSpin->setRange(3, 101);
    m_localWindowSpin->setSingleStep(2);
    m_localWindowSpin->setValue(15);

    QLabel *niblackKLabel = new QLabel(tr("k (Niblack):"));
    m_niblackKSpin = new QDoubleSpinBox();
    m_niblackKSpin->setRange(-5.0, 5.0);
    m_niblackKSpin->setSingleStep(0.1);
    m_niblackKSpin->setValue(-0.2);

    m_infoWidget = new ImageInfoWidget();

    rightBox->addWidget(m_loadBtn);
    rightBox->addSpacing(6);
    rightBox->addWidget(new QLabel(tr("Выберите фильтр:")));
    rightBox->addWidget(m_filterCombo);
    rightBox->addSpacing(6);
    rightBox->addWidget(kSizeLabel);
    rightBox->addWidget(m_kSizeSpin);
    rightBox->addWidget(m_kernelTable, 1);
    rightBox->addWidget(m_normalizeBox);
    rightBox->addWidget(contrastLabel);
    rightBox->addWidget(m_contrastSpin);

    contrastLabel->setVisible(false);
    m_contrastSpin->setVisible(false);

    rightBox->addWidget(localWindowLabel);
    rightBox->addWidget(m_localWindowSpin);
    rightBox->addWidget(niblackKLabel);
    rightBox->addWidget(m_niblackKSpin);

    localWindowLabel->setVisible(false);
    m_localWindowSpin->setVisible(false);
    niblackKLabel->setVisible(false);
    m_niblackKSpin->setVisible(false);

    rightBox->addWidget(m_applyBtn);
    rightBox->addWidget(m_saveBtn);
    rightBox->addWidget(m_resetBtn);
    rightBox->addStretch();
    rightBox->addWidget(m_infoWidget);

    mainLayout->addWidget(controlsBox, 1);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    statusBar()->addPermanentWidget(m_progressBar, 1);

    fillKernelFromVector(QVector<double>(box3x3, box3x3 + 9), 3);

    m_filterCombo->setEnabled(false);
    m_applyBtn->setEnabled(false);
    m_saveBtn->setEnabled(false);
    m_resetBtn->setEnabled(false);
    m_kSizeSpin->setEnabled(false);
    m_kernelTable->setEnabled(false);
}

void MainWindow::connectSignals() {
    connect(m_loadBtn, &QPushButton::clicked, this, &MainWindow::onLoadClicked);
    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::onApplyClicked);
    connect(m_saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveClicked);
    connect(m_resetBtn, &QPushButton::clicked, this, &MainWindow::onResetClicked);
    connect(m_filterCombo, &QComboBox::currentTextChanged, this, &MainWindow::onFilterChanged);
    connect(m_kSizeSpin, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::onKernelSizeChanged);
}

void MainWindow::setKernelTableSize(int k) {
    m_kernelTable->setRowCount(k);
    m_kernelTable->setColumnCount(k);
    for (int r = 0; r < k; ++r) {
        for (int c = 0; c < k; ++c) {
            if (!m_kernelTable->item(r,c)) {
                m_kernelTable->setItem(r, c, new QTableWidgetItem(QString::number(0.0)));
            }
        }
    }
}

void MainWindow::fillKernelFromVector(const QVector<double> &vec, int k) {
    setKernelTableSize(k);
    if (vec.size() == k * k) {
        for (int r = 0; r < k; ++r)
            for (int c = 0; c < k; ++c)
                m_kernelTable->item(r,c)->setText(QString::number(vec[r * k + c]));
    } else {
        for (int r = 0; r < k; ++r)
            for (int c = 0; c < k; ++c)
                m_kernelTable->item(r,c)->setText((r==k/2 && c==k/2) ? "1" : "0");
    }
}

QVector<double> MainWindow::readKernelFromTable() const {
    int k = m_kernelTable->rowCount();
    QVector<double> vec(k*k);
    for (int r = 0; r < k; ++r)
        for (int c = 0; c < k; ++c)
            vec[r*k + c] = m_kernelTable->item(r,c)->text().toDouble();
    return vec;
}

void MainWindow::onLoadClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Открыть изображение"), QString(),
                                                    tr("Images (*.png *.jpg *.bmp)"));
    if (!fileName.isEmpty()) {
        if (!m_originalImage.load(fileName)) {
            QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось загрузить изображение"));
            return;
        }
        m_currentImage = m_originalImage;
        m_origLabel->setPixmap(QPixmap::fromImage(m_originalImage).scaled(600, 400, Qt::KeepAspectRatio));
        m_procLabel->setPixmap(QPixmap::fromImage(m_currentImage).scaled(600, 400, Qt::KeepAspectRatio));
        m_infoWidget->setImage(m_currentImage);

        // включаем контролы
        m_filterCombo->setEnabled(true);
        m_applyBtn->setEnabled(true);
        m_saveBtn->setEnabled(true);
        m_resetBtn->setEnabled(true);
        m_kSizeSpin->setEnabled(true);
        m_kernelTable->setEnabled(true);
    }
}

void MainWindow::onApply() { 
    // пустая реализация нужна для moc/линкера (ругался на отсутствие реализации слота :( )
}

void MainWindow::onApplyClicked() {
    if (m_currentImage.isNull()) return;
    QString sel = m_filterCombo->currentText();

    // progress callback — будет вызываться из рабочего потока
    std::function<void(int)> progressCb = [this](int p){
        QMetaObject::invokeMethod(this, "setProgress", Qt::QueuedConnection, Q_ARG(int, p));
    };

    if (sel.contains(tr("Контраст"))) {
        double factor = m_contrastSpin->value();
        adjustContrast(m_currentImage, factor);
        m_procLabel->setPixmap(QPixmap::fromImage(m_currentImage).scaled(600, 400, Qt::KeepAspectRatio));
        m_infoWidget->setImage(m_currentImage);
        m_progressBar->setValue(100);
        return;
    } else if (sel.contains(tr("Нет"))) {
        return;
    } else if (sel.contains(tr("Блюр")) || sel.contains(tr("Резкость")) || sel.contains(tr("Пользовательский"))) {
        int k = m_kernelTable->rowCount();
        QVector<double> vec = readKernelFromTable();

        // нормализация
        if (m_normalizeBox->isChecked()) {
            double s = 0.0;
            for (double v : vec) s += v;
            if (std::abs(s) > 1e-12) {
                for (int i = 0; i < vec.size(); ++i) vec[i] /= s;
            } else {
                double sabs = 0.0;
                for (double v : vec) sabs += std::abs(v);
                if (sabs > 1e-12) for (int i = 0; i < vec.size(); ++i) vec[i] /= sabs;
            }
            int kk = m_kernelTable->rowCount();
            if (kk * kk == vec.size()) {
                for (int r = 0; r < kk; ++r)
                    for (int c = 0; c < kk; ++c)
                        m_kernelTable->item(r, c)->setText(QString::number(vec[r * kk + c], 'f', 6));
            }
        }

        QImage img = m_currentImage; 
        QVector<double> kernelVec = vec;

        QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);
        QFuture<void> future = QtConcurrent::run([img, kernelVec, k, progressCb, this]() mutable {
            std::vector<double> tmp(kernelVec.constBegin(), kernelVec.constEnd());
            filter2D(img, tmp.data(), std::size_t(k), std::size_t(k));
            QMetaObject::invokeMethod(const_cast<MainWindow*>(this), [this, img]() mutable {
                m_currentImage = img;
                m_procLabel->setPixmap(QPixmap::fromImage(m_currentImage).scaled(600, 400, Qt::KeepAspectRatio));
                m_infoWidget->setImage(m_currentImage);
                m_progressBar->setValue(100);
            }, Qt::QueuedConnection);
        });
        watcher->setFuture(future);
        connect(watcher, &QFutureWatcher<void>::finished, watcher, &QObject::deleteLater);
        return;
    }

    // Новые функции для конвертации в полутоновый и бинаризации
    if (sel.contains(tr("Полутоновый BT.601-7"))) {
        QImage img = m_currentImage;
        QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);

        QFuture<void> future = QtConcurrent::run([img, progressCb, this]() mutable {
            GrayscaleBT601::convert(img, progressCb);
            QMetaObject::invokeMethod(this, [this, img]() mutable {
                m_currentImage = img;
                m_procLabel->setPixmap(QPixmap::fromImage(m_currentImage).scaled(600, 400, Qt::KeepAspectRatio));
                m_infoWidget->setImage(m_currentImage);
                m_progressBar->setValue(100);
            }, Qt::QueuedConnection);
        });

        watcher->setFuture(future);
        connect(watcher, &QFutureWatcher<void>::finished, watcher, &QObject::deleteLater);
        return;

    } else if (sel.contains(tr("Полутоновый BT.709-6"))) {
        QImage img = m_currentImage;
        QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);

        QFuture<void> future = QtConcurrent::run([img, progressCb, this]() mutable {
            GrayscaleBT709::convert(img, progressCb);
            QMetaObject::invokeMethod(this, [this, img]() mutable {
                m_currentImage = img;
                m_procLabel->setPixmap(QPixmap::fromImage(m_currentImage).scaled(600, 400, Qt::KeepAspectRatio));
                m_infoWidget->setImage(m_currentImage);
                m_progressBar->setValue(100);
            }, Qt::QueuedConnection);
        });

        watcher->setFuture(future);
        connect(watcher, &QFutureWatcher<void>::finished, watcher, &QObject::deleteLater);
        return;

    } else if (sel.contains(tr("Бинаризация Оцу"))) {
        QImage img = m_currentImage;
        QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);

        QFuture<void> future = QtConcurrent::run([img, progressCb, this]() mutable {
            GrayscaleBT601::convert(img, progressCb);
            Otsu::binarize(img, progressCb);
            QMetaObject::invokeMethod(this, [this, img]() mutable {
                m_currentImage = img;
                m_procLabel->setPixmap(QPixmap::fromImage(m_currentImage).scaled(600, 400, Qt::KeepAspectRatio));
                m_infoWidget->setImage(m_currentImage);
                m_progressBar->setValue(100);
            }, Qt::QueuedConnection);
        });

        watcher->setFuture(future);
        connect(watcher, &QFutureWatcher<void>::finished, watcher, &QObject::deleteLater);
        return;

    } else if (sel.contains(tr("Бинаризация Хуанга"))) {
        QImage img = m_currentImage;
        QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);

        QFuture<void> future = QtConcurrent::run([img, progressCb, this]() mutable {
            GrayscaleBT601::convert(img, progressCb);
            Huang::binarize(img, progressCb);
            QMetaObject::invokeMethod(this, [this, img]() mutable {
                m_currentImage = img;
                m_procLabel->setPixmap(QPixmap::fromImage(m_currentImage).scaled(600, 400, Qt::KeepAspectRatio));
                m_infoWidget->setImage(m_currentImage);
                m_progressBar->setValue(100);
            }, Qt::QueuedConnection);
        });

        watcher->setFuture(future);
        connect(watcher, &QFutureWatcher<void>::finished, watcher, &QObject::deleteLater);
        return;

    } else if (sel.contains(tr("Бинаризация Ниблака"))) {
        QImage img = m_currentImage;
        int win = m_localWindowSpin->value();
        double k = m_niblackKSpin->value();
        QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);

        QFuture<void> future = QtConcurrent::run([img, win, k, progressCb, this]() mutable {
            GrayscaleBT601::convert(img, progressCb);
            Niblack::binarize(img, win, k, progressCb);
            QMetaObject::invokeMethod(this, [this, img]() mutable {
                m_currentImage = img;
                m_procLabel->setPixmap(QPixmap::fromImage(m_currentImage).scaled(600, 400, Qt::KeepAspectRatio));
                m_infoWidget->setImage(m_currentImage);
                m_progressBar->setValue(100);
            }, Qt::QueuedConnection);
        });

        watcher->setFuture(future);
        connect(watcher, &QFutureWatcher<void>::finished, watcher, &QObject::deleteLater);
        return;
        
    } else if (sel.contains(tr("Бинаризация ИСОМАД"))) {
        QImage img = m_currentImage;
        int win = m_localWindowSpin->value();
        QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);

        QFuture<void> future = QtConcurrent::run([img, win, progressCb, this]() mutable {
            GrayscaleBT601::convert(img, progressCb);
            ISOMAD::binarize(img, win, progressCb);
            QMetaObject::invokeMethod(this, [this, img]() mutable {
                m_currentImage = img;
                m_procLabel->setPixmap(QPixmap::fromImage(m_currentImage).scaled(600, 400, Qt::KeepAspectRatio));
                m_infoWidget->setImage(m_currentImage);
                m_progressBar->setValue(100);
            }, Qt::QueuedConnection);
        });

        watcher->setFuture(future);
        connect(watcher, &QFutureWatcher<void>::finished, watcher, &QObject::deleteLater);
        return;

    } else {
        // неизвестный пункт — ничего не делаю
    }
}

void MainWindow::onSaveClicked() {
    if (m_currentImage.isNull()) return;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить изображение"), QString(),
                                                    tr("PNG Image (*.png)"));
    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".png", Qt::CaseInsensitive)) fileName += ".png";
        m_currentImage.save(fileName, "PNG");
    }
}

void MainWindow::onResetClicked() {
    if (m_originalImage.isNull()) return;

    m_currentImage = m_originalImage;
    m_procLabel->setPixmap(QPixmap::fromImage(m_currentImage).scaled(600, 400, Qt::KeepAspectRatio));
    m_infoWidget->setImage(m_currentImage);

    // добавляю обновление прогресса
    if (m_progressBar) {
        m_progressBar->setValue(0);
    }
    statusBar()->showMessage("Сброшено");
}

void MainWindow::onFilterChanged(const QString &text) {
    bool isContrast = text.contains(tr("Контраст"));
    bool isCustom = text.contains(tr("Пользовательский"));
    bool isKernelPreset = text.contains(tr("Блюр")) || text.contains(tr("Резкость")) || isCustom;

    // показывать или нет контролы ядра и контраста
    m_kSizeSpin->setVisible(isKernelPreset);
    m_kernelTable->setVisible(isKernelPreset);
    m_normalizeBox->setVisible(isKernelPreset);

    m_contrastSpin->setVisible(isContrast);

    // локальная бинаризация (Ниблака, ИИСОМАД)
    bool isLocal = text.contains(tr("Niblack")) || text.contains(tr("ISOMAD"));
    m_localWindowSpin->setVisible(isLocal);
    m_niblackKSpin->setVisible(text.contains(tr("Niblack")));

    if (!isCustom && isKernelPreset && !isContrast) {
        int presetK = 3;
        QVector<double> vec = getPreset(text, presetK);
        m_kSizeSpin->setValue(presetK);
        fillKernelFromVector(vec, presetK);
    } else if (isCustom) {
        // ничего не делаем
    }
}

void MainWindow::onKernelSizeChanged(int newSize) {
    if (newSize % 2 == 0) {
        newSize = newSize + 1;
        m_kSizeSpin->setValue(newSize);
        return;
    }
    int oldR = m_kernelTable->rowCount();
    int oldC = m_kernelTable->columnCount();
    QVector<double> oldVals;
    for (int r = 0; r < oldR; ++r)
        for (int c = 0; c < oldC; ++c)
            oldVals.append(m_kernelTable->item(r,c)->text().toDouble());

    QVector<double> newVals(newSize*newSize, 0.0);
    int oldCenterR = oldR/2, oldCenterC = oldC/2;
    int newCenterR = newSize/2, newCenterC = newSize/2;
    for (int r = 0; r < oldR; ++r)
        for (int c = 0; c < oldC; ++c) {
            int dr = r - oldCenterR;
            int dc = c - oldCenterC;
            int nr = newCenterR + dr;
            int nc = newCenterC + dc;
            if (nr >= 0 && nr < newSize && nc >= 0 && nc < newSize)
                newVals[nr*newSize + nc] = oldVals[r*oldC + c];
        }
    setKernelTableSize(newSize);
    for (int r = 0; r < newSize; ++r)
        for (int c = 0; c < newSize; ++c)
            m_kernelTable->item(r,c)->setText(QString::number(newVals[r*newSize + c]));
}

void MainWindow::setProgress(int percent) {
    m_progressBar->setValue(percent);
}

