#ifndef SAVEIMAGEDIALOG_H
#define SAVEIMAGEDIALOG_H

#include <QDialog>
#include <QImage>
#include <QString>

// для ускорения компиляции
QT_BEGIN_NAMESPACE
class QComboBox;
class QSpinBox;
class QPushButton;
class QLabel;
class QLineEdit;
QT_END_NAMESPACE

class SaveImageDialog : public QDialog {
    Q_OBJECT
public:
    SaveImageDialog(const QImage &img, QWidget *parent = nullptr);
    ~SaveImageDialog() override = default;

private slots:
    void onFormatChanged(int idx);
    void onCompressionChanged(int idx);
    void onChooseFile();
    void onSave();
    void onHelpRequested();

private:
    void updateUIState();
    void populateCompressionOptions();                      // заполняет выпадающий список
    bool writeTIFFWithCompression(const QString &filename); // фактическая запись TIFF-файла с использованием выбранного алгоритма
    bool saveLZMA(const QString &filename);
    bool imageIsEffectivelyMonochrome(const QImage &img) const;
    bool imageIsGrayscale(const QImage &img) const;

    const QImage m_image;
    QString m_filename; // путь

    QComboBox *m_formatCombo; // PNG или TIFF
    QComboBox *m_compressionCombo; // все алгоритмы TIFF
    QSpinBox *m_levelSpin;
    QLabel *m_levelLabel;   // подпись для поля
    QLineEdit *m_fileEdit;  // строка с именем файла
    QPushButton *m_browseBtn;
    QPushButton *m_helpBtn; 
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};

bool saveImageWithDialog(const QImage &image, QWidget *parent = nullptr);

#endif
