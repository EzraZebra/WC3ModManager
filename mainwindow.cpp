#include "ui_mainwindow.h"
#include "ui_about.h"
#include "thread.h"
#include <sstream>
#include <QAction>

using namespace std;

MainWindow:: MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Icons
    warningIcon.addPixmap(QPixmap(":/icons/warning.png"), QIcon::Disabled);

    refresh("Setting up UI...");

    //About
    QDialog *about = new QDialog(this);
    Ui::About uiAbout;
    uiAbout.setupUi(about);
    about->setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    //Menubar
    connect(ui->actionOpenGameFolder, SIGNAL(triggered()), this, SLOT(openGameFolder()));
    connect(ui->actionOpenModsFolder, SIGNAL(triggered()), this, SLOT(openModsFolder()));
    connect(ui->actionSettings, SIGNAL(triggered()), this, SLOT(openSettings()));
    connect(ui->actionAbout, SIGNAL(triggered()), about, SLOT(show()));

    //Tools
    connect(ui->gameBtn, SIGNAL(clicked()), this, SLOT(launchGame()));
    connect(ui->editorBtn, SIGNAL(clicked()), this, SLOT(launchEditor()));
    connect(ui->allowFilesCbx, SIGNAL(stateChanged(int)), this, SLOT(setAllowFiles()));
    connect(ui->gameVersionCbx, SIGNAL(stateChanged(int)), this, SLOT(setGameVersion()));
    connect(ui->addModBtn, SIGNAL(clicked()), this, SLOT(addMod()));
    connect(ui->refreshBtn, SIGNAL(clicked()), this, SLOT(refresh()));

    //Modlist
    connect(ui->modList, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(renameModStart(QTableWidgetItem*)));
    QAction *actionMount = new QAction("Mount", ui->modList);
    ui->modList->addAction(actionMount);
    connect(actionMount, SIGNAL(triggered()), this, SLOT(mountMod()));
    ui->modList->addAction(ui->actionOpen);
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openModFolder()));
    ui->modList->addAction(ui->actionRename);
    connect(ui->actionRename, SIGNAL(triggered()), this, SLOT(renameModAction()));
    ui->modList->addAction(ui->actionDelete);
    connect(ui->actionDelete, SIGNAL(triggered()), this, SLOT(deleteMod()));

    status("Ready.");
}

void MainWindow::openGameFolder()
{
    status("Opening game folder...");

    if(QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(config->getSetting("GamePath")))))
        status("Game folder opened.");
    else status("Failed to open game folder.");
}

void MainWindow::openModsFolder()
{
    status("Opening mods folder...");

    if(QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(config->modPath))))
        status("Mods folder opened.");
    else status("Failed to open mods folder.");
}

void MainWindow::openSettings()
{
    if(!settings) settings = new Settings(this, config);
    settings->loadSettings();
    if(settings->exec() == 1) refresh("Settings saved.");
}

