#include "ui_mainwindow.h"
#include "ui_about.h"
#include "thread.h"
#include "utils.h"

MainWindow:: MainWindow(bool newLaunchingShortcut, QMainWindow *newSplash, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    shortcutMode = newLaunchingShortcut;
    splash = newSplash;

    if(shortcutMode) connect(this, &MainWindow::shortcutQuit, qApp, &QApplication::quit, Qt::QueuedConnection);
    else
    {
        status("Setting up UI...");
        ui->setupUi(this);

        //Icons
        warningIcon.addPixmap(QPixmap(":/icons/warning.png"), QIcon::Disabled);

        refresh();

        //Menubar
        connect(ui->actionOpenGameFolder, SIGNAL(triggered()), this, SLOT(openGameFolder()));
        connect(ui->actionOpenModsFolder, SIGNAL(triggered()), this, SLOT(openModsFolder()));
        connect(ui->actionSettings, SIGNAL(triggered()), this, SLOT(openSettings()));
        connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(openAbout()));

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

        splash->close();
        delete splash;
        splash = nullptr;
        status("Ready.");
    }
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
    if(settings->exec() == 1) refresh("Settings saved.");
}

void MainWindow::openAbout()
{
    if(!about)
    {
        about = new QDialog(this);
        Ui::About uiAbout;
        uiAbout.setupUi(about);
        about->setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);
    }
    about->show();
}

void MainWindow::refresh(QString statusMsg, QString selectedMod, bool scanMods)
{
    if(!shortcutMode)
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

            QDirIterator itMods(QString::fromStdString(config->modPath), QDir::NoDotAndDotDot|QDir::Dirs);
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
                else statusMsg = "Failed to create folder for "+selectedMod+".";
            }
        }

        getMount();
    }

    if(statusMsg == "")
    {
        if(!shortcutMode) status("WC3 Mod Manager refreshed.");
    }
    else status(statusMsg);
}

void MainWindow::scanModUpdate(QString modSize, QString fileCount, int row)
{
    if(modSize != "0 MB") ui->modList->item(row, 1)->setText(modSize);
    if(fileCount != "0 files") ui->modList->item(row, 2)->setText(fileCount);
    if(ui->modList->isRowHidden(row) && (fileCount != "0 files" || modSize != "0 MB" || config->getSetting("hideEmptyMods") != "1"))
    {
        ui->modList->showRow(row);
        ui->modList->resizeColumnToContents(0);
        ui->modList->scrollToItem(ui->modList->item(ui->modList->currentRow(), 0));
        ui->modList->setFocus();
    }
}

void MainWindow::launchGame()
{
    refresh("Launching game...", "", false);
    if(QDesktopServices::openUrl(QUrl::fromLocalFile(
            QString::fromStdString(config->getSetting("GamePath"))+"/Warcraft III.exe")))
    {
        status("Game launched.");
        if(shortcutMode) emit shortcutQuit();
    }
    else status("Failed to launch Warcraft III.exe", true);
}

