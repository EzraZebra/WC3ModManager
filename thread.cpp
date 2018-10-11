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

    ui->detailsTxt->insertPlainText("-----------------\n\n");
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

            emit scanModUpdate(QString("%0 MB").arg(round(modSize/1024/1024*100)/100),
                               QString("%0 files").arg(fileCount),
                               row);
        } while(itMod.hasNext());
    else emit scanModUpdate("0 MB", "0 files", row);

    emit scanModDone(QString("%0 MB").arg(round(modSize/1024/1024*100)/100),
                     QString("%0 files").arg(fileCount));
}

void Worker::mountModWorker()
{
    std::array<int, 3> result = mountModIterator(QString::fromStdString(config->modPath)+"/"+mod,
                                                 QString::fromStdString(config->getSetting("GamePath")));

    if(out_files.is_open()) out_files.close();
    if(backup_files.is_open()) backup_files.close();

    emit resultReady(mod, result[PROCFILE_SUCCESS], result[PROCFILE_FAILED],
                          result[PROCFILE_MISSING], abort);
}

std::array<int, 3> Worker::mountModIterator(QString dir, QString rootDst)
{
    std::array<int, 3> result{ 0, 0, 0 };

    QDirIterator itDir(dir, QDir::NoDotAndDotDot|QDir::AllEntries);
    if(!abort && itDir.hasNext())
    {
        QString rootSrc = QString::fromStdString(config->modPath)+"/"+mod;

        do
        {
            itDir.next();
            QString src = itDir.filePath(),
                    relativePath = src;
            relativePath.remove(rootSrc+"/");
            QString dst = rootDst+"/"+relativePath;
            QFileInfo fiDst = QFileInfo(dst);

            emit status(relativePath);

            if(!fiDst.isSymLink() && fiDst.exists() && fiDst.isDir())
            {
                std::array<int, 3> resultIt = mountModIterator(src, rootDst);
                result[PROCFILE_SUCCESS] += resultIt[PROCFILE_SUCCESS];
                result[PROCFILE_FAILED] += resultIt[PROCFILE_FAILED];
                result[PROCFILE_MISSING] += resultIt[PROCFILE_MISSING];
            }
            else
            {
                std::pair<int, std::string> resultIt = moveFile(src, dst, PROCFILE_LINK);

                result[unsigned(resultIt.first)]++;

                if(resultIt.first == PROCFILE_SUCCESS)
                {
                    if(!out_files.is_open()) out_files.open(config->outFilesPath.c_str());
                    out_files << dst.toStdString() << std::endl;
                }

                if(resultIt.second != "")
                {
                    if(!backup_files.is_open()) backup_files.open(config->backupFilesPath.c_str());
                    backup_files << resultIt.second << std::endl;
                }
            }
        } while(!abort && itDir.hasNext());
    }

    return result;
}

void Worker::unmountModWorker(bool force)
{
    std::array<int, 3> result{ 0, 0, 0 };
    utils::TxtReader* txtReader = new utils::TxtReader(config->outFilesPath);

    if(!abort && txtReader->next())
    {
        std::string newOutFiles = "";
        do
        {
            if(txtReader->line != "")
            {
                QString qsLine = QString::fromStdString(txtReader->line);
                QFileInfo fiLine(qsLine);
                emit status(qsLine);

                int resultIt = PROCFILE_FAILED;
                if(fiLine.isSymLink()) resultIt = deleteFile(qsLine);
                else if(!fiLine.exists())
                {
                    resultIt = PROCFILE_MISSING;
                    emit status("Missing file: "+qsLine, true);
                }
                else emit status("Not a symbolic link: "+qsLine, true);

                result[unsigned(resultIt)]++;
                if(!force && resultIt != PROCFILE_SUCCESS)
                    newOutFiles += txtReader->line+"\n";
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
                QString qsLine = QString::fromStdString(txtReader->line),
                        dst = qsLine;
                dst.truncate(qsLine.lastIndexOf(PROCFILE_BACKUP_EXT));

                emit status(QFileInfo(qsLine).fileName());

                int resultIt = moveFile(qsLine, dst).first;

                result[unsigned(resultIt)]++;
                if(!force && resultIt != PROCFILE_SUCCESS)
                    newBackupFiles += txtReader->line+"\n";
            }
        } while(!abort && txtReader->next());

        std::ofstream backup_files(config->backupFilesPath.c_str());
        backup_files << newBackupFiles;
        backup_files.close();

        emit appendAction("");
    }

    emit resultReady(mod, result[PROCFILE_SUCCESS], result[PROCFILE_FAILED], result[PROCFILE_MISSING], abort, force);
}

void Worker::moveFolderWorker(QString src, QString dst, int mode)
{
    std::array<int, 3> result{ 0, 0, 0 };

    QDirIterator itSrc(src, QDir::NoDotAndDotDot|QDir::Files, QDirIterator::Subdirectories);
    while(!abort && itSrc.hasNext())
    {
        itSrc.next();
        QString relativePath = itSrc.filePath().remove(src+"/"),
                itDstPath = dst+"/"+relativePath;

        emit status(relativePath);

        std::pair<int, std::string> resultIt = moveFile(itSrc.filePath(), itDstPath, mode);

        result[unsigned(resultIt.first)]++;
    }

    emit resultReady(mod, result[PROCFILE_SUCCESS], result[PROCFILE_FAILED], result[PROCFILE_MISSING], abort);
}