void MainWindow::refresh(string statusMsg, QString selectedMod, bool scanMods)
{
    getAllowFiles();
    getGameVersion();

    if(scanMods)
    {
        int selectedRow = -1;
        if(config->getSetting("Mounted") != "") selectedMod = QString::fromStdString(config->getSetting("Mounted"));
        else if(selectedMod == "") selectedRow = ui->modList->currentRow();

        ui->modList->clearContents();
        ui->modList->setRowCount(0);

        QDir dirMods(QString::fromStdString(config->modPath));
        dirMods.setFilter(QDir::NoDotAndDotDot|QDir::Dirs|QDir::NoSymLinks);
        QDirIterator itMods(dirMods);
        while(itMods.hasNext())
        {
            itMods.next();
            QString qsModSize = "0 MB", qsFileCount = "0 files",
                    modName = itMods.fileName();
            int row = ui->modList->rowCount();
            if(selectedMod == modName) selectedRow = row;

            ui->modList->insertRow(row);

            if(modName.toStdString() == config->getSetting("Mounted"))
            {
                qsModSize = QString::fromStdString(config->getSetting("MountedSize"));
                qsFileCount = QString::fromStdString(config->getSetting("MountedCount"));
            }
            else ui->modList->hideRow(row);

            ui->modList->setItem(row, 0, new QTableWidgetItem(modName));
            ui->modList->setItem(row, 1, new QTableWidgetItem(qsModSize));
            ui->modList->setItem(row, 2, new QTableWidgetItem(qsFileCount));

            if(modName.toStdString() != config->getSetting("Mounted"))
            {
                Controller *c = new Controller(this, "Scanning", modName);
                connect(c->worker, &Worker::scanModUpdate, this, &MainWindow::scanModUpdate);
                emit c->scanMod(row);
            }
            else
            {
                if(config->getSetting("MountedError") != "")
                {
                    QStringList errorList = QString::fromStdString(config->getSetting("MountedError")).split(";", QString::SkipEmptyParts);
                    if(errorList.size() > 0)
                    {
                        QString errorString = "Warning:";
                        for(int i=0; i < errorList.size(); i++)
                            if(i == 0 || errorList.at(i) != errorList.at(i-1))
                                errorString += "\n("+QString::number(i+1)+") "+errorList.at(i);

                        ui->modList->item(row, 0)->setIcon(warningIcon);
                        ui->modList->item(row, 0)->setToolTip(errorString);
                    }
                }

                ui->modList->resizeColumnToContents(0);

                if(selectedRow >= 0 && selectedRow < ui->modList->rowCount())
                {
                    ui->modList->scrollToItem(ui->modList->item(selectedRow, 0));
                    ui->modList->setFocus();
                }
            }
        }

        if(selectedRow >= 0 && selectedRow < ui->modList->rowCount())
            ui->modList->selectRow(selectedRow);
        else if(config->getSetting("Mounted") != "")
        {
            if(QDir().mkdir(QString::fromStdString(config->modPath)+"/"+selectedMod))
            {
                refresh(statusMsg, selectedMod);
                return;
            }
            else statusMsg = "Failed to create folder for "+selectedMod.toStdString()+".";
        }
    }

    getMount();

    if(statusMsg == "") status("WC3 Mod Manager refreshed.");
    else status(statusMsg);
}

void MainWindow::scanModUpdate(int row, QString modSize, QString fileCount)
{
    if(fileCount != "0 files" || modSize != "0 MB" || config->getSetting("hideEmptyMods") != "1")
    {
        if(modSize != "0 MB") ui->modList->item(row, 1)->setText(modSize);
        if(fileCount != "0 files") ui->modList->item(row, 2)->setText(fileCount);
        if(ui->modList->isRowHidden(row))
        {
            ui->modList->showRow(row);
            ui->modList->resizeColumnToContents(0);
            ui->modList->scrollToItem(ui->modList->item(ui->modList->currentRow(), 0));
            ui->modList->setFocus();
        }
    }
}

void MainWindow::launchGame()
{
    refresh("Launching game...", "", false);
    if(QDesktopServices::openUrl(QUrl::fromLocalFile(
            QString::fromStdString(config->getSetting("GamePath"))+"/Warcraft III.exe")))
        status("Game launched.");
    else status("Failed to launch Warcraft III.exe", true);
}

void MainWindow::launchEditor()
{
    refresh("Launching editor...", "", false);
    if(QDesktopServices::openUrl(QUrl::fromLocalFile(
            QString::fromStdString(config->getSetting("GamePath"))+"/World Editor.exe")))
        status("Editor launched.");
    else status("Failed to launch World Editor.exe", true);
}

void MainWindow::setLaunchIcons()
{
    if(ui->allowFilesCbx->isChecked() && config->getSetting("Mounted") != "")
    {
        if(ui->gameVersionCbx->isChecked()) ui->gameBtn->setIcon(war3xmod);
        else ui->gameBtn->setIcon(war3mod);
        ui->editorBtn->setIcon(worldeditmod);
    }
    else
    {
        if(ui->gameVersionCbx->isChecked()) ui->gameBtn->setIcon(war3x);
        else ui->gameBtn->setIcon(war3);
        ui->editorBtn->setIcon(worldedit);
    }
}

void MainWindow::setAllowFiles()
{
    DWORD allowFiles = ui->allowFilesCbx->isChecked() ? 1 : 0;

    if(utils->regSet(L"Allow Local Files", allowFiles))
    {
        if(allowFiles == 1) status("Allow Local Files enabled.");
        else status("Allow Local Files disabled.");
    }
    else status("Failed to set Allow Local Files.", true);

    getAllowFiles();
}

void MainWindow::getAllowFiles()
{
    string allowFiles = utils->regGet(L"Allow Local Files", REG_DWORD);

    if(allowFiles=="0") ui->allowFilesCbx->setChecked(false);
    else if(allowFiles=="1") ui->allowFilesCbx->setChecked(true);
    else status("Failed to get Allow Local Files setting.");
    setLaunchIcons();
}

