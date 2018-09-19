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

    refresh("Setting up UI...");

    Ui::About uiAbout;
    uiAbout.setupUi(about);
    about->setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    //Menubar
    connect(ui->actionSettings, SIGNAL(triggered()), this, SLOT(openSettings()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about->show()));

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

void MainWindow::openSettings()
{
    settings->loadSettings();
    if(settings->exec() == 1) refresh("Settings saved.");
}

void MainWindow::refresh(string statusMsg, QString selectedMod)
{
    setEnabled(false);
    getAllowFiles();
    getGameVersion();

    status("Scanning for mods...");

    int selectedRow = -1;
    ui->modList->clearContents();
    ui->modList->setRowCount(0);
    QDir dir(QString::fromStdString(config->modPath));
    dir.setFilter(QDir::NoDotAndDotDot|QDir::Dirs|QDir::NoSymLinks);
    QDirIterator it(dir);
    while (it.hasNext())
    {
        it.next();
        QString qsModSize, qsFileCount;

        status("Scanning "+it.fileName().toStdString()+"...");
        if(it.fileName().toStdString() == config->getSetting("Mounted"))
        {
            qsModSize = QString::fromStdString(config->getSetting("MountedSize"));
            qsFileCount = QString::fromStdString(config->getSetting("MountedCount"));
        }
        else
        {
            double modSize = 0;
            int fileCount = 0;
            QDir dir2(it.filePath());
            dir2.setFilter(QDir::NoDotAndDotDot|QDir::Files|QDir::NoSymLinks);
            QDirIterator it2(dir2, QDirIterator::Subdirectories);
            while (it2.hasNext())
            {
                it2.next();
                modSize += it2.fileInfo().size();
                fileCount++;
            }

            qsModSize = QString("%0 MB").arg(round(modSize/1024/1024*100)/100);
            qsFileCount = QString("%0 files").arg(fileCount);
        }

        if(qsFileCount != "0 files" || config->getSetting("hideEmptyMods") != "1")
        {
            int row = ui->modList->rowCount();
            ui->modList->insertRow(row);
            ui->modList->setItem(row, 0, new QTableWidgetItem(it.fileName()));
            ui->modList->setItem(row, 1, new QTableWidgetItem(qsModSize));
            ui->modList->setItem(row, 2, new QTableWidgetItem(qsFileCount));

            if(selectedMod == dir.dirName()) selectedRow = row;
        }
    }

    getMount();
    if(statusMsg == "") status("WC3 Mod Manager refreshed.");
    else status(statusMsg);
    setEnabled(true);
    if(selectedRow >= 0 && selectedRow < ui->modList->rowCount())
    {
        ui->modList->selectRow(selectedRow);
        ui->modList->setFocus();
    }
}

void MainWindow::launchGame()
{
    status("Launching game...");
    if(QDesktopServices::openUrl(QUrl::fromLocalFile(
            QString::fromStdString(config->getSetting("GamePath"))+"/Warcraft III.exe")))
        status("Game launched.");
    else status("Failed to launch Warcraft III.exe", true);
}

void MainWindow::launchEditor()
{
    status("Launching editor...");
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
                connect(c->worker, &Worker::moveFolderReady, this, &MainWindow::mountModReady);
                emit c->moveFolder(QString::fromStdString(config->modPath)+"/"+selectedMod, QString::fromStdString(gamePath), true);
                mounting = true;
            }
            else status("Invalid Warcraft III folder.", true);
        }
        else status("Select a mod to mount.");
    } else status("Already mounted: "+config->getSetting("Mounted"));

    if(!mounting) getMount(true);
}

void MainWindow::mountModReady(int success, int failed, int missing)
{
    int totErrors = failed+missing;
    if(success > 0) status(config->getSetting("Mounted")+" mounted."
                          +(totErrors > 0 ? " ("+utils->int2string(totErrors)+" errors)" : ""));
    else
    {
        if(totErrors > 0) status("Failed to mount "+config->getSetting("Mounted")+". ("+utils->int2string(totErrors)+" errors)");
        else status("No files to mount.");

        config->deleteSetting("Mounted");
        config->deleteSetting("MountedTo");
        config->deleteSetting("MountedSize");
        config->deleteSetting("MountedCount");
        config->saveConfig();
    }

    getMount(true);
}

void MainWindow::unmountMod()
{
    ui->toggleMountBtn->setEnabled(false);
    ui->gameBtn->setEnabled(false);
    ui->editorBtn->setEnabled(false);

    string mounted = config->getSetting("Mounted");
    if(mounted != "")
    {
        status("Unmounting "+mounted+"...");

        Controller *c = new Controller(this, "Unmounting", QString::fromStdString(mounted));
        connect(c->worker, &Worker::unmountModReady, this, &MainWindow::unmountModReady);
        emit c->unmountMod(QString::fromStdString(mounted));
    }
    else
    {
        status("No mod mounted.");
        getMount(true);
    }
}

