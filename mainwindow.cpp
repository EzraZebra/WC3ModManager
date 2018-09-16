#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <sstream>
#include <QAction>

using namespace std;

MainWindow:: MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    refresh("Setting up UI...");

    //Menubar
    connect(ui->actionSettings, SIGNAL(triggered()), this, SLOT(openSettings()));

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
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openExplorer()));
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
    dir.setFilter(QDir::NoDotAndDotDot|QDir::Dirs);
    QDirIterator it(dir);
    while (it.hasNext())
    {
        it.next();
        QString qsModSize, qsFileCount;

        status("Scanning "+it.fileName().toStdString()+"...");
        if(it.fileName().toStdString() == config->loadSetting("Mounted"))
        {
            qsModSize = QString::fromStdString(config->loadSetting("MountedSize"));
            qsFileCount = QString::fromStdString(config->loadSetting("MountedCount"));
        }
        else
        {
            utils->error("test",it.fileName().toStdString());
            double modSize = 0;
            int fileCount = 0;
            QDir dir2(it.filePath());
            dir2.setFilter(QDir::NoDotAndDotDot|QDir::Files);
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

        if(qsFileCount != "0 files" || config->loadSetting("hideEmptyMods") != "1")
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
            QString::fromStdString(config->loadSetting("GamePath"))+"/Warcraft III.exe")))
        status("Game launched.");
    else status("Failed to launch Warcraft III.exe", true);
}

void MainWindow::launchEditor()
{
    status("Launching editor...");
    if(QDesktopServices::openUrl(QUrl::fromLocalFile(
            QString::fromStdString(config->loadSetting("GamePath"))+"/World Editor.exe")))
        status("Editor launched.");
    else status("Failed to launch World Editor.exe", true);
}

void MainWindow::setLaunchIcons()
{
    if(ui->allowFilesCbx->isChecked() && config->loadSetting("Mounted") != "")
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
    ui->toggleMountBtn->setEnabled(false);

    if(config->loadSetting("Mounted") == "")
    {
        int iSelectedMod = ui->modList->currentRow();
        if(iSelectedMod >= 0 && iSelectedMod < ui->modList->rowCount())
        {
            string  gamePath = config->loadSetting("GamePath"),
                    selectedMod = ui->modList->item(iSelectedMod, 0)->text().toStdString();

            status("Mounting "+selectedMod+"...");

            QFileInfo fiGP(QString::fromStdString(gamePath));
            if(fiGP.isDir())
            {
                config->saveSetting("Mounted", selectedMod);
                config->saveSetting("MountedTo", gamePath);
                config->saveSetting("MountedSize", ui->modList->item(iSelectedMod, 1)->text().toStdString());
                config->saveSetting("MountedCount", ui->modList->item(iSelectedMod, 2)->text().toStdString());
                config->saveConfig();

                string selectedModPath = config->modPath+"/"+selectedMod;
                ofstream out_files(config->outFilesPath.c_str());
                ofstream backup_files(config->backupFilesPath.c_str());
                QDir dir(QString::fromStdString(selectedModPath));
                dir.setFilter(QDir::NoDotAndDotDot|QDir::Files);
                QDirIterator it(dir, QDirIterator::Subdirectories);
                bool filesFound = false;
                while(it.hasNext())
                {
                    it.next();
                    string relativePath = it.filePath().toStdString().substr(selectedModPath.size()+1);
                    QString newPath = QString::fromStdString(gamePath+"/"+relativePath);

                    status("Mounting "+selectedMod+": "+relativePath+"...");
                    out_files << relativePath << endl;
                    backup_files << moveFile(it.filePath(), newPath) << endl;
                    filesFound = true;
                }
                out_files.close();
                backup_files.close();

                if(filesFound) status(selectedMod+" mounted.");
                else
                {
                    config->deleteSetting("Mounted");
                    config->deleteSetting("MountedTo");
                    config->deleteSetting("MountedSize");
                    config->deleteSetting("MountedCount");
                    config->saveConfig();
                    status("Nothing to mount.");
                }
            }
            else status("Invalid Warcraft III folder.", true);
        }
        else status("Select a mod to mount.");
    } else status("Already mounted: "+config->loadSetting("Mounted"));

    getMount();
}