void MainWindow::setGameVersion()
{
    DWORD gameVersion = ui->gameVersionCbx->isChecked() ? 1 : 0;

    if(utils->regSet(L"Preferred Game Version", gameVersion))
    {
        if(gameVersion == 1) status("Expansion enabled.");
        else status("Expansion disabled.");
    }
    else status("Failed to set Preferred Game Version.", true);

    getGameVersion();
}

void MainWindow::getGameVersion()
{
    string gameVersion = utils->regGet(L"Preferred Game Version", REG_DWORD);

    if(gameVersion == "0") ui->gameVersionCbx->setChecked(false);
    else if(gameVersion == "1") ui->gameVersionCbx->setChecked(true);
    else status("Failed to get Preferred Game Version.");

    setLaunchIcons();
}

void MainWindow::mountMod()
{
    bool mounting = false;
    ui->toggleMountBtn->setEnabled(false);
    ui->modList->setEnabled(false);
    ui->gameBtn->setEnabled(false);
    ui->editorBtn->setEnabled(false);

    if(config->getSetting("Mounted") == "")
    {
        int iSelectedMod = ui->modList->currentRow();
        if(iSelectedMod >= 0 && iSelectedMod < ui->modList->rowCount())
        {
            string gamePath = config->getSetting("GamePath");
            QString selectedMod = ui->modList->item(iSelectedMod, 0)->text();

            status("Mounting "+selectedMod.toStdString()+"...");

            if(QFileInfo(QString::fromStdString(gamePath)).isDir())
            {
                config->setSetting("Mounted", selectedMod.toStdString());
                config->setSetting("MountedTo", gamePath);
                config->setSetting("MountedSize", ui->modList->item(iSelectedMod, 1)->text().toStdString());
                config->setSetting("MountedCount", ui->modList->item(iSelectedMod, 2)->text().toStdString());
                config->saveConfig();

                Controller *c = new Controller(this, "Mounting", selectedMod);
                connect(c->worker, &Worker::resultReady, this, &MainWindow::mountModReady);
                emit c->moveFolder(QString::fromStdString(config->modPath)+"/"+selectedMod, QString::fromStdString(gamePath), true);
                mounting = true;
            }
            else status("Invalid Warcraft III folder.", true);
        }
        else status("Select a mod to mount.");
    } else status("Already mounted: "+config->getSetting("Mounted"));

    if(!mounting) getMount(true);
}

void MainWindow::mountModReady(QString modName, int success, int failed, int missing, bool abort)
{
    bool scanMods = false;
    if(success > 0)
    {
        if(abort || failed+missing > 0)
        {
            scanMods = true;
            config->setSetting("MountedError", result2errorMsg("Mounting", success, failed, missing, abort));
            config->saveConfig();
        }
    }
    else
    {
        config->deleteSetting("Mounted");
        config->deleteSetting("MountedTo");
        config->deleteSetting("MountedSize");
        config->deleteSetting("MountedCount");
        config->deleteSetting("MountedError");
        config->saveConfig();
    }

    refresh(result2statusMsg(modName.toStdString(), "Mounting", success, failed, missing, abort), modName, scanMods);
}

void MainWindow::unmountMod()
{
    ui->toggleMountBtn->setEnabled(false);
    ui->gameBtn->setEnabled(false);
    ui->editorBtn->setEnabled(false);

    if(config->getSetting("Mounted") != "")
    {
        status("Unmounting "+config->getSetting("Mounted")+"...");

        Controller *c = new Controller(this, "Unmounting", QString::fromStdString(config->getSetting("Mounted")));
        connect(c->worker, &Worker::resultReady, this, &MainWindow::unmountModReady);
        emit c->unmountMod();
    }
    else
    {
        status("No mod mounted.");
        getMount(true);
    }
}

void MainWindow::unmountModReady(QString modName, int success, int failed, int missing, bool abort, bool force)
{
    bool scanMods = false;
    if(!force && (abort || failed+missing > 0))
    {
        string errorMsg = result2errorMsg("Unmounting", success, failed, missing, abort, force);
        if(config->getSetting("MountedError") != "") errorMsg = config->getSetting("MountedError")+";"+errorMsg;
        config->setSetting("MountedError", errorMsg);
    }
    else
    {
        scanMods = true;
        config->deleteSetting("Mounted");
        config->deleteSetting("MountedTo");
        config->deleteSetting("MountedSize");
        config->deleteSetting("MountedCount");
        config->deleteSetting("MountedError");
    }

    config->saveConfig();

    refresh(result2statusMsg(modName.toStdString(), "Unmounting", success, failed, missing, abort, force), modName, scanMods);
}

