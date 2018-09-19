#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "settings.h"
#include <QMainWindow>
#include <QtWidgets>
#include <set>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Ui::MainWindow *ui;
    Utils *utils = new Utils();

    void setLaunchIcons();
    void getAllowFiles();
    void getGameVersion();
    void getMount(bool=false);
    bool modSelected();

    std::string moveFile(QString, QString, bool=false);
    void status(std::string, bool=false);

    QString renameModName;
    QTableWidgetItem *renameModItem;
    QString addModName;

    QIcon war3 = QIcon(":/war3.png");
    QIcon war3mod = QIcon(":/war3_mod.png");
    QIcon war3x = QIcon(":/war3x.png");
    QIcon war3xmod = QIcon(":/war3x_mod.png");
    QIcon worldedit = QIcon(":/worldedit.png");
    QIcon worldeditmod = QIcon(":/worldedit_mod.png");

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Config *config = new Config(utils);

private:
    Settings *settings = new Settings(this, config); //must be initialized after config

private slots:
    void openSettings();
    void openAbout();
    void refresh(std::string="", QString="");
    void launchGame();
    void launchEditor();
    void setAllowFiles();
    void setGameVersion();
    void mountMod();
    void mountModReady(int, int, int);
    void unmountMod();
    void unmountModReady(int, int, int);
    void addMod();
    void addModReady(int, int, int);
    void openModFolder();
    void renameModStart(QTableWidgetItem*);
    void renameModSave(QTableWidgetItem*);
    void renameModAction();
    void deleteMod();
    void deleteModReady(int, int, int);
};

#endif // MAINWINDOW_H