void MainWindow::unmountMod()
{
    ui->toggleMountBtn->setEnabled(false);

    string mounted = config->loadSetting("Mounted");
    if(mounted != "")
    {
        status("Unmounting "+mounted+"...");
        if(utils->txtReaderStart(config->outFilesPath))
        {
            while(utils->txtReaderNext())
                moveFile(QString::fromStdString(config->loadSetting("MountedTo")+"/"+utils->txtReaderLine),
                         QString::fromStdString(config->modPath+"/"+mounted+"/"+utils->txtReaderLine));

            ofstream out_files(config->outFilesPath.c_str());
            out_files << "";
            out_files.close();
        }
        else status("No files to unmount.");

        status("Unmounting "+mounted+": restoring backups...");
        if(utils->txtReaderStart(config->backupFilesPath))
        {
            while(utils->txtReaderNext())
                moveFile(QString::fromStdString(utils->txtReaderLine),
                         QString::fromStdString(utils->txtReaderLine.substr(0, utils->txtReaderLine.find_last_of("."))));

            ofstream backup_files(config->backupFilesPath.c_str());
            backup_files << "";
            backup_files.close();
        }

        config->deleteSetting("Mounted");
        config->deleteSetting("MountedTo");
        config->deleteSetting("MountedSize");
        config->deleteSetting("MountedCount");
        config->saveConfig();

        status(mounted+" unmounted.");
    }
    else status("No mod mounted.");

    getMount(true);
}

void MainWindow::getMount(bool setFocus)
{
    if(config->loadSetting("Mounted") == "")
    {
        disconnect(ui->toggleMountBtn, SIGNAL(clicked()), this, SLOT(unmountMod()));
        ui->toggleMountBtn->setText("&Mount");
        ui->modList->setStyleSheet("");
        ui->modList->setEnabled(true);
        if(setFocus) ui->modList->setFocus();
        connect(ui->toggleMountBtn, SIGNAL(clicked()), this, SLOT(mountMod()));
    }
    else
    {
        disconnect(ui->toggleMountBtn, SIGNAL(clicked()), this, SLOT(mountMod()));
        ui->toggleMountBtn->setText("Un&mount");
        ui->modList->setStyleSheet(tr("QTableWidget::item:selected { border: 2px dashed #bcbe00;")
                                  +tr("border-top-color: #f7f500; border-bottom-color: #f7f500;")
                                  +tr("color: #eee; background-color: #555; }"));
        ui->modList->setEnabled(false);

        bool modFound = false;
        string mount = config->loadSetting("Mounted");
        for(int i=0; i<ui->modList->rowCount() && !modFound; i++)
            if(ui->modList->item(i, 0)->text() == QString::fromStdString(mount)) {
                modFound = true;
                ui->modList->selectRow(i);
                ui->modList->scrollToItem(ui->modList->item(i, 0));
            }
        if(!modFound) {
            string newModFolderStr = config->modPath+"/"+mount;
            wstring newModFolderWStr(newModFolderStr.begin(), newModFolderStr.end());
            if(CreateDirectory(newModFolderWStr.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError())
                refresh();
            else status("Failed to create mod folder.");
        }
        connect(ui->toggleMountBtn, SIGNAL(clicked()), this, SLOT(unmountMod()));
    }
    ui->toggleMountBtn->setEnabled(true);
    setLaunchIcons();
}

void MainWindow::addMod(QString dlgStart)
{
    QString qsFolder = QFileDialog::getExistingDirectory(this, tr("Add Mod"), dlgStart,
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

        if(result == 0)
        {
            //copy files
        }
        else if(result == 1)
        {
            QDir dir(qsFolder);
            QString modName = dir.dirName(),
                    qsNewFolder = QString::fromStdString(config->modPath)+"/"+modName;
            if(!QDir(qsNewFolder).exists())
            {
                dir.setFilter(QDir::NoDotAndDotDot|QDir::AllEntries);
                QDirIterator it(dir, QDirIterator::Subdirectories);
                while(it.hasNext())
                {
                    it.next();
                    moveFile(it.filePath(), qsNewFolder+it.filePath().remove(qsFolder));
                }
                refresh(modName.toStdString()+" added.", modName);
            }
            else status("A mod with that foldername already exists.", true);
        }
    }
}

void MainWindow::openExplorer()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(
        QString::fromStdString(config->modPath)+"/"+ui->modList->item(ui->modList->currentRow(), 0)->text()));
}

