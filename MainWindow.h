#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QVector>

class QLabel;
class QPushButton;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QTableWidget;
class QCheckBox;
class QProgressBar;
class ImageInfoWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    // UI элементы (основные)
    QWidget *m_central;
    QLabel *m_origLabel;
    QLabel *m_procLabel;
    QPushButton *m_loadBtn;
    QPushButton *m_applyBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_resetBtn;
    QComboBox *m_filterCombo;
    QSpinBox *m_kSizeSpin;
    QTableWidget *m_kernelTable;
    QCheckBox *m_normalizeBox;
    QDoubleSpinBox *m_contrastSpin;

    // дополнительные контролы для бинаризации
    QSpinBox *m_localWindowSpin; // для Ниблака и ИСОМАД
    QDoubleSpinBox *m_niblackKSpin;

    ImageInfoWidget *m_infoWidget;
    QProgressBar *m_progressBar;

    // состояния изображений
    QImage m_originalImage;
    QImage m_currentImage;

    // вспомогательные
    void setupUi();
    void connectSignals();
    void setKernelTableSize(int k);
    QVector<double> readKernelFromTable() const;
    void fillKernelFromVector(const QVector<double> &vec, int k);

private slots:
    void onLoadClicked();
    void onApplyClicked();
    void onApply(); // Реализация нужна для moc/линкера
    void onSaveClicked();
    void onResetClicked();
    void onFilterChanged(const QString &text);
    void onKernelSizeChanged(int newSize);
    void setProgress(int percent);
};

#endif