std::pair<int, std::string> Worker::moveFile(QString src, QString dst, int mode)
{
    std::string backupPath = "";
    int result = PROCFILE_FAILED;
    QFileInfo fiSrc(src);

    if(fiSrc.isSymLink() || fiSrc.isFile() || (mode == PROCFILE_LINK && fiSrc.isDir()))
    {
        QFileInfo fiDst(dst);
        //if dst exists, make backup
        if(fiDst.isSymLink() || fiDst.exists())
        {
            QString qsBackupPath = dst+PROCFILE_BACKUP_EXT;
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
                return { PROCFILE_FAILED, "" };
            }
        }
        else QDir().mkpath(fiDst.absolutePath());

        switch(mode)
        {
            case PROCFILE_MOVE:
                if(QFile::rename(src, dst))
                {
                    result = PROCFILE_SUCCESS;
                    removePath(fiSrc.absolutePath());
                }
                break;

            case PROCFILE_COPY:
                if(QFile::copy(src, dst)) result = PROCFILE_SUCCESS;
                break;

            case PROCFILE_LINK:
                if(CreateSymbolicLinkA(QDir::toNativeSeparators(dst).toStdString().c_str(),
                                       QDir::toNativeSeparators(src).toStdString().c_str(),
                                       QFileInfo(src).isDir() ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0x0))
                    result = PROCFILE_SUCCESS;
        }
    }
    else if(src != "" && !fiSrc.exists())
    {
        result = PROCFILE_MISSING;
        emit status("Missing file: "+src, true);
    }

    if(result == PROCFILE_FAILED)
    {
        QString modeStr = "process";
        switch(mode)
        {
            case PROCFILE_MOVE: modeStr = "move";
                break;
            case PROCFILE_COPY: modeStr = "copy";
                break;
            case PROCFILE_LINK: modeStr = "create symbolic link to";
        }
        emit status("Failed to "+modeStr+" file: "+src, true);
    }

    return { result, backupPath };
}

void Worker::deleteModWorker()
{
    std::array<int, 3> result{ 0, 0, 0 };
    QString deleteModPath = QString::fromStdString(config->modPath)+"/"+mod;

    QDirIterator itMod(deleteModPath, QDir::NoDotAndDotDot|QDir::AllEntries, QDirIterator::Subdirectories);
    while(!abort && itMod.hasNext())
    {
        itMod.next();
        QString filePath = itMod.filePath();

        emit status(filePath);

        if(!itMod.fileInfo().isSymLink() && itMod.fileInfo().isDir()) removePath(filePath, QString::fromStdString(config->modPath));
        else result[unsigned(deleteFile(filePath))]++;
    }

    QDir().rmdir(deleteModPath);

    emit resultReady(mod, result[PROCFILE_SUCCESS], result[PROCFILE_FAILED], result[PROCFILE_MISSING], abort);
}

int Worker::deleteFile(QString filePath)
{
    QFileInfo fiFilePath(filePath);
    if(!fiFilePath.isSymLink() && !fiFilePath.exists())
    {
        emit status("Missing file: "+filePath, true);
        return PROCFILE_MISSING;
    }
    else if(   (fiFilePath.isFile() && QFile(filePath).remove())
            || (fiFilePath.isSymLink() && fiFilePath.isDir() && QDir().rmdir(filePath)))
    {
        removePath(fiFilePath.absolutePath(), QString::fromStdString(config->modPath));
        return PROCFILE_SUCCESS;
    }
    else
    {
        emit status("Failed to delete file: "+filePath, true);
        return PROCFILE_FAILED;
    }
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

Controller::Controller(MainWindow *mw, QString newAction, QString newMod, bool showStatus)
{
    action = newAction;
    mod = newMod;

    worker = new Worker(mw->config, mod);
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Controller::scanMod, worker, &Worker::scanModWorker);
    connect(this, &Controller::mountMod, worker, &Worker::mountModWorker);
    connect(this, &Controller::unmountMod, worker, &Worker::unmountModWorker);
    connect(this, &Controller::deleteMod, worker, &Worker::deleteModWorker);
    connect(this, &Controller::moveFolder, worker, &Worker::moveFolderWorker);

    if(showStatus)
    {
        connect(worker, &Worker::status, this, &Controller::status);
        connect(worker, &Worker::appendAction, this, &Controller::appendAction);
        connect(worker, &Worker::resultReady, this, &Controller::result);

        fileStatus = new FileStatus(mw->activeWindow());
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
    else deleteLater();
}

void Controller::abort()
{
    worker->abort = true;
    appendAction("aborted.");
}

Controller::~Controller()
{
    workerThread.quit();
    workerThread.wait();
    if(fileStatus)
    {
        if(fileStatus->isVisible()) fileStatus->close();
        if(fileStatus) delete fileStatus;
    }
}
