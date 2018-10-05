#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "settings.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Ui::MainWindow *ui;
    Settings *settings=nullptr;
    QDialog *about=nullptr;
    QString renameModName;
    QTableWidgetItem *renameModItem;

    void setLaunchIcons();
    void getAllowFiles();
    void getGameVersion();
    void getMount(bool=false);
    bool modSelected();
    void status(QString, bool=false);

    QString result2statusMsg(QString, QString, int, int, int, bool=false, bool=false);
    std::string result2errorMsg(std::string, int, int, int, bool=false, bool=false);
    std::map<QString, QString> action_dict(QString);

    QIcon war3 = QIcon(":/icons/war3.png");
    QIcon war3mod = QIcon(":/icons/war3_mod.png");
    QIcon war3x = QIcon(":/icons/war3x.png");
    QIcon war3xmod = QIcon(":/icons/war3x_mod.png");
    QIcon worldedit = QIcon(":/icons/worldedit.png");
    QIcon worldeditmod = QIcon(":/icons/worldedit_mod.png");
    QIcon warningIcon = QIcon();

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Config *config = new Config();

private slots:
    void openGameFolder();
    void openModsFolder();
    void openSettings();
    void openAbout();
    void refresh(QString="", QString="", bool=true);
    void scanModUpdate(int, QString, QString);
    void launchGame();
    void launchEditor();
    void setAllowFiles();
    void setGameVersion();
    void mountMod();
    void mountModReady(QString, int, int, int, bool=false);
    void unmountMod();
    void unmountModReady(QString, int, int, int, bool=false, bool=false);
    void addMod();
    void addModReady(QString, int, int, int, bool=false);
    void openModFolder();
    void renameModAction();
    void renameModStart(QTableWidgetItem*);
    void renameModSave(QTableWidgetItem*);
    void deleteMod();
    void deleteModReady(QString, int, int, int, bool=false);
};

#endif // MAINWINDOW_H
