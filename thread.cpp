#include "thread.h"
#include "utils.h"
#include "ui_filestatus.h"

// FILESTATUS DIALOG
FileStatus::FileStatus(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileStatus)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowCloseButtonHint, false);
    ui->okBtn->hide();
    ui->forceBtn->hide();

    connect(ui->abortBtn, SIGNAL(clicked()), this, SLOT(abort()));
}

void FileStatus::setText(QString msg)
{
    ui->msgLbl->setText(msg);
}

void FileStatus::setInfoText(QString msg)
{
    ui->infoLbl->setText(msg);
}

void FileStatus::addErrorText(QString msg)
{
    if(ui->detailsTxt->height() == 0)
    {
        setMinimumHeight(minimumHeight()+90);
        ui->detailsTxt->setMinimumHeight(90);
        ui->detailsTxt->setMaximumHeight(QWIDGETSIZE_MAX);
    }
    ui->detailsTxt->moveCursor(QTextCursor::End);
    ui->detailsTxt->insertPlainText(msg+"\n");
}

void FileStatus::result(bool enableForce)
{
    setWindowFlag(Qt::WindowCloseButtonHint);

    disconnect(ui->abortBtn, SIGNAL(clicked()), this, SLOT(abort()));
    ui->abortBtn->hide();

    connect(ui->okBtn, SIGNAL(clicked()), this, SLOT(accept()));
    ui->okBtn->show();

    if(enableForce)
    {
        connect(ui->forceBtn, SIGNAL(clicked()), this, SLOT(forceUnmountClicked()));
        ui->forceBtn->show();
    }
    else
    {
        disconnect(ui->forceBtn, SIGNAL(clicked()), this, SLOT(forceUnmountClicked()));
        ui->forceBtn->hide();
    }
}

void FileStatus::abort()
{
    emit rejected();
}

