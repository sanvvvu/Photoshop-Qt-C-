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
#include <vector>
// Ядра фильтров
static double box3x3[] = {
    1/9.0, 1/9.0, 1/9.0,
    1/9.0, 1/9.0, 1/9.0,
    1/9.0, 1/9.0, 1/9.0
};
static double sharpen3x3[] = {
     0, -0.4,  0,
    -1,  5, -1,
     0, -1,  0
};
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QWidget win;
    win.setWindowTitle(QObject::tr("Пример фильтрации изображения"));
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

    // Правая панель - кнопки
    auto rightBox = new QVBoxLayout();
    QPushButton *loadBtn = new QPushButton(QObject::tr("Загрузить изображение..."));
    QPushButton *boxBtn = new QPushButton(QObject::tr("Фильтр блюра"));
    QPushButton *sharpenBtn = new QPushButton(QObject::tr("Фильтр резкости"));
    QPushButton *contrastBtn = new QPushButton(QObject::tr("Фильтр контраста"));
    QPushButton *resetBtn = new QPushButton(QObject::tr("Сброс"));

    ImageInfoWidget *infoWidget = new ImageInfoWidget();

    rightBox->addWidget(loadBtn);
    rightBox->addWidget(boxBtn);
    rightBox->addWidget(sharpenBtn);
    rightBox->addWidget(contrastBtn);
    rightBox->addWidget(resetBtn);
    rightBox->addStretch();
    rightBox->addWidget(infoWidget);

    mainLayout->addLayout(rightBox, 1);

    QImage originalImage;
    QImage currentImage;

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
        }
    });

    // Применение фильтров (накопительно)
    QObject::connect(boxBtn, &QPushButton::clicked, [&]() {
        filter2D(currentImage, box3x3, 3, 3);
        procLabel->setPixmap(QPixmap::fromImage(currentImage).scaled(600, 400, Qt::KeepAspectRatio));
        infoWidget->setImage(currentImage);
    });

    QObject::connect(sharpenBtn, &QPushButton::clicked, [&]() {
        filter2D(currentImage, sharpen3x3, 3, 3);
        procLabel->setPixmap(QPixmap::fromImage(currentImage).scaled(600, 400, Qt::KeepAspectRatio));
        infoWidget->setImage(currentImage);
    });

    QObject::connect(contrastBtn, &QPushButton::clicked, [&]() {
        adjustContrast(currentImage, 1.2);
        procLabel->setPixmap(QPixmap::fromImage(currentImage).scaled(600, 400, Qt::KeepAspectRatio));
        infoWidget->setImage(currentImage);
    });

    QObject::connect(resetBtn, &QPushButton::clicked, [&]() {
        currentImage = originalImage;
        procLabel->setPixmap(QPixmap::fromImage(currentImage).scaled(600, 400, Qt::KeepAspectRatio));
        infoWidget->setImage(currentImage);
    });

    win.resize(900, 700);
    win.show();
    
    return app.exec();
}
