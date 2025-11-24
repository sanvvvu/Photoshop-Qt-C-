#include "filter.h"
#include "ImageInfoWidget.h"
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QImage>
#include <QPixmap>
#include <QFileDialog>
#include <QScrollArea>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QCheckBox>
#include <QFont>
#include <QStyleOption>
#include <QPainter>
#include <vector>
#include <cmath>

// Пресеты ядер 
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

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // небольшой глобальный шрифт чуть больше
    QFont globalFont = app.font();
    globalFont.setPointSize(globalFont.pointSize() + 1);
    app.setFont(globalFont);

    QWidget win;
    win.setWindowTitle(QObject::tr("Пример фильтрации изображения"));
    win.setMinimumSize(900, 700);

    // Стиль: нежный бело-кремовый фон с красными акцентами
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
    app.setStyleSheet(style);

    auto mainLayout = new QHBoxLayout(&win);

    // Левая панель - изображения
    auto imagesBox = new QGroupBox(QObject::tr("Изображения"));
    auto imagesLayout = new QVBoxLayout(imagesBox);
    QLabel *origLabel = new QLabel();
    QLabel *procLabel = new QLabel();
    origLabel->setAlignment(Qt::AlignCenter);
    procLabel->setAlignment(Qt::AlignCenter);

    QScrollArea *origScroll = new QScrollArea();
    origScroll->setWidgetResizable(true);
    QWidget *origHolder = new QWidget();
    QVBoxLayout *ohL = new QVBoxLayout(origHolder);
    ohL->addWidget(origLabel);
    origScroll->setWidget(origHolder);

    QScrollArea *procScroll = new QScrollArea();
    procScroll->setWidgetResizable(true);
    QWidget *procHolder = new QWidget();
    QVBoxLayout *phL = new QVBoxLayout(procHolder);
    phL->addWidget(procLabel);
    procScroll->setWidget(procHolder);

    imagesLayout->addWidget(new QLabel(QObject::tr("Исходное изображение:")));
    imagesLayout->addWidget(origScroll, 1);
    imagesLayout->addWidget(new QLabel(QObject::tr("Обработанное изображение:")));
    imagesLayout->addWidget(procScroll, 1);

    mainLayout->addWidget(imagesBox, 3);

    // Правая панель - контролы
    auto controlsBox = new QGroupBox(QObject::tr("Управление"));
    auto rightBox = new QVBoxLayout(controlsBox);

    QPushButton *loadBtn = new QPushButton(QObject::tr("Загрузить изображение..."));
    QPushButton *applyBtn = new QPushButton(QObject::tr("Применить фильтр"));
    QPushButton *saveBtn = new QPushButton(QObject::tr("Сохранить результат..."));
    QPushButton *resetBtn = new QPushButton(QObject::tr("Сброс"));

    QComboBox *filterCombo = new QComboBox();
    filterCombo->addItem(QObject::tr("Нет"));
    filterCombo->addItem(QObject::tr("Блюр"));
    filterCombo->addItem(QObject::tr("Резкость"));
    filterCombo->addItem(QObject::tr("Контраст"));
    filterCombo->addItem(QObject::tr("Пользовательский"));

    // kernel controls
    QLabel *kSizeLabel = new QLabel(QObject::tr("Размер ядра (нечётное):"));
    QSpinBox *kSizeSpin = new QSpinBox();
    kSizeSpin->setRange(1, 11);
    kSizeSpin->setSingleStep(2);
    kSizeSpin->setValue(3);

    QTableWidget *kernelTable = new QTableWidget(3, 3);
    kernelTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    kernelTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QCheckBox *normalizeBox = new QCheckBox(QObject::tr("Нормировать ядро (сумма = 1)"));
    normalizeBox->setChecked(true);

    // contrast control (shown when needed)
    QLabel *contrastLabel = new QLabel(QObject::tr("Коэффициент контраста:"));
    QDoubleSpinBox *contrastSpin = new QDoubleSpinBox();
    contrastSpin->setRange(0.1, 5.0);
    contrastSpin->setSingleStep(0.1);
    contrastSpin->setValue(1.2);

    // Info widget
    ImageInfoWidget *infoWidget = new ImageInfoWidget();

    rightBox->addWidget(loadBtn);
    rightBox->addSpacing(6);
    rightBox->addWidget(new QLabel(QObject::tr("Выберите фильтр:")));
    rightBox->addWidget(filterCombo);
    rightBox->addSpacing(6);

    // kernel area
    rightBox->addWidget(kSizeLabel);
    rightBox->addWidget(kSizeSpin);
    rightBox->addWidget(kernelTable, 1);
    rightBox->addWidget(normalizeBox);

    // contrast area
    rightBox->addWidget(contrastLabel);
    rightBox->addWidget(contrastSpin);
    contrastLabel->setVisible(false);
    contrastSpin->setVisible(false);

    rightBox->addWidget(applyBtn);
    rightBox->addWidget(saveBtn);
    rightBox->addWidget(resetBtn);
    rightBox->addStretch();
    rightBox->addWidget(infoWidget);

    mainLayout->addWidget(controlsBox, 1);

    // состояние изображений
    QImage originalImage;
    QImage currentImage;

    auto setKernelTableSize = [&](int k) {
        kernelTable->setRowCount(k);
        kernelTable->setColumnCount(k);
        for (int r = 0; r < k; ++r) {
            for (int c = 0; c < k; ++c) {
                if (!kernelTable->item(r,c)) {
                    kernelTable->setItem(r, c, new QTableWidgetItem(QString::number(0.0)));
                }
            }
        }
    };

    auto fillKernelFromVector = [&](const QVector<double> &vec, int k) {
        setKernelTableSize(k);
        if (vec.size() == k * k) {
            for (int r = 0; r < k; ++r)
                for (int c = 0; c < k; ++c)
                    kernelTable->item(r,c)->setText(QString::number(vec[r * k + c]));
        } else {
            // иначе записываем "единицей в центре"
            for (int r = 0; r < k; ++r)
                for (int c = 0; c < k; ++c)
                    kernelTable->item(r,c)->setText((r==k/2 && c==k/2) ? "1" : "0");
        }
    };

    // Инициализация начального ядра 3x3 (box)
    fillKernelFromVector(QVector<double>(box3x3, box3x3 + 9), 3);

    // видимость контролов при выборе фильтра
    QObject::connect(filterCombo, &QComboBox::currentTextChanged, [&](const QString &text){
        bool isContrast = text.contains(QObject::tr("Контраст"));
        bool isCustom = text.contains(QObject::tr("Пользовательский"));
        bool isKernelPreset = text.contains(QObject::tr("Блюр")) || text.contains(QObject::tr("Резкость")) || isCustom;

        kSizeLabel->setVisible(isKernelPreset);
        kSizeSpin->setVisible(isKernelPreset);
        kernelTable->setVisible(isKernelPreset);
        normalizeBox->setVisible(isKernelPreset);

        contrastLabel->setVisible(isContrast);
        contrastSpin->setVisible(isContrast);

        // при выборе предустановки — заполнить таблицу и размер
        if (!isCustom && isKernelPreset && !isContrast) {
            int presetK = 3;
            QVector<double> vec = getPreset(text, presetK);
            kSizeSpin->setValue(presetK);
            fillKernelFromVector(vec, presetK);
        } else if (isCustom) {
            // ничего не делаем: оставим текущее ядро для правки
        }
    });

    // при изменении размера ядра — пересоздать таблицу (с сохранением центра, если возможно)
    QObject::connect(kSizeSpin, qOverload<int>(&QSpinBox::valueChanged), [&](int newSize){
        if (newSize % 2 == 0) { // гарантия нечетности — если пользователь установил четное (защита)
            newSize = newSize + 1;
            kSizeSpin->setValue(newSize);
            return;
        }
        // создаём новую таблицу, попытавшись скопировать центр старой
        int oldR = kernelTable->rowCount();
        int oldC = kernelTable->columnCount();
        QVector<double> oldVals;
        for (int r = 0; r < oldR; ++r)
            for (int c = 0; c < oldC; ++c)
                oldVals.append(kernelTable->item(r,c)->text().toDouble());

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
                kernelTable->item(r,c)->setText(QString::number(newVals[r*newSize + c]));
    });

    // Загрузка изображения
    QObject::connect(loadBtn, &QPushButton::clicked, [&]() {
        QString fileName = QFileDialog::getOpenFileName(&win, QObject::tr("Открыть изображение"), QString(),
                                                        QObject::tr("Images (*.png *.jpg *.bmp)"));
        if (!fileName.isEmpty()) {
            originalImage.load(fileName);
            currentImage = originalImage;
            origLabel->setPixmap(QPixmap::fromImage(originalImage).scaled(600, 400, Qt::KeepAspectRatio));
            procLabel->setPixmap(QPixmap::fromImage(currentImage).scaled(600, 400, Qt::KeepAspectRatio));
            infoWidget->setImage(currentImage);

            // включаем контролы
            filterCombo->setEnabled(true);
            applyBtn->setEnabled(true);
            saveBtn->setEnabled(true);
            resetBtn->setEnabled(true);
            kSizeSpin->setEnabled(true);
            kernelTable->setEnabled(true);
        }
    });

    // функция чтения ядра из таблицы в вектор
    auto readKernelFromTable = [&]() -> QVector<double> {
        int k = kernelTable->rowCount();
        QVector<double> vec(k*k);
        for (int r = 0; r < k; ++r)
            for (int c = 0; c < k; ++c)
                vec[r*k + c] = kernelTable->item(r,c)->text().toDouble();
        return vec;
    };

    // применить
    QObject::connect(applyBtn, &QPushButton::clicked, [&]() {
        if (currentImage.isNull()) return;
        QString sel = filterCombo->currentText();

        if (sel.contains(QObject::tr("Контраст"))) {
            double factor = contrastSpin->value();
            adjustContrast(currentImage, factor);
        } else if (sel.contains(QObject::tr("Нет"))) {
            // ничего не делаем
        } else {
            int k = kernelTable->rowCount();
            QVector<double> vec = readKernelFromTable();

            // Нормализация: если чекбокс включён — привести ядро к сумме = 1.
            // Если сумма == 0, то будет выполнено приведение по сумме абсолютных значений.
            // После нормализации записываем значения обратно в таблицу, чтобы UI обновился.
            if (normalizeBox->isChecked()) {
                double s = 0.0;
                for (double v : vec) s += v;
                if (std::abs(s) > 1e-12) {  // в пределах машинной точности
                    for (int i = 0; i < vec.size(); ++i) vec[i] /= s;
                } else {
                    double sabs = 0.0;
                    for (double v : vec) sabs += std::abs(v);
                    if (sabs > 1e-12) {
                        for (int i = 0; i < vec.size(); ++i) vec[i] /= sabs;
                    } else {
                        // все элементы нулевые — оставляем как есть
                    }
                }

                int k = kernelTable->rowCount();
                if (k * k == vec.size()) {
                    for (int r = 0; r < k; ++r) {
                        for (int c = 0; c < k; ++c) {
                            // форматируем, чтобы не было долгих дробей (например 6 знаков)
                            kernelTable->item(r, c)->setText(QString::number(vec[r * k + c], 'f', 6));
                        }
                    }
                }
            }

            // вызов фильтра
            filter2D(currentImage, vec.data(), k, k);
        }

        procLabel->setPixmap(QPixmap::fromImage(currentImage).scaled(600, 400, Qt::KeepAspectRatio));
        infoWidget->setImage(currentImage);
    });

    QObject::connect(saveBtn, &QPushButton::clicked, [&]() {
        if (currentImage.isNull()) return;
        QString fileName = QFileDialog::getSaveFileName(&win, QObject::tr("Сохранить изображение"), QString(),
                                                        QObject::tr("PNG Image (*.png)"));
        if (!fileName.isEmpty()) {
            if (!fileName.endsWith(".png", Qt::CaseInsensitive)) fileName += ".png";
            currentImage.save(fileName, "PNG");
        }
    });

    QObject::connect(resetBtn, &QPushButton::clicked, [&]() {
        if (originalImage.isNull()) return;
        currentImage = originalImage;
        procLabel->setPixmap(QPixmap::fromImage(currentImage).scaled(600, 400, Qt::KeepAspectRatio));
        infoWidget->setImage(currentImage);
    });

    // в начале деактивируем контролы, пока не загружено изображение
    filterCombo->setEnabled(false);
    applyBtn->setEnabled(false);
    saveBtn->setEnabled(false);
    resetBtn->setEnabled(false);
    kSizeSpin->setEnabled(false);
    kernelTable->setEnabled(false);

    win.show();
    return app.exec();
}
