#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
    QApplication a(argc, argv);
    QFontDatabase::addApplicationFont(":/fonts/Warcraft Font.ttf");
    QFontDatabase::addApplicationFont(":/fonts/FRIZQT_.TTF");
    QFontDatabase::addApplicationFont(":/fonts/folkard.ttf");
    MainWindow w;
    w.show();

    return a.exec();
}
