#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QTextBrowser;
class QPushButton;
QT_END_NAMESPACE

class HelpDialog : public QDialog {
    Q_OBJECT
public:
    explicit HelpDialog(QWidget *parent = nullptr);
    ~HelpDialog() override = default;

private:
    QTextBrowser *m_browser;
    QPushButton *m_closeBtn;
};

#endif
