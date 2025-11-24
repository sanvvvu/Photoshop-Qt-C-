#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // немного увеличила глобальный шрифт
    QFont globalFont = app.font();
    globalFont.setPointSize(globalFont.pointSize() + 1);
    app.setFont(globalFont);

    MainWindow w;
    w.show();

    return app.exec();
}
