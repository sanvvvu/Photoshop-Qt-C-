#ifndef IMAGEINFOWIDGET_H
#define IMAGEINFOWIDGET_H

#include <QWidget>
#include <QImage>

class QLabel;

class ImageInfoWidget : public QWidget{
    Q_OBJECT
  public:
    explicit ImageInfoWidget(QWidget *parent = nullptr);
    void setImage(const QImage &image);
  private:
    QLabel *m_label;
    void updateInfo(const QImage &image);
};

#endif // IMAGEINFOWIDGET_H