void MainWindow::launchEditor()
{
    refresh("Launching editor...", "", false);
    if(QDesktopServices::openUrl(QUrl::fromLocalFile(
            QString::fromStdString(config->getSetting("GamePath"))+"/World Editor.exe")))
    {
        status("Editor launched.");
        if(shortcutMode) emit shortcutQuit();
    }
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

void MainWindow::setAllowFiles(QString alwFls)
{
    DWORD allowFiles = alwFls == "0" ? 0 : alwFls == "1" ? 1 : ui->allowFilesCbx->isChecked() ? 1 : 0;

    if(utils::regSet(L"Allow Local Files", allowFiles))
    {
        if(allowFiles == 1) status("Allow Local Files enabled.");
        else status("Allow Local Files disabled.");
    }
    else status("Failed to set Allow Local Files.", true);

    if(!shortcutMode) getAllowFiles();
}

void MainWindow::getAllowFiles()
{
    std::string allowFiles = utils::regGet(L"Allow Local Files", REG_DWORD);

    if(allowFiles=="0") ui->allowFilesCbx->setChecked(false);
    else if(allowFiles=="1") ui->allowFilesCbx->setChecked(true);
    else status("Failed to get Allow Local Files setting.");
    setLaunchIcons();
}

void MainWindow::setGameVersion(QString gVrs)
{
    DWORD gameVersion = gVrs == "0" ? 0 : gVrs == "1" ? 1 : ui->gameVersionCbx->isChecked() ? 1 : 0;

    if(utils::regSet(L"Preferred Game Version", gameVersion))
    {
        if(gameVersion == 1) status("Expansion enabled.");
        else status("Expansion disabled.");
    }
    else status("Failed to set Preferred Game Version.", true);

    if(!shortcutMode) getGameVersion();
}

void MainWindow::getGameVersion()
{
    std::string gameVersion = utils::regGet(L"Preferred Game Version", REG_DWORD);

    if(gameVersion == "0") ui->gameVersionCbx->setChecked(false);
    else if(gameVersion == "1") ui->gameVersionCbx->setChecked(true);
    else status("Failed to get Preferred Game Version.");

    setLaunchIcons();
}

void MainWindow::mountMod(QString modSize, QString fileCount)
{
    bool mounting = false;
    if(!shortcutMode)
    {
        ui->toggleMountBtn->setEnabled(false);
        ui->modList->setEnabled(false);
        ui->gameBtn->setEnabled(false);
        ui->editorBtn->setEnabled(false);
    }

    if(config->getSetting("Mounted") == "")
    {
        if(shortcutMode || modSelected())
        {
            std::string gamePath = config->getSetting("GamePath");
            int iSelectedMod = 0;
            QString selectedMod;
            if(shortcutMode) selectedMod = tmpModName;
            else
            {
                iSelectedMod = ui->modList->currentRow();
                selectedMod = ui->modList->item(iSelectedMod, 0)->text();
            }

            status("Mounting "+selectedMod+"...");

            if(QFileInfo(QString::fromStdString(gamePath)).isDir())
            {
                if(!shortcutMode)
                {
                    modSize = ui->modList->item(iSelectedMod, 1)->text();
                    fileCount = ui->modList->item(iSelectedMod, 2)->text();

                }
                config->setSetting("Mounted", selectedMod.toStdString());
                config->setSetting("MountedSize", modSize.toStdString());
                config->setSetting("MountedCount", fileCount.toStdString());
                config->saveConfig();

                Controller *c = new Controller(this, "Mounting", selectedMod);
                connect(c->worker, &Worker::resultReady, this, &MainWindow::mountModReady);
                emit c->mountMod();
                mounting = true;
            }
            else status("Invalid Warcraft III folder.", true);
        }
        else status("Select a mod to mount.");
    } else
    {
        status("Already mounted: "+QString::fromStdString(config->getSetting("Mounted")));
        if(shortcutMode) unmountMod();
    }

    if(!mounting && !shortcutMode) getMount(true);
}

void MainWindow::mountModReady(QString modName, int success, int failed, int missing, bool abort)
{
    bool scanMods = false,
         errors = true;
    if(success > 0)
    {
        if(abort || failed+missing > 0)
        {
            scanMods = true;
            config->setSetting("MountedError", result2errorMsg("Mounting", success, failed, missing, abort));
            config->saveConfig();
        }
        else errors = false;
    }
    else
    {
        config->deleteSetting("Mounted");
        config->deleteSetting("MountedSize");
        config->deleteSetting("MountedCount");
        config->deleteSetting("MountedError");
        config->saveConfig();
    }

    QString statusMsg = result2statusMsg(modName, "Mounting", success, failed, missing, abort);
    refresh(statusMsg, modName, scanMods);

    if(shortcutMode)
    {
        if(errors && QMessageBox::warning(splash, "Launch game?",
                        QString(abort ? "Mounting aborted: " : "Errors occurred while mounting: ")
                            +statusMsg
                            +"\nDo you want to continue launching the game?",
                        QMessageBox::Yes|QMessageBox::No)
           == QMessageBox::No)
        {
            emit shortcutQuit();
            return;
        }

        if(shortcutEditor) launchEditor();
        else launchGame();
    }
}

void MainWindow::unmountMod()
{
    if(!shortcutMode)
    {
        ui->toggleMountBtn->setEnabled(false);
        ui->gameBtn->setEnabled(false);
        ui->editorBtn->setEnabled(false);
    }

    if(config->getSetting("Mounted") != "")
    {
        status("Unmounting "+QString::fromStdString(config->getSetting("Mounted"))+"...");

        Controller *c = new Controller(this, "Unmounting", QString::fromStdString(config->getSetting("Mounted")));
        connect(c->worker, &Worker::resultReady, this, &MainWindow::unmountModReady);
        emit c->unmountMod();
    }
    else
    {
        status("No mod mounted.");

        if(shortcutMode) shortcutMountMod();
        else getMount(true);
    }
}

void MainWindow::unmountModReady(QString modName, int success, int failed, int missing, bool abort, bool force)
{
    bool unmounted = false;
    if(!force && (abort || failed+missing > 0))
    {
        std::string errorMsg = result2errorMsg("Unmounting", success, failed, missing, abort, force);
        if(config->getSetting("MountedError") != "") errorMsg = config->getSetting("MountedError")+";"+errorMsg;
        config->setSetting("MountedError", errorMsg);
    }
    else
    {
        unmounted = true;
        config->deleteSetting("Mounted");
        config->deleteSetting("MountedSize");
        config->deleteSetting("MountedCount");
        config->deleteSetting("MountedError");
    }

    config->saveConfig();

    QString statusMsg = result2statusMsg(modName, "Unmounting", success, failed, missing, abort, force);
    refresh(statusMsg, modName, unmounted);

    if(shortcutMode)
    {
        if(unmounted) shortcutMountMod();
        else if(QMessageBox::warning(splash, "Try again?",
                            "Unmounting "+QString(abort ? "failed: " : "aborted: ")
                                +statusMsg
                                +"\nDo you want to try again?",
                            QMessageBox::Yes|QMessageBox::No)
                == QMessageBox::Yes) unmountMod();
        else emit shortcutQuit();
    }
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
        ui->modList->setStyleSheet( QString("QTableWidget::item:selected { border: 2px dashed #bcbe00;")
                                   +"border-top-color: #f7f500; border-bottom-color: #f7f500;"
                                   +"color: #eee; background-color: #555; }");
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
    QString folder = QFileDialog::getExistingDirectory(this, "Add Mod", "",
                                                        QFileDialog::ShowDirsOnly | QFileDialog::HideNameFilterDetails);

    if(folder != "")
    {
        QMessageBox copyMove(this);
        copyMove.setWindowTitle("Copy or move?");
        copyMove.setText("Do you want to copy or move this folder?\n"+folder);
        copyMove.addButton("Move", QMessageBox::ActionRole); //btn index: 0 (== PROCFILE_MOVE)
        copyMove.addButton("Copy", QMessageBox::ActionRole); //btn index: 1 (== PROCFILE_COPY)
        copyMove.addButton(QMessageBox::Cancel);
        copyMove.setIcon(QMessageBox::Warning);

        int result = copyMove.exec();

        if(result == PROCFILE_MOVE || result == PROCFILE_COPY)
        {
            QString modName = QDir(folder).dirName(),
                    newFolder = QString::fromStdString(config->modPath)+"/"+modName;
            if(!QDir(newFolder).exists())
            {
                status("Adding "+modName+"...");

                Controller *c = new Controller(this, "Adding", modName);
                connect(c->worker, &Worker::resultReady, this, &MainWindow::addModReady);
                emit c->moveFolder(folder, newFolder, result);
            }
            else status("A mod with that name already exists.", true);
        }
    }
}

void MainWindow::addModReady(QString modName, int success, int failed, int missing, bool abort)
{
    refresh(result2statusMsg(modName, "Adding", success, failed, missing, abort), modName);
}

void MainWindow::openModFolder()
{
    QString path = QString::fromStdString(config->modPath);
    QString modName = "Mods";

    if(modSelected())
    {
        QString modName = ui->modList->item(ui->modList->currentRow(), 0)->text();
        path += "/"+modName;
    }

    status("Opening "+modName+" folder...");

    if(QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        status(modName+" folder opened.");
    else status("Failed to open "+modName+" folder.");
}

void MainWindow::renameModAction()
{
    if(modSelected()) renameModStart(ui->modList->item(ui->modList->currentRow(), 0));
}

void MainWindow::renameModStart(QTableWidgetItem *item)
{
    if(item->column() != 0) item = ui->modList->item(item->row(), 0);
    tmpModName = item->text();
    renameModItem = item;
    connect(ui->modList, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(renameModSave(QTableWidgetItem*)));
    ui->modList->editItem(item);
}

void MainWindow::renameModSave(QTableWidgetItem *item)
{
    disconnect(ui->modList, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(renameModSave(QTableWidgetItem*)));

    QString statusMsg = "";
    QString selectedMod = tmpModName;
    if(item == renameModItem)
    {
        QString qsModPath = QString::fromStdString(config->modPath);
        if(QFileInfo(qsModPath+"/"+tmpModName).isDir())
        {
            QString newModName = item->text();
            if(!QDir().rename(qsModPath+"/"+tmpModName, qsModPath+"/"+newModName))
                statusMsg = "Failed to rename mod.";
            else
            {
                selectedMod = newModName;
                statusMsg = tmpModName+" renamed to "+newModName+".";
            }
        }
        else statusMsg = "Mod not found.";
    }
    else statusMsg = "Saving wrong item.";

    tmpModName = "";
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
            status("Deleting "+qsModName+"...");

            Controller *c = new Controller(this, "Deleting", qsModName);
            connect(c->worker, &Worker::resultReady, this, &MainWindow::deleteModReady);
            emit c->deleteMod();
        }
    }
}