void MainWindow::renameModAction()
{
    renameModStart(ui->modList->item(ui->modList->currentRow(), 0));
}

void MainWindow::renameModStart(QTableWidgetItem* item)
{
    if(item->column() == 0)
    {
      modName = item->text();
      modItem = item;
      connect(ui->modList, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(renameModSave(QTableWidgetItem*)));
      ui->modList->editItem(item);
    }
}

void MainWindow::renameModSave(QTableWidgetItem* item)
{
    disconnect(ui->modList, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(renameModSave(QTableWidgetItem*)));

    string statusMsg = "";
    QString selectedMod = modName;
    if(item == modItem)
    {
        QString qsModPath = QString::fromStdString(config->modPath);
        if(QFileInfo(qsModPath+"/"+modName).isDir())
        {
            QString newModName = item->text();
            if(!QDir().rename(qsModPath+"/"+modName, qsModPath+"/"+newModName))
                statusMsg = "Folder name in use or invalid.";
            else
            {
                selectedMod = newModName;
                statusMsg = modName.toStdString()+" renamed to "+newModName.toStdString()+".";
            }
        }
        else statusMsg = "Mod not found.";
    }
    else statusMsg = "Saving wrong item.";

    modName = "";
    modItem = nullptr;
    refresh(statusMsg, selectedMod);
}

void MainWindow::deleteMod()
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
        QDir dir(QString::fromStdString(config->modPath)+"/"+qsModName);
        if(dir.removeRecursively()) refresh(sModName+" deleted.");
        else refresh("Failed to delete "+sModName+".", qsModName);
    }
}

string MainWindow::moveFile(QString src, QString dst)
{
    QFileInfo fi(src);
    string returnPath = "";

    if(fi.isFile())
    {
        QFileInfo new_fi(dst);
        if(new_fi.exists())
        {
            QString backupPath = dst+".w3mmbackup",
                    newBackupPath = backupPath;
            QFileInfo backup_fi(backupPath);
            for(int i=2; backup_fi.exists(); i++)
            {
                newBackupPath = backupPath+QString::number(i);
                backup_fi.setFile(newBackupPath);
            }
            returnPath = newBackupPath.toStdString();
            QFile().rename(dst, newBackupPath);
        }

        string dstFolder = dst.toStdString();
        dstFolder = dstFolder.substr(0,dstFolder.find_last_of("/\\"));
        QDir().mkpath(QString::fromStdString(dstFolder));

        if(QFile().rename(src, dst))
        {
            string srcFolder = src.toStdString();
            srcFolder = srcFolder.substr(0,srcFolder.find_last_of("/\\"));
            QDir dir(QString::fromStdString(srcFolder));
            dir.setFilter(QDir::NoDotAndDotDot|QDir::AllEntries);

            while(dir.count() == 0) {
                QString delPath = dir.absolutePath();
                dir.cdUp();
                string sDelPath = delPath.toStdString(),
                       delPathParent = sDelPath.substr(0,sDelPath.find_last_of("/\\"));
                if(sDelPath != config->modPath && delPathParent != config->modPath && sDelPath != config->loadSetting("GamePath"))
                    QDir().rmdir(delPath);
            }
        }
        else status("Failed to move file: "+src.toStdString(), true);
    }
    else if(src != "" && !fi.exists()) status("Missing file: "+src.toStdString(), true);

    return returnPath;
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
