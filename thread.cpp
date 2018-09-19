#include "thread.h"
#include <iostream>
#include <QDir>
#include <QDirIterator>

Worker::Worker(Config *newConfig)
{
    config = newConfig;
    utils = config->utils;
}

void Worker::moveFolderWorker(QString src, QString dst, bool savePaths, bool copy)
{
    QDir dirMod(src);
    dirMod.setFilter(QDir::NoDotAndDotDot|QDir::Files|QDir::NoSymLinks);
    QDirIterator itMod(dirMod, QDirIterator::Subdirectories);

    int success = 0, failed = 0, missing = 0;

    if(itMod.hasNext())
    {
        std::ofstream out_files, backup_files;

        do
        {
            itMod.next();
            QString relativePath = itMod.filePath().remove(src+"/");

            emit status(relativePath);

            std::pair<int, std::string> result = moveFile2(itMod.filePath(), dst+"/"+relativePath, copy);
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
        } while(itMod.hasNext());

        if(savePaths)
        {
            if(out_files.is_open()) out_files.close();
            if(backup_files.is_open()) backup_files.close();
        }
    }

    emit moveFolderReady(success, failed, missing);
}

void Worker::deleteFolderWorker(QString folder)
{
    QDir dirMod(folder);
    dirMod.setFilter(QDir::NoDotAndDotDot|QDir::AllEntries);
    QDirIterator itMod(dirMod, QDirIterator::Subdirectories);

    int success = 0, failed = 0, missing = 0;
    bool isFailed;

    while(itMod.hasNext())
    {
        itMod.next();

        emit status(itMod.filePath());

        isFailed = false;
        QFileInfo fiMod(itMod.filePath());

        if(fiMod.isFile())
        {
            if(QFile(itMod.filePath()).remove())  success++;
            else isFailed = true;
        }
        else if(!fiMod.exists())
        {
            missing++;
            emit status("Missing file: "+itMod.filePath()+".", true);
        }
        else isFailed = true;

        if(isFailed)
        {
            failed++;
            emit status("Failed to delete file: "+itMod.filePath(), true);
        }

    }

    QDir().rmdir(folder);

    emit deleteFolderReady(success, failed, missing);
}

void Worker::unmountModWorker(QString mod)
{
    int success = 0, failed = 0, missing = 0;

    if(utils->txtReaderStart(config->outFilesPath))
    {
        while(utils->txtReaderNext())
        {
            if(utils->txtReaderLine != "")
            {
                emit status(QString::fromStdString(utils->txtReaderLine));

                int result = moveFile2(QString::fromStdString(config->getSetting("MountedTo")+"/"+utils->txtReaderLine),
                                       QString::fromStdString(config->modPath)+"/"+mod+"/"+QString::fromStdString(utils->txtReaderLine))
                             .first;
                if(result == 0) success++;
                else if(result == 2) missing++;
                else failed++;
            }
        }

        if(success > 0)
        {
            std::ofstream out_files(config->outFilesPath.c_str());
            out_files << "";
            out_files.close();
        }
    }

    if(utils->txtReaderStart(config->backupFilesPath))
    {
        emit appendStatus("restoring backups...");

        while(utils->txtReaderNext())
        {
            if(utils->txtReaderLine != "")
            {
                emit status(QFileInfo(QString::fromStdString(utils->txtReaderLine)).filePath());
                moveFile2(QString::fromStdString(utils->txtReaderLine),
                         QString::fromStdString(utils->txtReaderLine.substr(0, utils->txtReaderLine.find_last_of("."))));
            }
        }

        if(success > 0)
        {
            std::ofstream backup_files(config->backupFilesPath.c_str());
            backup_files << "";
            backup_files.close();
        }

        emit appendStatus(" ");
    }

    emit unmountModReady(success, failed, missing);
}

std::pair<int, std::string> Worker::moveFile2(QString src, QString dst, bool copy)
{
    std::string backupPath = "";
    int result = 1;
    QFileInfo fiSrc(src);

    if(fiSrc.isFile())
    {
        //if dst exists, make backup
        QFileInfo fiDst(dst);
        if(fiDst.exists())
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
            else emit status("Failed to create backup of "+dst, true);
        }
        else QDir().mkpath(fiDst.absolutePath());

        if(copy ? QFile::copy(src, dst) : QFile::rename(src, dst))
        {
            result = 0;

            //Delete empty folders from src
            if(!copy)
            {
                QDir dirMod(fiSrc.absolutePath());
                dirMod.setFilter(QDir::NoDotAndDotDot|QDir::AllEntries);

                while(dirMod.count() == 0) {
                    QString delPath = dirMod.absolutePath();
                    dirMod.cdUp();

                    if(     delPath.toStdString() != config->modPath
                        &&  delPath.toStdString() != config->getSetting("GamePath")
                        &&  dirMod.absolutePath().toStdString() != config->modPath)
                        QDir().rmdir(delPath);
                    else break;
                }
            }
        }
    }
    else if(src != "" && !fiSrc.exists()) result = 2;

    if(result != 0)
        emit status(result == 2 ? "Missing file: "+src : tr("Failed to ")+tr(copy ? "copy" : "move")+tr(" file: ")+src, true);

    return { result, backupPath };
}

Controller::Controller(MainWindow *newMw, QString newAction, QString newMod)
{
    mw = newMw;
    action = newAction;
    mod = newMod;

    worker = new Worker(mw->config);
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Controller::moveFolder, worker, &Worker::moveFolderWorker);
    connect(this, &Controller::unmountMod, worker, &Worker::unmountModWorker);
    connect(worker, &Worker::moveFolderReady, this, &Controller::result);
    connect(worker, &Worker::deleteFolderReady, this, &Controller::result);
    connect(worker, &Worker::unmountModReady, this, &Controller::result);
    connect(worker, &Worker::status, this, &Controller::status);
    connect(worker, &Worker::appendStatus, this, &Controller::appendStatus);

    fileStatus = new FileStatus(mw);
    connect(fileStatus, SIGNAL(rejected()), this, SLOT(cancel()));
    fileStatus->setText(action+" "+mod+":");
    fileStatus->setWindowTitle(action+" "+mod+"...");

    workerThread.start();
}

void Controller::status(QString msg, bool error)
{
    if(!fileStatus->isVisible()) fileStatus->show();

    if(error) fileStatus->addErrorText(msg);
    else fileStatus->setInfoText(msg);
}

void Controller::appendStatus(QString msg)
{
    fileStatus->setText(action+" "+mod+": "+msg);
}

void Controller::result(int success, int failed, int missing)
{
    disconnect(fileStatus, SIGNAL(rejected()), this, SLOT(cancel()));
    if(failed+missing > 0)
    {
        QString msg = QString("%0 files succeeded").arg(success);
        if(failed > 0) msg += QString(", %0 files failed").arg(failed);
        if(missing > 0) msg += QString(", %2 files missing").arg(failed).arg(missing);
        msg += ".";

        fileStatus->result(msg);
        if(!fileStatus->isVisible()) fileStatus->show();
    }
    else this->~Controller();
}

void Controller::cancel()
{
    std::cout << "cancel" << std::endl;
}

Controller::~Controller()
{
    if(fileStatus->isVisible()) fileStatus->close();
    workerThread.quit();
    workerThread.wait();
}