void MainWindow::getMount(bool setFocus)
{
    disconnect(ui->toggleMountBtn, SIGNAL(clicked()), this, SLOT(unmountMod()));
    disconnect(ui->toggleMountBtn, SIGNAL(clicked()), this, SLOT(mountMod()));

    if(config->getSetting("Mounted") == "")
    {
        ui->toggleMountBtn->setText("&Mount");
        ui->modList->setStyleSheet("");
        ui->modList->setEnabled(true);
        if(setFocus) ui->modList->setFocus();
        connect(ui->toggleMountBtn, SIGNAL(clicked()), this, SLOT(mountMod()));
    }
    else
    {
        ui->toggleMountBtn->setText("Un&mount");
        ui->modList->setStyleSheet(tr("QTableWidget::item:selected { border: 2px dashed #bcbe00;")
                                  +tr("border-top-color: #f7f500; border-bottom-color: #f7f500;")
                                  +tr("color: #eee; background-color: #555; }"));
        ui->modList->setEnabled(false);

        connect(ui->toggleMountBtn, SIGNAL(clicked()), this, SLOT(unmountMod()));
    }

    ui->toggleMountBtn->setEnabled(true);
    setLaunchIcons();
    ui->gameBtn->setEnabled(true);
    ui->editorBtn->setEnabled(true);
}

void MainWindow::addMod()
{
    status("Adding mod...");
    QString qsFolder = QFileDialog::getExistingDirectory(this, tr("Add Mod"), "",
                                                        QFileDialog::ShowDirsOnly | QFileDialog::HideNameFilterDetails);

    if(qsFolder != "")
    {
        QMessageBox copyMove(this);
        copyMove.setWindowTitle("Copy or move?");
        copyMove.setText("Do you want to copy or move this folder?\n"+qsFolder);
        copyMove.addButton("Copy", QMessageBox::ActionRole);
        copyMove.addButton("Move", QMessageBox::ActionRole);
        copyMove.addButton(QMessageBox::Cancel);
        copyMove.setIcon(QMessageBox::Warning);

        int result = copyMove.exec();

        if(result == 0 || result == 1)
        {
            QString modName = QDir(qsFolder).dirName(),
                    qsNewFolder = QString::fromStdString(config->modPath)+"/"+modName;
            if(!QDir(qsNewFolder).exists())
            {
                status("Adding "+modName.toStdString()+"...");

                Controller *c = new Controller(this, "Adding", modName);
                connect(c->worker, &Worker::resultReady, this, &MainWindow::addModReady);
                emit c->moveFolder(qsFolder, qsNewFolder, false, result == 0);
            }
            else status("A mod with that name already exists.", true);
        }
    }
}

void MainWindow::addModReady(QString modName, int success, int failed, int missing, bool abort)
{
    refresh(result2statusMsg(modName.toStdString(), "Adding", success, failed, missing, abort), modName);
}