void FileStatus::forceUnmountClicked()
{
    if(QMessageBox::warning(this, "Force unmount?", "Are you sure you want to remove all record of mounted files and backups?",
                            QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
        emit forceUnmount(true);
}

FileStatus::~FileStatus()
{
    delete ui;
}

// THREAD WORKER
Worker::Worker(Config *newConfig, QString newMod)
{
    config = newConfig;
    mod = newMod;
}

void Worker::scanModWorker(int row)
{
    double modSize = 0;
    int fileCount = 0;

    QDirIterator itMod(QString::fromStdString(config->modPath)+"/"+mod, QDir::NoDotAndDotDot|QDir::Files, QDirIterator::Subdirectories);
    if(itMod.hasNext())
        do
        {
            itMod.next();
            modSize += itMod.fileInfo().size();
            fileCount++;

            emit scanModUpdate(row,
                               QString("%0 MB").arg(round(modSize/1024/1024*100)/100),
                               QString("%0 files").arg(fileCount));
        } while(itMod.hasNext());
    else emit scanModUpdate(row, "0 MB", "0 files");
}

void Worker::moveFolderWorker(QString src, QString dst, bool savePaths, bool copy)
{
    int success = 0, failed = 0, missing = 0;

    QDirIterator itSrc(src, QDir::NoDotAndDotDot|QDir::Files, QDirIterator::Subdirectories);
    if(!abort && itSrc.hasNext())
    {
        std::ofstream out_files, backup_files;

        do
        {
            itSrc.next();
            QString relativePath = itSrc.filePath().remove(src+"/");

            emit status(relativePath);

            std::pair<int, std::string> result = moveFile(itSrc.filePath(), dst+"/"+relativePath, copy);
            if(result.first == 0)
            {
                success++;
                if(savePaths)
                {
                    if(!out_files.is_open()) out_files.open(config->outFilesPath.c_str());
                    out_files << relativePath.toStdString() << std::endl;
                }
            }
            else if(result.first == 2) missing++;
            else failed++;

            if(savePaths && result.second != "")
            {
                if(!backup_files.is_open()) backup_files.open(config->backupFilesPath.c_str());
                backup_files << result.second << std::endl;
            }
        } while(!abort && itSrc.hasNext());

        if(savePaths)
        {
            if(out_files.is_open()) out_files.close();
            if(backup_files.is_open()) backup_files.close();
        }
    }

    emit resultReady(mod, success, failed, missing, abort);
}

void Worker::deleteModWorker()
{
    int success = 0, failed = 0, missing = 0;
    QString deleteModPath = QString::fromStdString(config->modPath)+"/"+mod;

    QDirIterator itMod(deleteModPath, QDir::NoDotAndDotDot|QDir::AllEntries, QDirIterator::Subdirectories);
    while(!abort && itMod.hasNext())
    {
        itMod.next();
        QString filePath = itMod.filePath();
        QFileInfo fiFilePath = itMod.fileInfo();

        emit status(filePath);

        if(fiFilePath.exists())
        {
            missing++;
            emit status("Missing file: "+filePath+".", true);
        }
        else if(fiFilePath.isFile() && QFile(filePath).remove())
        {
            success++;
            removePath(fiFilePath.absolutePath(), QString::fromStdString(config->modPath));
        }
        else if(fiFilePath.isDir()) removePath(filePath, QString::fromStdString(config->modPath));
        else
        {
            failed++;
            emit status("Failed to delete file: "+filePath, true);
        }
    }

    QDir().rmdir(deleteModPath);

    emit resultReady(mod, success, failed, missing, abort);
}

void Worker::unmountModWorker(bool force)
{
    int success = 0, failed = 0, missing = 0;
    utils::TxtReader* txtReader = new utils::TxtReader(config->outFilesPath);

    if(!abort && txtReader->next())
    {
        std::string newOutFiles = "";
        do
        {
            if(txtReader->line != "")
            {
                QString qsLine = QString::fromStdString(txtReader->line);
                emit status(qsLine);

                int result = moveFile(QString::fromStdString(config->getSetting("MountedTo"))+"/"+qsLine,
                                      QString::fromStdString(config->modPath)+"/"+mod+"/"+qsLine).first;
                if(result == 0) success++;
                else
                {
                    if(!force) newOutFiles += txtReader->line+"\n";
                    if(result == 2) missing++;
                    else failed++;
                }
            }
        } while(!abort && txtReader->next());

        std::ofstream out_files(config->outFilesPath.c_str());
        out_files << newOutFiles;
        out_files.close();
    }

    txtReader = new utils::TxtReader(config->backupFilesPath);
    if(!abort && txtReader->next())
    {
        emit appendAction("restoring backups...");

        std::string newBackupFiles = "";
        do
        {
            if(txtReader->line != "")
            {
                QString qsLine = QString::fromStdString(txtReader->line);
                emit status(QFileInfo(qsLine).fileName());
                int result = moveFile(qsLine, qsLine.chopped(qsLine.lastIndexOf("."))).first;

                if(result == 0) success++;
                else
                {
                    if(!force) newBackupFiles += txtReader->line+"\n";
                    if(result == 2) missing++;
                    else failed++;
                }
            }
        } while(!abort && txtReader->next());

        std::ofstream backup_files(config->backupFilesPath.c_str());
        backup_files << newBackupFiles;
        backup_files.close();

        emit appendAction(" ");
    }

    emit resultReady(mod, success, failed, missing, abort, force);
}

std::pair<int, std::string> Worker::moveFile(QString src, QString dst, bool copy)
{
    std::string backupPath = "";
    int result = 1;
    QFileInfo fiSrc(src);

    if(fiSrc.isFile())
    {
        //if dst exists, make backup
        if(QFileInfo(dst).exists())
        {
            QString qsBackupPath = dst+".wmmbackup";
            QString newBackupPath = qsBackupPath;
            QFileInfo fiBackup(qsBackupPath);
            for(int i=2; fiBackup.exists(); i++)
            {
                newBackupPath = qsBackupPath+QString::number(i);
                fiBackup.setFile(newBackupPath);
            }

            if(QFile::rename(dst, newBackupPath)) backupPath = newBackupPath.toStdString();
            else
            {
                emit status("Failed to create backup of "+dst+"\n-->Skipping file: "+src, true);
                return { 1, "" };
            }
        }
        else QDir().mkpath(QFileInfo(dst).absolutePath());

        if(copy ? QFile::copy(src, dst) : QFile::rename(src, dst))
        {
            result = 0;
            if(!copy) removePath(fiSrc.absolutePath());
        }
    }
    else if(src != "" && !fiSrc.exists())
    {
        result = 2;
        emit status("Missing file: "+src, true);
    }

    if(result == 1) emit status("Failed to "+tr(copy ? "copy" : "move")+" file: "+src, true);

    return { result, backupPath };
}

void Worker::removePath(QString path, QString stopPath)
{
    QDir dirEmpty(path);
    dirEmpty.setFilter(QDir::NoDotAndDotDot|QDir::AllEntries);

    while(dirEmpty.count() == 0) {
        QString delPath = dirEmpty.absolutePath();
        dirEmpty.cdUp();

        if(   delPath != stopPath
           && delPath.toStdString() != config->modPath
           && delPath.toStdString() != config->getSetting("GamePath")
           && (stopPath != "" || dirEmpty.absolutePath().toStdString() != config->modPath))
        {
            if(!QDir().rmdir(delPath)) emit status("Failed to delete empty folder: "+delPath, true);
        }
        else break;
    }
}

Controller::Controller(MainWindow *newMw, QString newAction, QString newMod, bool newStatus)
{
    mw = newMw;
    action = newAction;
    mod = newMod;
    showStatus = newStatus;

    worker = new Worker(mw->config, mod);
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Controller::scanMod, worker, &Worker::scanModWorker);
    connect(this, &Controller::moveFolder, worker, &Worker::moveFolderWorker);
    connect(this, &Controller::deleteMod, worker, &Worker::deleteModWorker);
    connect(this, &Controller::unmountMod, worker, &Worker::unmountModWorker);

    if(showStatus)
    {
        connect(worker, &Worker::status, this, &Controller::status);
        connect(worker, &Worker::appendAction, this, &Controller::appendAction);
        connect(worker, &Worker::resultReady, this, &Controller::result);

        fileStatus = new FileStatus(mw);
        connect(fileStatus, SIGNAL(rejected()), this, SLOT(abort()));
        connect(fileStatus, SIGNAL(forceUnmount(bool)), worker, SLOT(unmountModWorker(bool)));
        fileStatus->setText(action+" "+mod+":");
        fileStatus->setWindowTitle(action+" "+mod+"...");
    }

    workerThread.start();
}

void Controller::status(QString msg, bool error)
{
    if(error) fileStatus->addErrorText(msg);
    else fileStatus->setInfoText(msg);

    if(!fileStatus->isVisible()) fileStatus->show();
}

void Controller::appendAction(QString msg)
{
    fileStatus->setText(action+" "+mod+": "+msg);
}

void Controller::result(QString, int success, int failed, int missing, bool abort, bool force)
{
    disconnect(fileStatus, SIGNAL(rejected()), this, SLOT(abort()));

    int totErrors = failed+missing;
    if(abort || totErrors > 0)
    {
        fileStatus->result(!force && !abort && action == "Unmounting" && success == 0 && totErrors > 0);
        if(!abort)
        {
            if(!force && totErrors > 0 && (action == "Unmounting" || success <= 0))
                appendAction("failed.");
            else appendAction("done.");
        }

        QString msg = QString("%0 files succeeded").arg(success);
        if(failed > 0) msg += QString(", %0 files failed").arg(failed);
        if(missing > 0) msg += QString(", %0 files missing").arg(missing);
        msg += ".";

        status(msg);
    }
    else this->~Controller();
}

void Controller::abort()
{
    worker->abort = true;
    appendAction("aborted.");
}

Controller::~Controller()
{
    if(fileStatus->isVisible()) fileStatus->close();
    workerThread.quit();
    workerThread.wait();
}