void MainWindow::deleteModReady(QString modName, int success, int failed, int missing, bool abort)
{
    refresh(result2statusMsg(modName, "Deleting", success, failed, missing, abort), modName);
}

bool MainWindow::modSelected()
{
    if(ui->modList->currentRow() < 0 || ui->modList->currentRow() >= ui->modList->rowCount())
    {
        status("No mod selected.");
        return false;
    }
    return true;
}

void MainWindow::status(QString msg, bool warning)
{
    if(splash) splash->statusBar()->showMessage(msg);
    else ui->statusBarLbl->setText(msg);
    if(warning) QMessageBox::warning(activeWindow(), "Error", msg);
}

QString MainWindow::result2statusMsg(QString modName, QString action,
                                         int success, int failed, int missing, bool abort, bool force)
{
    std::map<QString, QString> dict = action_dict(action);
    QString statusMsg,
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
        statusMsg += " ["+QString::number(success)+" files "+pAction;
        if(failed > 0) statusMsg += ", "+QString::number(failed)+" files failed";
        if(missing > 0) statusMsg += ", "+QString::number(missing)+" files missing";
        statusMsg += "]";
    }

    return statusMsg;
}

std::string MainWindow::result2errorMsg(std::string action,int success, int failed, int missing, bool abort, bool force)
{
    std::map<QString, QString> dict = action_dict(QString::fromStdString(action));
    std::string errorMsg,
           pAction = dict.find("pAction")->second.toStdString(),
           cAction = dict.find("cAction")->second.toStdString(),
           pAction_c = dict.find("pAction_c")->second.toStdString();
    int totErrors = failed+missing;

    if(abort) errorMsg = action+" aborted";
    else if(force) "Force "+cAction;
    else if(success > 0 && !(action == "Unmounting" && totErrors > 0))
        errorMsg = pAction_c;
    else if(totErrors > 0) errorMsg = action+" failed";
    else errorMsg = "No files to "+cAction;

    errorMsg += ": "+utils::i2s(success)+" files "+pAction;
    if(failed > 0) errorMsg += ", "+utils::i2s(failed)+" files failed";
    if(missing > 0) errorMsg += ", "+utils::i2s(missing)+" files missing";

    return errorMsg;
}

std::map<QString, QString> MainWindow::action_dict(QString action)
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
    delete config;
    if(splash) delete splash;
    if(settings) delete settings;
    if(about) delete about;
    if(renameModItem) delete renameModItem;
}

void MainWindow::shortcutLaunch(QString mod, QString gameVersion, bool editor)
{
    if(gameVersion == "0" || gameVersion == "1") setGameVersion(gameVersion);

    if(mod != "" && mod != nullptr)
    {
        setAllowFiles("1");
        if(mod.toStdString() != config->getSetting("Mounted"))
        {
            tmpModName = mod;
            shortcutEditor = editor;

            if(config->getSetting("Mounted") != "") unmountMod();
            else shortcutMountMod();

            return;
        }
    }
    else setAllowFiles("0");

    if(editor) launchEditor();
    else launchGame();
}

void MainWindow::shortcutMountMod()
{
    Controller *c = new Controller(this, "Scanning", tmpModName);
    connect(c->worker, &Worker::scanModDone, this, &MainWindow::mountMod);
    emit c->scanMod();
}

QMainWindow* MainWindow::activeWindow()
{
    if(splash) return splash;
    else return this;
}