void MainWindow::openModFolder()
{
    QString path = QString::fromStdString(config->modPath);
    string dirName = "mods";

    if(modSelected())
    {
        QString modName = ui->modList->item(ui->modList->currentRow(), 0)->text();
        path += "/"+modName;
        dirName = modName.toStdString();
    }

    status("Opening "+dirName+" folder...");

    if(QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        status(dirName+" folder opened.");
    else status("Failed to open "+dirName+" folder.");
}

void MainWindow::renameModAction()
{
    if(modSelected()) renameModStart(ui->modList->item(ui->modList->currentRow(), 0));
}

void MainWindow::renameModStart(QTableWidgetItem *item)
{
    if(item->column() != 0) item = ui->modList->item(item->row(), 0);
    renameModName = item->text();
    renameModItem = item;
    connect(ui->modList, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(renameModSave(QTableWidgetItem*)));
    ui->modList->editItem(item);
}

void MainWindow::renameModSave(QTableWidgetItem *item)
{
    disconnect(ui->modList, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(renameModSave(QTableWidgetItem*)));

    string statusMsg = "";
    QString selectedMod = renameModName;
    if(item == renameModItem)
    {
        QString qsModPath = QString::fromStdString(config->modPath);
        if(QFileInfo(qsModPath+"/"+renameModName).isDir())
        {
            QString newModName = item->text();
            if(!QDir().rename(qsModPath+"/"+renameModName, qsModPath+"/"+newModName))
                statusMsg = "Failed to rename mod.";
            else
            {
                selectedMod = newModName;
                statusMsg = renameModName.toStdString()+" renamed to "+newModName.toStdString()+".";
            }
        }
        else statusMsg = "Mod not found.";
    }
    else statusMsg = "Saving wrong item.";

    renameModName = "";
    renameModItem = nullptr;
    refresh(statusMsg, selectedMod);
}

void MainWindow::deleteMod()
{
    if(modSelected())
    {
        QString qsModName = ui->modList->item(ui->modList->currentRow(), 0)->text();

        if(QMessageBox::warning(this, "Permanently delete "+qsModName+"?",
                "Are you sure you want to PERMANENTLY delete "+qsModName+" ("
               +ui->modList->item(ui->modList->currentRow(), 1)->text()+" / "
               +ui->modList->item(ui->modList->currentRow(), 2)->text()+")?",
                QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
        {
            string sModName = qsModName.toStdString();
            status("Deleting "+sModName+"...");

            Controller *c = new Controller(this, "Deleting", qsModName);
            connect(c->worker, &Worker::resultReady, this, &MainWindow::deleteModReady);
            emit c->deleteMod();
        }
    }
}

void MainWindow::deleteModReady(QString modName, int success, int failed, int missing, bool abort)
{
    refresh(result2statusMsg(modName.toStdString(), "Deleting", success, failed, missing, abort), modName);
}

bool MainWindow::modSelected()
{
    if(ui->modList->currentRow() < 0 || ui->modList->currentRow() >= ui->modList->rowCount())
    {
        status("Select a mod first.");
        return false;
    }
    return true;
}

void MainWindow::status(string msg, bool warning)
{
    ui->statusBarLbl->setText(QString::fromStdString(msg));
    if(warning) QMessageBox::warning(this, tr("Error"), tr(msg.c_str()));
}

string MainWindow::result2statusMsg(string modName, string action,int success, int failed, int missing, bool abort, bool force)
{
    map<string, string> dict = action_dict(action);
    string statusMsg,
           pAction = dict.find("pAction")->second,
           cAction = dict.find("cAction")->second;
    int totErrors = failed+missing;

    if(abort) statusMsg = action+" "+modName+": aborted.";
    else if((force && totErrors > 0) || (success > 0 && !(action == "Unmounting" && totErrors > 0)))
        statusMsg = modName+" "+pAction+".";
    else if(totErrors > 0) statusMsg = "Failed to "+cAction+" "+modName+".";
    else statusMsg = "No files to "+cAction+".";

    if(abort || totErrors > 0)
    {
        statusMsg += " ["+utils->int2string(success)+" files "+pAction;
        if(failed > 0) statusMsg += ", "+utils->int2string(failed)+" files failed";
        if(missing > 0) statusMsg += ", "+utils->int2string(missing)+" files missing";
        statusMsg += "]";
    }

    return statusMsg;
}

string MainWindow::result2errorMsg(string action,int success, int failed, int missing, bool abort, bool force)
{
    map<string, string> dict = action_dict(action);
    string errorMsg,
           pAction = dict.find("pAction")->second,
           cAction = dict.find("cAction")->second,
           pAction_c = dict.find("pAction_c")->second;
    int totErrors = failed+missing;

    if(abort) errorMsg = action+" aborted";
    else if(force) "Force "+cAction;
    else if(success > 0 && !(action == "Unmounting" && totErrors > 0))
        errorMsg = pAction_c;
    else if(totErrors > 0) errorMsg = action+" failed";
    else errorMsg = "No files to "+cAction;

    errorMsg += ": "+utils->int2string(success)+" files "+pAction;
    if(failed > 0) errorMsg += ", "+utils->int2string(failed)+" files failed";
    if(missing > 0) errorMsg += ", "+utils->int2string(missing)+" files missing";

    return errorMsg;
}

map<string, string> MainWindow::action_dict(string action)
{
    if(action == "Mounting")
        return {
            { "pAction", "mounted" },
            { "pAction_c", "Mounted" },
            { "cAction", "mount" }
        };
    else if(action == "Unmounting")
        return {
            { "pAction", "unmounted" },
            { "pAction_c", "Unmounted" },
            { "cAction", "unmount" }
        };
    else if(action == "Adding")
        return {
            { "pAction", "added" },
            { "pAction_c", "Added" },
            { "cAction", "add" }
        };
    else if(action == "Deleting")
        return {
            { "pAction", "deleted" },
            { "pAction_c", "Deleted" },
            { "cAction", "delete" }
        };
    else
        return {
            { "pAction", "processed" },
            { "pAction_c", "Processed" },
            { "cAction", "process" }
        };
}

MainWindow::~MainWindow()
{
    delete ui;
}
