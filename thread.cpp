#define WINVER _WIN32_WINNT_WIN7 //Must be at least Vista for CreateSymbolicLink()
#ifdef _WIN32_WINNT
    #undef _WIN32_WINNT
#endif
#define _WIN32_WINNT _WIN32_WINNT_WIN7

#include "_dic.h"
#include "_msgr.h"
#include "thread.h"
#include "thread_pvt.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QDirIterator>
#include <QProcess>
#include <QApplication>
#include <QStringList>
#include <QWaitCondition>
#include <QMutex>

#include <windef.h>  // winbase.h needs to be
#include <winbase.h> // preceded by windef.h
#include <cmath>

#include <QDebug>

/********************************************************************/
/*      THREADACTION        *****************************************/
/********************************************************************/
    ThreadAction::ThreadAction(const Action &action, const QString &modName)
        : PROCESSING(action == Mount     ? d::MOUNTING
                     : action == Unmount ? d::UNMOUNTING
                     : action == Add     ? d::ADDING
                     : action == Delete  ? d::DELETING
                                         : d::PROCESSING),

          modName(modName), action(action) {}

/********************************************************************/
/*      FILESTATUS DIALOG       *************************************/
/********************************************************************/
    ProgressDiag::ProgressDiag(QWidget *parent, const QString &status, QWaitCondition *confirmWait)
        : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint),
          confirmWait(confirmWait), status(status)
    {
        setWindowTitle(status);
        setWindowFlag(Qt::WindowCloseButtonHint, false);
        setFixedSize(400, 90);

        QVBoxLayout *layout = new QVBoxLayout;
        layout->setContentsMargins(9, 9, 9, 12);
        setLayout(layout);

            statusLbl = new QLabel;
            statusLbl->setFixedHeight(13);
            statusLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            statusLbl->setText(status+":");
            layout->addWidget(statusLbl);

            infoLbl = new QLabel;
            infoLbl->setFixedHeight(13);
            infoLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            layout->addWidget(infoLbl);

            buttonBox = new QDialogButtonBox(QDialogButtonBox::Abort);
            layout->addWidget(buttonBox);

        connect(buttonBox, &QDialogButtonBox::rejected, this, &ProgressDiag::reject);
    }

    void ProgressDiag::showProgress(const QString &msg, const bool error)
    {
        if(error)
        {
            if(!errorTxt)
            {
                setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, false);
                setMinimumHeight(180);
                setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
                setSizeGripEnabled(true);
                errorTxt = new QPlainTextEdit;
                //errorTxt->setMinimumSize(380, 90);
                errorTxt->setAcceptDrops(false);
                errorTxt->setInputMethodHints(Qt::ImhNone);
                errorTxt->setUndoRedoEnabled(false);
                errorTxt->setLineWrapMode(QPlainTextEdit::NoWrap);
                errorTxt->setReadOnly(true);
                errorTxt->setTextInteractionFlags(Qt::TextSelectableByMouse);
                layout()->addWidget(errorTxt);

                adjustSize();
            }
            errorTxt->moveCursor(QTextCursor::End);
            errorTxt->insertPlainText(QDir::toNativeSeparators(msg)+"\n");
        }
        else infoLbl->setText(QDir::toNativeSeparators(msg));

        if(!isVisible()) show();
    }

    void ProgressDiag::showResult(/* OBSOLETE - const bool enableForce */)
    {
        setWindowFlag(Qt::WindowCloseButtonHint);

        disconnect(buttonBox, &QDialogButtonBox::rejected, this, &ProgressDiag::reject);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &ProgressDiag::accept);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        /* OBSOLETE *
        if(enableForce)
        {
            if(!forceBtn)
            {
                forceBtn = new QPushButton(d::FORCE_X.arg(d::UNMOUNT));
                buttonBox->addButton(forceBtn, QDialogButtonBox::ActionRole);
                connect(forceBtn, &QPushButton::clicked, this, &ProgressDiag::forceUnmount);
            }
        }
        else if(forceBtn)
        {
            disconnect(forceBtn, &QPushButton::clicked, this, &ProgressDiag::forceUnmount);
            buttonBox->removeButton(forceBtn);
            delete forceBtn;
            forceBtn = nullptr;
        }
        **/

        if(errorTxt)
        {
            errorTxt->moveCursor(QTextCursor::End);
            errorTxt->insertPlainText("-----------------\n\n");
        }
    }

    void ProgressDiag::appendStatus(const QString &msg)
    {
        statusLbl->setText(QStringLiteral(u"%0: %1").arg(status, msg));
    }

    void ProgressDiag::reject()
    {
        if(doAbort)
        {
            emit interrupted();
            if(QMessageBox::warning(this, d::ABORT+"?", d::ARE_YOU_SURE_Xq.arg(d::ABORT),
                                    QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
                emit aborted(); // calls confirmWait->wakeAll()
            else confirmWait->wakeAll();
        }
        else QDialog::reject();
    }

    /* OBSOLETE *
    void ProgressDiag::forceUnmount()
    {
        if(QMessageBox::warning(this, d::FORCE_X.arg(d::lUNMOUNT)+"?", d::ARE_YOU_SURE_Xq.arg(d::FORCE_X).arg(d::lUNMOUNT),
                                QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
            emit unmountForced();
    }
    **/

/********************************************************************/
/*      THREAD WORKER       *****************************************/
/********************************************************************/
    const QString ThreadWorker::extBackup = QStringLiteral(u".wmmbackup");

    void ThreadWorker::init(const qint64 index, const QString &data1, const QString &data2,
                            const QString &args, const md::modData &modData)
    {

        switch(action.action)
        {
    // MOD DATA (data1 -> mountedMod)
        case ThreadAction::ModData:
        {
            bool mountedFound = data1.isEmpty();
            md::modData newData;
            QStringList modNames;

            int i=0;
            for(QDirIterator itMods(pathMods, QDir::NoDotAndDotDot|QDir::Dirs|QDir::NoSymLinks);
                itMods.hasNext(); ++i)
            {
                itMods.next();
                const QString &modName = itMods.fileName();
                
                if(!mountedFound && data1 == modName) mountedFound = true;

                modNames << modName;
                newData.insert({ modName, md::newData(i, md::exists(modData, modName) ? md::busy(modData.at(modName)) : false ) });
            }
            
            if(!mountedFound)
            {
                modNames.insert(0, data1);
                newData.insert({ data1, md::newData(0, false) });
                for(int i=1; i < modNames.length(); ++i) // Renumber mods following added
                    md::setRow(newData[modNames[i]], i);
            }

            emit modDataReady(newData, modNames);

            break;
        }

    // SCAN
        case ThreadAction::Scan:
            scanPath(pathMods+"/"+action.modName);
            emit scanModReady(action.modName);
            break;
            
    // SCAN EXTERNAL (data1 -> modPath)
       case ThreadAction::ScanEx:
            scanPath(data1);
            emit scanModReady(action.modName);
            break;

    // MOUNT
        case ThreadAction::Mount:
            action.add(processFile(pathMods+"/"+action.modName, pathGame+"/"+md::w3mod, Link));
            emit resultReady(action);
            break;

    // UNMOUNT
        case ThreadAction::Unmount:
        {
            const QString &modPath = pathGame+"/"+md::w3mod;
            const QFileInfo &fiMounted(modPath);
            
            if(!fiMounted.isSymLink() && !fiMounted.exists())
                emit progressUpdate(d::X_NOT_MOUNTED__X.arg(action.modName, d::NOTHING_UNMOUNT_), true);
            else if(fiMounted.isSymLink())
            {
                const QFileInfo &fiTarget = QFileInfo(fiMounted.symLinkTarget());
                const QFileInfo &fiNew = QFileInfo(pathGame+"/"+md::w3modX.arg(fiTarget.fileName()));

                if(action.modName != fiTarget.fileName())
                    emit progressUpdate(d::X_NOT_MOUNTED__X.arg(action.modName, d::UNMOUNTING_X___.arg(fiTarget.fileName())), true);

                if(fiTarget.absolutePath() == pathMods || (fiNew.isSymLink() && fiNew.symLinkTarget() == fiTarget.absoluteFilePath()))
                    action.add(processFile(modPath, pathGame, Delete));
                else action.add(backupFile(modPath, false, pathGame+md::w3modX.arg(fiTarget.fileName()+"%0")) ? ThreadAction::Success
                                                                                                              : ThreadAction::Failed);
            }
            else
            {
                if(action.modName != md::unknownMod)
                    emit progressUpdate(d::X_NOT_MOUNTED__X.arg(action.modName, d::UNMOUNTING_X___.arg(d::lUNKNOWN_MOD)), true);
                action.add(backupFile(modPath, false, pathGame+md::w3modX.arg(d::lUNKNOWN+"%0")) ? ThreadAction::Success
                                                                                                 : ThreadAction::Failed);
            }
            
            emit resultReady(action);
            
            break;
        }

     // ADD (index, data1, data2 -> (bool)copy, src, dst)
        case ThreadAction::Add:
        {
            bool created = false;
            for(QDirIterator srcItr(data1, QDir::NoDotAndDotDot|QDir::Files|QDir::Hidden|QDir::System, QDirIterator::Subdirectories);
                !action.aborted() && srcItr.hasNext();
                checkState())
            {
                srcItr.next();
                const QString relativePath = srcItr.filePath().remove(data1+"/"),
                              itrDst = data2+"/"+relativePath;

                emit progressUpdate(relativePath);

                ThreadAction::Result result = processFile(srcItr.filePath(), itrDst, index ? Mode::Copy : Mode::Move);
                action.add(result);

                if(result == ThreadAction::Success)
                {
                    if(!created)
                    {
                        int row=-1;
                        for(QDirIterator itMods(pathMods, QDir::NoDotAndDotDot|QDir::Dirs|QDir::NoSymLinks);
                            itMods.hasNext() && QDir(itMods.filePath()).dirName() != action.modName;
                            ++row) itMods.next();

                        if(row != -1)
                        {
                            created = true;
                            emit modAdded(action.modName, row);
                            QThread::msleep(10); // make sure signal arrives first
                        }
                    }

                    if(created) scanFile(itrDst);
                }
            }

            emit resultReady(action);

            break;
        }

     // DELETE (index, data1 -> size, fileCount)
        case ThreadAction::Delete:
        {
            if(index > 0) modSize = index;
            if(!data1.isEmpty()) getFileCount(data1);

            const QString &pathMod = pathMods+"/"+action.modName;

            for(QDirIterator itMod(pathMod, QDir::NoDotAndDotDot|QDir::Files|QDir::Hidden|QDir::System,  QDirIterator::Subdirectories);
                !action.aborted() && itMod.hasNext(); checkState())
            {
                itMod.next();
                const QString &filePath = itMod.filePath();

                emit progressUpdate(filePath);

                scanFile(filePath, true, true); // Silently remove file from data

                ThreadAction::Result result = processFile(filePath, pathMods, ThreadWorker::Delete);
                action.add(result);

                if(result != ThreadAction::Success && (itMod.fileInfo().isSymLink() || itMod.fileInfo().exists()))
                    scanFile(filePath, false, true); // Silently add file back to data if it still exists

                emit scanModUpdate(action.modName, getMB(), d::X_FILES.arg(fileCount), modSize); // Data is up to date
            }

            if(!QFileInfo().exists(pathMod)) emit modDeleted(action.modName);

            emit resultReady(action);

            break;
        }

     // SHORTCUT (index, data1, data2, args -> iconIndex, dst, iconPath, args)
        case ThreadAction::Shortcut:
        {
            // Using QProcess because winAPI method requires ~7MB of libraries
            QStringList cmds = {
                    QString("$s=(New-Object -COM WScript.Shell).CreateShortcut('%0.lnk');")
                         .arg(QDir::toNativeSeparators(data1)),
                    QString("$s.TargetPath='%0';")
                         .arg(QDir::toNativeSeparators(QCoreApplication::applicationFilePath())),
                    QString("$s.WorkingDirectory='%0';")
                         .arg(QDir::toNativeSeparators(QCoreApplication::applicationDirPath()))
                };
            if(!args.isEmpty())     cmds << QString("$s.Arguments='%0';").arg(args);
            if(!data2.isEmpty()) cmds << QString("$s.IconLocation='%0, %1';")
                                                .arg(QDir::toNativeSeparators(data2), int(index));
            cmds << QString("$s.Save()");

            QProcess powershell(this);
            powershell.start("powershell", cmds);
            powershell.waitForFinished();

            if(powershell.exitCode() == 0) emit msgr->msg(d::SHORTCUT_CREATED_, Msgr::Info);
            else emit msgr->msg(d::FAILED_TO_CREATE_SHORTCUTc_X_.arg(powershell.errorString()), Msgr::Error);

            emit shortcutReady();

            break;
        }

     // (no action)
        case ThreadAction::NoAction:;
        }

        /********************************************************************/
        /*      OBSOLETE - may be useful later      *************************/
        /********************************************************************

            case ThreadAction::ScanMounted:
            {
                std::ifstream txtReader(pathOutFiles);
                for(std::string line; std::getline(txtReader, line); ) if(line != "")
                {
                    QFileInfo fi(QString::fromStdString(line));
                    if(fi.isSymLink() && fi.exists())
                    {
                        const QString &linkTrgt = fi.symLinkTarget();
                        fi = QFileInfo(linkTrgt);
                        if(fi.isSymLink() || fi.exists()) scanPath(linkTrgt);
                    }
                }

                emit scanModReady(action.modName);

                break;
            }

            case ThreadAction::Mount:
            {
                mountModIterator();

                if(outFilesIt.is_open()) outFilesIt.close();
                if(backupFilesIt.is_open()) backupFilesIt.close();

                emit resultReady(action);

                break;
            }

            case ThreadAction::Unmount:
            {
                if(index > 0) modSize = index;
                if(!data1.isEmpty()) getFileCount(data1);

                std::string newOutFiles = "";
                std::ifstream txtReader(pathOutFiles);
                for(std::string line; std::getline(txtReader, line); checkState()) if(line != "")
                {
                    if(!action.aborted())
                    {
                        const QString &qsLine = QString::fromStdString(line);
                        const QFileInfo &fiLine(qsLine);
                        const QString &targetPath = fiLine.symLinkTarget(); // Must be defined before calling deleteFile(qsLine)

                        emit progressUpdate(qsLine);

                        ThreadAction::Result resultIt = ThreadAction::Failed;
                        if(fiLine.isSymLink()) resultIt = processFile(qsLine, QString(), ThreadWorker::Delete);
                        else if(!fiLine.exists())
                        {
                            resultIt = ThreadAction::Missing;
                            emit progressUpdate(d::MISSING_FILE_X.arg(qsLine), true);
                        }
                        else emit progressUpdate(d::NOT_A_SYMLINKc_X.arg(qsLine), true);

                        if(resultIt == ThreadAction::Success)
                        {
                            const int beforeCount = fileCount;
                            scanPath(targetPath, true);
                            action.add(resultIt, beforeCount-fileCount);
                        }
                        else
                        {
                            action.add(resultIt);
                            if(!action.forced()) newOutFiles += line+"\n";
                        }
                    }
                    else newOutFiles += line+"\n";
                }
                txtReader.close();

                std::ofstream txtWriter(pathOutFiles);
                txtWriter << newOutFiles;
                txtWriter.close();

                if(!action.aborted())
                {
                    emit statusUpdate(d::lRESTORING_BACKUPS___);

                    std::string newBackupFiles = "";
                    txtReader.open(pathBackupFiles);
                    for(std::string line; std::getline(txtReader, line); checkState()) if(line != "")
                    {
                        if(!action.aborted())
                        {
                            const QString &qsLine = QString::fromStdString(line);
                            QString dst = qsLine;
                            dst.truncate(dst.lastIndexOf(extBackup));

                            emit progressUpdate(QFileInfo(qsLine).fileName());

                            if(processFile(qsLine, dst) != ThreadAction::Success && !action.forced()) newBackupFiles += line+"\n";
                        }
                        else newBackupFiles += line+"\n";
                    }
                    txtReader.close();

                    txtWriter.open(pathBackupFiles);
                    txtWriter << newBackupFiles;
                    txtWriter.close();

                    emit statusUpdate(QString());
                }

                emit resultReady(action);

                break;
            }
         ********************************************************************/
        /********************************************************************/
        /********************************************************************/
    }

    /* OBSOLETE *
    void ThreadWorker::forceUnmount()
    {
        if(action == ThreadAction::Unmount)
        {
            action.force();
            init();
        }
    }
    **/

    void ThreadWorker::checkState()
    {
        if(mutex)
        {
            mutex->lock();
            if(paused)
            {
                confirmWait->wait(mutex);
                paused = false;
            }
            mutex->unlock();
        }
    }

    QString ThreadWorker::getMB()
    {
        if(modSize > 0)
        {
            double sizeMB = double(modSize)/1024/1024;
            if(sizeMB < 0.01) return d::ALMOST_ZERO_MB;
            else
            {
                QString qsModSize = QString::number(round(sizeMB*100)/100);
                if(qsModSize.lastIndexOf('.') == qsModSize.length()-2) qsModSize += "0";
                else if(qsModSize.lastIndexOf('.') == -1) qsModSize += ".00";
                return d::X_MB.arg(qsModSize);
            }
        }
        else return d::ZERO_MB;
    }

    void ThreadWorker::getFileCount(QString qsFileCount)
    {
        qsFileCount.truncate(qsFileCount.lastIndexOf(d::X_FILES.arg(QString())));
        fileCount = qsFileCount.toInt();
    }

    qint64 ThreadWorker::scanFile(const QFileInfo &fi, const bool subtract, const bool silent)
    {
        qint64 size = 0;
        if(fi.isSymLink() && fi.size() == QFileInfo(fi.symLinkTarget()).size())
        {
            const QString &filePath = fi.filePath();

            QFile file(filePath);
            if(file.exists() && file.open(QIODevice::ReadOnly))
                size = file.size();
            file.close();

            if(size == 0)
            {
                QString tmpPath = filePath+".wmmTmp";
                for(int i=2; QFileInfo().exists(tmpPath); ++i)
                    tmpPath = filePath+".wmmTmp"+QString::number(i);

                if(processFile(filePath, tmpPath, Mode::Copy) == ThreadAction::Success)
                {
                    size = QFileInfo(tmpPath).size();
                    processFile(tmpPath, QString(), ThreadWorker::Delete);
                }
            }
        }
        else size = fi.size();

        modSize += (subtract ? -1 : 1) * size;
        fileCount += subtract ? -1 : 1;

        if(!silent) emit scanModUpdate(action.modName, getMB(), d::X_FILES.arg(fileCount), modSize);

        return size;
    }

    void ThreadWorker::scanPath(const QString &path, const bool subtract)
    {
        QFileInfo itrFi(path);
        if(itrFi.isSymLink() || !itrFi.isDir()) scanFile(itrFi, subtract);
        else for(QDirIterator pathItr(path, QDir::NoDotAndDotDot|QDir::Files|QDir::Hidden|QDir::System, QDirIterator::Subdirectories);
                 pathItr.hasNext(); scanFile(pathItr.fileInfo(), subtract)) pathItr.next();
    }

    ThreadAction::Result ThreadWorker::processFile(const QString &src, const QString &dst, const Mode &mode, const bool logBackups)
    {
        ThreadAction::Result result = ThreadAction::Failed;
        const QFileInfo &fiSrc(src);

        if(fiSrc.isSymLink() || (fiSrc.exists() && (fiSrc.isFile() || (mode == Link && fiSrc.isDir()))))
        {
            QFileInfo fiDst(dst);
            //if dst exists, make backup
            if(mode != Delete && (fiDst.isSymLink() || fiDst.exists()))
            {
                if(!backupFile(dst, logBackups))
                {
                    emit progressUpdate(d::FAILED_TO_CREATE_BACKUP_X.arg(dst)+"\n"+d::SKIPPING_FILE_X.arg(src), true);
                    return ThreadAction::Failed;
                }
            }
            else if(!dst.isEmpty()) QDir().mkpath(fiDst.absolutePath());

            switch(mode)
            {
            case Link:
                if(CreateSymbolicLink(QDir::toNativeSeparators(dst).toStdWString().c_str(),
                                      QDir::toNativeSeparators(src).toStdWString().c_str(),
                                      fiSrc.isDir() ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0x0))
                    result = ThreadAction::Success;
                break;
            case Move:
                if(QFile::rename(src, dst))
                {
                    result = ThreadAction::Success;
                    removePath(fiSrc.absolutePath());
                }
                break;
            case Copy:
                if(QFile::copy(src, dst)) result = ThreadAction::Success;
                break;
            case Delete:
                if((fiSrc.isFile() && QFile(src).remove())
                    || (fiSrc.isSymLink() && fiSrc.isDir() && QDir().rmdir(src)))
                {
                    result = ThreadAction::Success;
                    removePath(fiSrc.absolutePath(), dst);
                }
            }
        }
        else
        {
            result = ThreadAction::Missing;
            emit progressUpdate(d::MISSING_FILE_X.arg(src), true);
        }

        if(result == ThreadAction::Failed)
            emit progressUpdate(d::FAILED_TO_X.arg((mode == Copy ? d::lCOPY : mode == Link ? d::lCREATE_SYMLINK_TO
                                                    : mode == Delete ? d::lDELETE : d::lMOVE)
                                                   +" "+d::lFILEc_X.arg(src)), true);

        return result;
    }
    
    bool ThreadWorker::backupFile(const QString &src, const bool logBackups, QString dstMarked)
    {
        if(dstMarked.isEmpty()) dstMarked = src+extBackup+"%0";
        QString backupPath = dstMarked.arg(QString());

        for(int i=2; QFileInfo().exists(backupPath); ++i)
            backupPath = dstMarked.arg(QString::number(i));

        if(QFile::rename(src, backupPath))
        {
            if(logBackups)
            {
                if(!backupFilesIt.is_open()) backupFilesIt.open(pathBackupFiles);
                backupFilesIt << backupPath.toStdString() << std::endl;
            }
            
            return true;
        }
        else return false;
    }

    void ThreadWorker::removePath(const QString &path, const QString &stopPath)
    {
        QDir dirEmpty(path);
        dirEmpty.setFilter(QDir::NoDotAndDotDot|QDir::AllEntries|QDir::Hidden|QDir::System);

        while(dirEmpty.count() == 0) {
            const QString &delPath = dirEmpty.absolutePath();
            dirEmpty.cdUp();

            if(   delPath != stopPath
               && delPath != pathMods
               && delPath != pathGame
               && (!stopPath.isEmpty() || dirEmpty.absolutePath() != pathMods))
            {
                if(!QDir().rmdir(delPath)) emit progressUpdate(d::FAILED_TO_DELETE_EMPTY_FOLDERc_X_.arg(delPath), true);
            }
            else break;
        }
    }

    /********************************************************************/
    /*      OBSOLETE - may be useful later      *************************/
    /********************************************************************

        void ThreadWorker::mountModIterator(QString relativePath)
        {
            for(QDirIterator itMod(pathMods+"/"+action.modName+(!relativePath.isEmpty() ? "/"+relativePath : QString()),
                                   QDir::NoDotAndDotDot|QDir::AllEntries|QDir::Hidden|QDir::System);
                !action.aborted() && itMod.hasNext();
                checkState())
            {
                itMod.next();
                const QString &src = itMod.filePath();
                relativePath = src;
                relativePath.remove(pathMods+"/"+action.modName+"/");
                const QString &dst = pathGame+"/"+relativePath;
                const QFileInfo &fiDst = QFileInfo(dst);

                if(!fiDst.isSymLink() && fiDst.exists() && fiDst.isDir()) mountModIterator(relativePath);
                else
                {
                    emit progressUpdate(relativePath);

                    const ThreadAction::Result &resultIt = processFile(src, dst, Link, true);

                    if(resultIt == ThreadAction::Success)
                    {
                        if(!outFilesIt.is_open()) outFilesIt.open(pathOutFiles);
                        outFilesIt << dst.toStdString() << std::endl;

                        const int beforeCount = fileCount;
                        scanPath(src);
                        action.add(resultIt, fileCount-beforeCount);
                    }
                    else action.add(resultIt);
                }
            }
        }
     ********************************************************************/
    /********************************************************************/
    /********************************************************************/

/********************************************************************/
/*      THREAD CONTROLLER       *************************************/
/********************************************************************/

    Thread::Thread(const ThreadAction::Action &thrAction, const QString &modName,
                   const QString &pathMods, const QString &pathGame, Msgr *const msgr)
        : ThreadBase(), action(ThreadAction(thrAction, modName))
    {
        if(action == ThreadAction::NoAction)
        {
            qDebug() << "Thread::Thread: ThreadAction::NoAction -- deleting Thread.";
            if(msgr) emit msgr->msg(d::INVALID_ACTION_, Msgr::Error);
            deleteLater();
        }
        else
        {
            ThreadWorker *worker = new ThreadWorker(action, paused, pathMods, pathGame, msgr);
            worker->moveToThread(&workerThread);

            connect(&workerThread, &QThread::finished,           worker, &ThreadWorker::deleteLater);
            connect(this,          &Thread::init,                worker, &ThreadWorker::init);
            connect(worker,        &ThreadWorker::scanModUpdate, this,   &Thread::scanModUpdate);

            switch(action.action)
            {
            case ThreadAction::ModData:
                connect(worker, &ThreadWorker::modDataReady, this, &Thread::modDataReady);
                connect(worker, &ThreadWorker::modDataReady, this, &Thread::deleteLater);
                break;
            case ThreadAction::Scan: case ThreadAction::ScanEx:
                connect(worker, &ThreadWorker::scanModReady, this, &Thread::scanModReady);
                connect(worker, &ThreadWorker::scanModReady, this, &Thread::deleteLater);
                break;
            case ThreadAction::Shortcut:
                connect(worker, &ThreadWorker::shortcutReady, this, &Thread::shortcutReady);
                connect(worker, &ThreadWorker::shortcutReady, this, &Thread::deleteLater);
                break;
            default:
                confirmWait = new QWaitCondition;
                mutex = new QMutex;

                progressDiag = new ProgressDiag(QApplication::activeModalWidget(), action.PROCESSING+" "+action.modName,
                                                confirmWait);

                worker->setWaitConditionEx(confirmWait, mutex);

                connect(worker, &ThreadWorker::resultReady,    this,         &Thread::resultReady);
                connect(worker, &ThreadWorker::resultReady,    this,         &Thread::processResult);
                connect(worker, &ThreadWorker::progressUpdate, progressDiag, &ProgressDiag::showProgress);
                connect(worker, &ThreadWorker::statusUpdate,   progressDiag, &ProgressDiag::appendStatus);

                connect(progressDiag, &ProgressDiag::interrupted, this, &Thread::pause);
                connect(progressDiag, &ProgressDiag::aborted,     this, &Thread::abort);

                /* OBSOLETE *
                if(action == ThreadAction::Unmount)
                    connect(progressDiag, &ProgressDiag::unmountForced, worker, &ThreadWorker::forceUnmount);
                else **/if(action == ThreadAction::Add)
                    connect(worker, &ThreadWorker::modAdded, this, &Thread::modAdded);
                else if(action == ThreadAction::Delete)
                    connect(worker, &ThreadWorker::modDeleted, this, &Thread::modDeleted);
            }

            workerThread.start();
        }
    }

    Thread::~Thread()
    {
        workerThread.quit();
        workerThread.wait();
        if(progressDiag)
        {
            progressDiag->close();
            delete progressDiag;
        }
    }

    void Thread::abort()
    {
        action.abort();
        confirmWait->wakeAll();
    }

    void Thread::pause()
    {
        mutex->lock();
        paused = true;
        mutex->unlock();
    }

    void Thread::processResult()
    {
        bool deleteThread = true;

        if(progressDiag)
        {
            progressDiag->disableAbort();

            if(!action.errors() && !progressDiag->errors()) progressDiag->close();
            else
            {
                deleteThread = false;
                connect(progressDiag, &ProgressDiag::accepted, this, &Thread::deleteLater);
                connect(progressDiag, &ProgressDiag::rejected, this, &Thread::deleteLater);

                                         // show Force button? (only as last resort)
                progressDiag->showResult(/* OBSOLETE - !action.forced() && !action.aborted() && !action.get(ThreadAction::Success)
                                         && action == ThreadAction::Unmount */);

                progressDiag->appendStatus(action.aborted()                             ? d::lABORTED+"."
                                           : /*action.forced() || */action.success() ?
                                                 /*action.forced() || */action.errors() ? d::lFINISHED_WITH_ERRORS
                                                                                        : d::lDONE_
                                                                                        : d::lFAILED+".");

                progressDiag->showProgress(d::X_FILES_SUCCEEDED.arg(action.get(ThreadAction::Success))
                                           +(action.get(ThreadAction::Failed)
                                             ? ", "+d::X_FAILED.arg(d::X_FILES).arg(action.get(ThreadAction::Failed))
                                             : QString())
                                           +(action.get(ThreadAction::Missing)
                                             ? ", "+d::X_MISSING.arg(d::X_FILES).arg(action.get(ThreadAction::Missing))
                                             : QString())
                                           +".");
            }
        }

        if(deleteThread) deleteLater();
    }