void MainWindow::unmountModReady(int success, int failed, int missing)
{
    int totErrors = failed+missing;
    string statusMsg;

    if(success == 0 && totErrors == 0) statusMsg = "No files to unmount.";
    else
    {
        statusMsg = config->getSetting("Mounted")+" unmounted.";
        if(totErrors > 0) statusMsg += " ("+utils->int2string(totErrors)+" errors)";
    }
    status(statusMsg);

    config->deleteSetting("Mounted");
    config->deleteSetting("MountedTo");
    config->deleteSetting("MountedSize");
    config->deleteSetting("MountedCount");
    config->saveConfig();

    getMount(true);
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

        bool modFound = false;
        for(int i=0; i<ui->modList->rowCount() && !modFound; i++)
            if(ui->modList->item(i, 0)->text() == QString::fromStdString(config->getSetting("Mounted"))) {
                modFound = true;
                ui->modList->selectRow(i);
                ui->modList->scrollToItem(ui->modList->item(i, 0));
            }

        if(!modFound) {
            if(QDir().mkdir(QString::fromStdString(config->modPath+"/"+config->getSetting("Mounted"))))
                refresh();
            else status("Failed to create mod folder.");
        }

        connect(ui->toggleMountBtn, SIGNAL(clicked()), this, SLOT(unmountMod()));
    }

    ui->toggleMountBtn->setEnabled(true);
    ui->gameBtn->setEnabled(true);
    ui->editorBtn->setEnabled(true);
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
            QDir dir(qsFolder);
            QString modName = dir.dirName(),
                    qsNewFolder = QString::fromStdString(config->modPath)+"/"+modName;
            if(!QDir(qsNewFolder).exists())
            {
                status("Adding "+modName.toStdString()+"...");

                addModName = modName;
                Controller *c = new Controller(this, "Adding", modName);
                connect(c->worker, &Worker::moveFolderReady, this, &MainWindow::addModReady);
                emit c->moveFolder(qsFolder, qsNewFolder, false, result == 0);
            }
            else status("A mod with that name already exists.", true);
        }
    }
}

void MainWindow::addModReady(int success, int failed, int missing)
{
    int totErrors = failed+missing;
    string sModName = addModName.toStdString();

    if(success == 0)
    {
        if(totErrors == 0) status("No files to add.");
        else status("Failed to add "+sModName+". ("+utils->int2string(totErrors)+"errors)");
    }
    else
    {
        string statusMsg = sModName+" added.";
        if(totErrors > 0) statusMsg += " ("+utils->int2string(totErrors)+" errors)";

        refresh(statusMsg, addModName);
    }
}

void MainWindow::openModFolder()
{
    if(modSelected())
        QDesktopServices::openUrl(QUrl::fromLocalFile(
            QString::fromStdString(config->modPath)+"/"+ui->modList->item(ui->modList->currentRow(), 0)->text()));
}

void MainWindow::renameModAction()
{
    if(modSelected()) renameModStart(ui->modList->item(ui->modList->currentRow(), 0));
}

void MainWindow::renameModStart(QTableWidgetItem* item)
{
    if(item->column() == 0)
    {
      renameModName = item->text();
      renameModItem = item;
      connect(ui->modList, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(renameModSave(QTableWidgetItem*)));
      ui->modList->editItem(item);
    }
}

void MainWindow::renameModSave(QTableWidgetItem* item)
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
                "Are you sure you want to permanently delete "+qsModName+" ("
               +ui->modList->item(ui->modList->currentRow(), 1)->text()+" / "
               +ui->modList->item(ui->modList->currentRow(), 2)->text()+")?",
                QMessageBox::Yes|QMessageBox::Cancel) == QMessageBox::Yes)
        {
            string sModName = qsModName.toStdString();
            status("Deleting "+sModName+"...");

            Controller *c = new Controller(this, "Deleting", qsModName);
            connect(c->worker, &Worker::deleteFolderReady, this, &MainWindow::deleteModReady);
            emit c->deleteFolder(QString::fromStdString(config->modPath)+"/"+qsModName);

            QDir dir(QString::fromStdString(config->modPath)+"/"+qsModName);
            if(dir.removeRecursively()) refresh(sModName+" deleted.");
            else refresh("Failed to delete "+sModName+".", qsModName);
        }
    }
}

void MainWindow::deleteModReady(int success, int failed, int missing)
{
    int totErrors = failed+missing;
    string sModName = addModName.toStdString();

    if(success == 0)
    {
        if(totErrors == 0) status("No files to delete.");
        else status("Failed to add "+sModName+". ("+utils->int2string(totErrors)+"errors)");
    }
    else
    {
        string statusMsg = sModName+" added.";
        if(totErrors > 0) statusMsg += " ("+utils->int2string(totErrors)+" errors)";

        refresh(statusMsg, addModName);
    }
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

MainWindow::~MainWindow()
{
    delete ui;
}
