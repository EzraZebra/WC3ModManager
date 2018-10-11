#include "mainwindow.h"
#include "ui_splashscreen.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setAttribute(Qt::AA_DisableWindowContextHelpButton);

    QMainWindow *splash = new QMainWindow();
    Ui::SplashScreen uiSplashScreen;
    uiSplashScreen.setupUi(splash);
    splash->setWindowFlags(Qt::SplashScreen|Qt::WindowStaysOnTopHint);
    splash->statusBar()->showMessage("Starting...");
    splash->show();

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList() << "l" << "launch", "Mod to launch. Enter \"\" to launch without mod.", "mod", ""));
    parser.addOption(QCommandLineOption(QStringList() << "v" << "game-version",
                                        "Preferred Game Version. 0: vanilla, 1: expansion.", "version", ""));
    parser.addOption(QCommandLineOption(QStringList() << "e" << "editor", "Launch World Editor."));
    parser.process(a);

    MainWindow w(parser.isSet("launch"), splash);
    if(parser.isSet("launch")) w.shortcutLaunch(parser.value("launch"), parser.value("game-version"), parser.isSet("editor"));
    else w.show();

    return a.exec();
}
