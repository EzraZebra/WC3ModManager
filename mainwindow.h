#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "settings.h"
#include <QMainWindow>
#include <QtWidgets>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void setAllowFiles();
    void setGameVersion();
    void launchGame();
    void launchEditor();
    void mountMod();
    void unmountMod();
    void openSettings();
    void openModFolder();
    void refresh(std::string="", QString="");
    void renameModStart(QTableWidgetItem*);
    void renameModSave(QTableWidgetItem*);
    void renameModAction();
    void deleteMod();
    void addMod(QString="");
    void openAbout();

public:
    Utils* utils = new Utils();
    Config* config = new Config(utils);

private:
    Ui::MainWindow *ui;
    Settings *settings = new Settings(this, config);
    std::string moveFile(QString, QString, bool=false);
    void status(std::string, bool=false);
    void setLaunchIcons();
    void getAllowFiles();
    void getGameVersion();
    void getMount(bool=false);
    bool modSelected();
    QIcon war3 = QIcon(":/war3.png");
    QIcon war3mod = QIcon(":/war3_mod.png");
    QIcon war3x = QIcon(":/war3x.png");
    QIcon war3xmod = QIcon(":/war3x_mod.png");
    QIcon worldedit = QIcon(":/worldedit.png");
    QIcon worldeditmod = QIcon(":/worldedit_mod.png");
    QString modName;
    QTableWidgetItem *modItem;
};

#endif // MAINWINDOW_H
