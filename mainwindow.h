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
    QDialog *about = new QDialog(this);
    QString renameModName;
    QTableWidgetItem *renameModItem;

    void setLaunchIcons();
    void getAllowFiles();
    void getGameVersion();
    void getMount(bool=false);
    bool modSelected();
    void status(std::string, bool=false);

    QIcon war3 = QIcon(":/icons/war3.png");
    QIcon war3mod = QIcon(":/icons/war3_mod.png");
    QIcon war3x = QIcon(":/icons/war3x.png");
    QIcon war3xmod = QIcon(":/icons/war3x_mod.png");
    QIcon worldedit = QIcon(":/icons/worldedit.png");
    QIcon worldeditmod = QIcon(":/icons/worldedit_mod.png");

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Config *config = new Config(utils);

private:
    Settings *settings = new Settings(this, config); //must be initialized after config

private slots:
    void openSettings();
    void refresh(std::string="", QString="");
    void scanModUpdate(int, QString, QString);
    void launchGame();
    void launchEditor();
    void setAllowFiles();
    void setGameVersion();
    void mountMod();
    void mountModReady(QString, int, int, int);
    void unmountMod();
    void unmountModReady(QString, int, int, int);
    void addMod();
    void addModReady(QString, int, int, int);
    void openModFolder();
    void renameModStart(QTableWidgetItem*);
    void renameModSave(QTableWidgetItem*);
    void renameModAction();
    void deleteMod();
    void deleteModReady(QString, int, int, int);
};

#endif // MAINWINDOW_H
