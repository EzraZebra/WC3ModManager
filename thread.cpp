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
        setLayout(layout);
        layout->setContentsMargins(9, 9, 9, 12);

            statusLbl = new QLabel;
            infoLbl   = new QLabel;
            layout->addWidget(statusLbl);
            layout->addWidget(infoLbl);
            statusLbl->setFixedHeight(13);
            infoLbl->setFixedHeight(13);
            statusLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            infoLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

            statusLbl->setText(status+":");

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
                layout()->addWidget(errorTxt);
                //errorTxt->setMinimumSize(380, 90);
                errorTxt->setAcceptDrops(false);
                errorTxt->setInputMethodHints(Qt::ImhNone);
                errorTxt->setUndoRedoEnabled(false);
                errorTxt->setLineWrapMode(QPlainTextEdit::NoWrap);
                errorTxt->setReadOnly(true);
                errorTxt->setTextInteractionFlags(Qt::TextSelectableByMouse);

                adjustSize();
            }
            errorTxt->moveCursor(QTextCursor::End);
            errorTxt->insertPlainText(QDir::toNativeSeparators(msg)+"\n");
        }
        else infoLbl->setText(QDir::toNativeSeparators(msg));

        if(!isVisible()) show();
    }

    void ProgressDiag::showResult(const bool enableForce)
    {
        setWindowFlag(Qt::WindowCloseButtonHint);

        disconnect(buttonBox, &QDialogButtonBox::rejected, this, &ProgressDiag::reject);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &ProgressDiag::accept);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        if(forceBtn) disconnect(forceBtn, &QPushButton::clicked, this, &ProgressDiag::forceUnmount);
        if(enableForce)
        {
            if(!forceBtn) forceBtn = new QPushButton(d::FORCE_X.arg(d::UNMOUNT));
            buttonBox->addButton(forceBtn, QDialogButtonBox::ActionRole);
            connect(forceBtn, &QPushButton::clicked, this, &ProgressDiag::forceUnmount);
        }
        else if(forceBtn)
        {
            buttonBox->removeButton(forceBtn);
            delete forceBtn;
            forceBtn = nullptr;
        }

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
            {
                emit aborted(); // calls confirmWait->wakeAll()
            } else confirmWait->wakeAll();
        }
        else QDialog::reject();
    }

    void ProgressDiag::forceUnmount()
    {
        if(QMessageBox::warning(this, d::FORCE_X.arg(d::lUNMOUNT)+"?", d::ARE_YOU_SURE_Xq.arg(d::FORCE_X).arg(d::lUNMOUNT),
                                QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
            emit unmountForced();
    }

/********************************************************************/
/*      THREAD WORKER       *****************************************/
/********************************************************************/
    const QString ThreadWorker::extBackup = QStringLiteral(u".wmmbackup");

    void ThreadWorker::init(const int index, const QString &path1, const QString &path2,
                            const QString &args, const mod_m &modData)
    {

        switch(action.action)
        {
        case ThreadAction::ModData:     modDataWorker(modData);
            break;
        case ThreadAction::Scan:        scanModWorker();
            break;
        case ThreadAction::ScanMounted: scanMountedModWorker();
            break;
        case ThreadAction::Mount:       mountModWorker();
            break;
        case ThreadAction::Unmount:     unmountModWorker();
            break;
        case ThreadAction::Add:         addModWorker(path1, path2, Mode(index)); //src , dst, mode
            break;
        case ThreadAction::Delete:      deleteModWorker();
            break;
        case ThreadAction::Shortcut:    shortcutWorker(path1, args, path2, index); //dst, args, icon path, icon index
            break;
        case ThreadAction::NoAction:;
        }
    }

    void ThreadWorker::forceUnmount()
    {
        if(action == ThreadAction::Unmount)
        {
            action.force();
            unmountModWorker();
        }
    }

    QString ThreadWorker::B2MB(double size)
    {
        QString result = "0.00";
        if(size > 0)
        {
            size = size/1024/1024;
            if(size < 0.01) result = "< 0.01";
            else
            {
                result = QString::number(round(size*100)/100);
                if(result.lastIndexOf('.') == result.length()-2)
                    result += "0";
                else if(result.lastIndexOf('.') == -1)
                    result += ".00";
            }
        }

        result = d::X_MB.arg(result);

        return result;
    }

    qint64 ThreadWorker::getSize(const QFileInfo &fi)
    {
        qint64 size = 0;
        if(fi.isSymLink() && fi.size() == QFileInfo(fi.symLinkTarget()).size())
        {
            QFile file(fi.filePath());
            if(file.exists() && file.open(QIODevice::ReadOnly))
                size = file.size();
            file.close();
        }

        return size == 0 ? fi.size() : size;
    }

    ThreadAction::Result ThreadWorker::processFile(const QString &src, const QString &dst, const Mode &mode, const bool logBackups)
    {
        ThreadAction::Result result = ThreadAction::Failed;
        const QFileInfo &fiSrc(src);

        if(fiSrc.isSymLink() || (fiSrc.exists() && (fiSrc.isFile() || (mode == Link && fiSrc.isDir()))))
        {
            QFileInfo fiDst(dst);
            //if dst exists, make backup
            if(fiDst.isSymLink() || fiDst.exists())
            {
                QString backupPath = dst+extBackup;
                for(int i=2; QFileInfo().exists(backupPath); ++i)
                    backupPath = dst+extBackup+QString::number(i);

                if(QFile::rename(dst, backupPath))
                {
                    if(logBackups)
                    {
                        if(!backupFilesIt.is_open()) backupFilesIt.open(pathBackupFiles);
                        backupFilesIt << backupPath.toStdString() << std::endl;
                    }
                }
                else
                {
                    emit progressUpdate(d::FAILED_TO_CREATE_BACKUP_X.arg(dst)+"\n"+d::SKIPPING_FILE_X.arg(src), true);
                    return ThreadAction::Failed;
                }
            }
            else QDir().mkpath(fiDst.absolutePath());

            switch(mode)
            {
            case Copy:
                if(QFile::copy(src, dst)) result = ThreadAction::Success;
                break;
            case Link:
                if(CreateSymbolicLink(QDir::toNativeSeparators(dst).toStdWString().c_str(),
                                      QDir::toNativeSeparators(src).toStdWString().c_str(),
                                      fiSrc.isDir() ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0x0))
                    result = ThreadAction::Success;
                break;
            default:
                if(QFile::rename(src, dst))
                {
                    result = ThreadAction::Success;
                    removePath(fiSrc.absolutePath());
                }
            }
        }
        else if(!src.isEmpty() && !fiSrc.isSymLink() && !fiSrc.exists())
        {
            result = ThreadAction::Missing;
            emit progressUpdate(d::MISSING_FILE_X.arg(src), true);
        }

        if(result == ThreadAction::Failed)
            emit progressUpdate(d::FAILED_TO_X.arg((mode == Copy ? d::lCOPY : mode == Link ? d::lCREATE_SYMLINK_TO : d::lMOVE)
                                                   +" "+d::lFILEc_X.arg(src)), true);

        return result;
    }

    ThreadAction::Result ThreadWorker::deleteFile(const QString &path, const QString &stopPath)
    {
        const QFileInfo &fileInfo(path);
        if(!fileInfo.isSymLink() && !fileInfo.exists())
        {
            emit progressUpdate(d::MISSING_FILE_X.arg(path), true);
            return ThreadAction::Missing;
        }
        else if((fileInfo.isFile() && QFile(path).remove())
                || (fileInfo.isSymLink() && fileInfo.isDir() && QDir().rmdir(path)))
        {
            removePath(fileInfo.absolutePath(), stopPath);
            return ThreadAction::Success;
        }
        else
        {
            emit progressUpdate(d::FAILED_TO_X.arg(d::lDELETE_X).arg(d::lFILEc_X).arg(path), true);
            return ThreadAction::Failed;
        }
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
                if(!QDir().rmdir(delPath))
                    emit progressUpdate(d::FAILED_TO_DELETE_EMPTY_FOLDERc_X_.arg(delPath), true);
            }
            else break;
        }
    }

    void ThreadWorker::modDataWorker(const mod_m &modData)
    {
        mod_m newData;
        QStringList modNames;

        int i=0;
        for(QDirIterator itMods(pathMods, QDir::NoDotAndDotDot|QDir::Dirs|QDir::NoSymLinks);
            itMods.hasNext(); ++i)
        {
            itMods.next();
            const QString &modName = itMods.fileName();

            modNames << modName;

            mod_t mod = modData.find(modName) != modData.end() ? modData.at(modName) : mod_t{};
            std::get<int(ModData::Row)>(mod) = i;
            newData.insert({ modName, mod });
        }

        emit modDataReady(newData, modNames);
    }

    void ThreadWorker::scanPathIterator(const QString &path, qint64 &modSize, int &fileCount, const bool emitUpdate)
    {
        for(QDirIterator pathItr(path, QDir::NoDotAndDotDot|QDir::Files|QDir::Hidden|QDir::System,
                                 QDirIterator::Subdirectories);
            pathItr.hasNext(); )
        {
            pathItr.next();

            modSize += getSize(pathItr.fileInfo());
            ++fileCount;

            if(emitUpdate) emit scanModUpdate(action.modName, B2MB(modSize), d::X_FILES.arg(fileCount));
        }
    }

    // Scan actions are "threadsafe" because they are silent (i.e. no progressDiag/abort/pause/processResult -> no access into action)
    void ThreadWorker::scanModWorker()
    {
        qint64 modSize = 0;
        int fileCount = 0;

        scanPathIterator(pathMods+"/"+action.modName, modSize, fileCount, true);

        if(!fileCount) emit scanModUpdate(action.modName, d::ZERO_MB, d::ZERO_FILES);

        emit scanModReady(action.modName);
    }

    void ThreadWorker::scanMountedModWorker()
    {
        qint64 modSize = 0;
        int fileCount = 0;

        std::ifstream txtReader(pathOutFiles);
        for(std::string line; std::getline(txtReader, line); )
            if(line != "")
            {
                const QFileInfo &fiLine(QString::fromStdString(line)); // meow
                if(fiLine.isSymLink() && fiLine.exists())
                {
                    QFileInfo fiLink(fiLine.symLinkTarget());
                    if(fiLink.isSymLink() || fiLink.exists())
                    {
                        if(!fiLink.isSymLink() && fiLink.isDir()) scanPathIterator(fiLink.absoluteFilePath(), modSize, fileCount, true);
                        else
                        {
                            modSize += getSize(fiLink);
                            ++fileCount;
                            emit scanModUpdate(action.modName, B2MB(modSize), d::X_FILES.arg(fileCount));
                        }
                    }
                }
            }

        emit scanModReady(action.modName);
    }

    void ThreadWorker::mountModIterator(QString relativePath)
    {
        qint64 modSize = 0;
        int fileCount = 0;

        for(QDirIterator itMod(pathMods+"/"+action.modName+(!relativePath.isEmpty() ? "/"+relativePath : QString()),
                               QDir::NoDotAndDotDot|QDir::AllEntries|QDir::Hidden|QDir::System);
            !action.aborted() && itMod.hasNext(); )
        {
            itMod.next();
            const QString &src = itMod.filePath();
            relativePath = src;
            relativePath.remove(pathMods+"/"+action.modName+"/");
            const QString &dst = pathGame+"/"+relativePath;
            const QFileInfo &fiDst = QFileInfo(dst);

            if(!fiDst.isSymLink() && fiDst.exists() && fiDst.isDir())
                mountModIterator(relativePath);
            else
            {
                emit progressUpdate(relativePath);

                const ThreadAction::Result &resultIt = processFile(src, dst, Link, true);

                if(resultIt == ThreadAction::Success)
                {
                    if(!outFilesIt.is_open()) outFilesIt.open(pathOutFiles);
                    outFilesIt << dst.toStdString() << std::endl;

                    int addCount = 0;
                    if(!itMod.fileInfo().isSymLink() && itMod.fileInfo().isDir())
                        scanPathIterator(src, modSize, addCount, true);
                    else
                    {
                        addCount = 1;
                        modSize += getSize(src);
                    }

                    fileCount += addCount;
                    emit scanModUpdate(action.modName, B2MB(modSize), d::X_FILES.arg(fileCount));

                    action.add(resultIt, addCount);
                }
                else
                {
                    action.add(resultIt);
                }
            }

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
    }

    void ThreadWorker::mountModWorker()
    {
        mountModIterator();

        if(outFilesIt.is_open()) outFilesIt.close();
        if(backupFilesIt.is_open()) backupFilesIt.close();

        emit resultReady(action);
    }

    void ThreadWorker::unmountModWorker()
    {
        std::string newOutFiles = "";
        std::ifstream txtReader(pathOutFiles);
        for(std::string line; std::getline(txtReader, line); )
        {
            if(line != "")
            {
                if(!action.aborted())
                {
                    const QString &qsLine = QString::fromStdString(line);
                    const QFileInfo &fiLine(qsLine);
                    const QString &lineTrgt = fiLine.symLinkTarget();
                    emit progressUpdate(qsLine);

                    ThreadAction::Result resultIt = ThreadAction::Failed;
                    if(fiLine.isSymLink()) resultIt = deleteFile(qsLine);
                    else if(!fiLine.exists())
                    {
                        resultIt = ThreadAction::Missing;
                        emit progressUpdate(d::MISSING_FILE_X.arg(qsLine), true);
                    }
                    else emit progressUpdate(d::NOT_A_SYMLINKc_X.arg(qsLine), true);

                    if(resultIt == ThreadAction::Success)
                    {
                        int fileCount = !QFileInfo(lineTrgt).isDir();
                        if(!fileCount)
                        {
                            qint64 modSize = 0;
                            scanPathIterator(lineTrgt, modSize, fileCount);
                        }

                        action.add(resultIt, fileCount);
                    }
                    else
                    {
                        action.add(resultIt);
                        if(!action.forced()) newOutFiles += line+"\n";
                    }
                }
                else newOutFiles += line+"\n";
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

            //QCoreApplication::processEvents(); // Allow signals to come through
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
            for(std::string line; std::getline(txtReader, line); )
            {
                if(line != "")
                {
                    if(!action.aborted())
                    {
                        const QString &qsLine = QString::fromStdString(line);
                        QString dst = qsLine;
                        dst.truncate(dst.lastIndexOf(extBackup));

                        emit progressUpdate(QFileInfo(qsLine).fileName());

                        const ThreadAction::Result &resultIt = processFile(qsLine, dst);

                        action.add(resultIt);
                        if(!action.forced() && resultIt != ThreadAction::Success) newBackupFiles += line+"\n";
                    }
                    else newBackupFiles += line+"\n";

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
            }
            txtReader.close();

            txtWriter.open(pathBackupFiles);
            txtWriter << newBackupFiles;
            txtWriter.close();
        }

        emit statusUpdate(QString());
        emit resultReady(action);
    }

    void ThreadWorker::addModWorker(const QString &src, const QString &dst, const Mode &mode)
    {
        for(QDirIterator itSrc(src, QDir::NoDotAndDotDot|QDir::AllEntries|QDir::Hidden|QDir::System, QDirIterator::Subdirectories);
            !action.aborted() && itSrc.hasNext(); )
        {
            itSrc.next();
            const QString &relativePath = itSrc.filePath().remove(src+"/");

            emit progressUpdate(relativePath);

            action.add(processFile(itSrc.filePath(), dst+"/"+relativePath, mode));

            QCoreApplication::processEvents(); // Allow signals to come through
        }

        emit resultReady(action);
    }

    void ThreadWorker::deleteModWorker()
    {
        // (try to) move to temp dir so mod can't be mounted, renamed or deleted while deleting
        QString tmpDir = pathMods+"/"+action.modName,
                tmpDirTry = tmpDir+".wmmdelete";
        for(int i=2; QFileInfo().exists(tmpDirTry);
            tmpDirTry = tmpDir+".wmmdelete"+QString::number(i++))

        if(QDir().rename(tmpDir, tmpDirTry)) tmpDir = tmpDirTry;

        QDirIterator itMod(tmpDir, QDir::NoDotAndDotDot|QDir::AllEntries|QDir::Hidden|QDir::System,
                           QDirIterator::Subdirectories);
        while(!action.aborted() && itMod.hasNext())
        {
            itMod.next();
            const QString &filePath = itMod.filePath();

            emit progressUpdate(filePath);

            if(!itMod.fileInfo().isSymLink() && itMod.fileInfo().isDir()) removePath(filePath, pathMods);
            else action.add(deleteFile(filePath, pathMods));

            QCoreApplication::processEvents(); // Allow signals to come through
        }

        if(!QFileInfo().exists(tmpDir) || QDir().rmdir(tmpDir))
            action.add(ThreadAction::Success);
        else
        {
            action.add(ThreadAction::Failed);
            QDir().rename(tmpDir, pathMods+"/"+action.modName);
        }

        emit resultReady(action);
    }

    void ThreadWorker::shortcutWorker(const QString &dst, const QString &args, const QString &iconPath, const int iconIndex)
    {
        // Using QProcess because winAPI method requires ~7MB of libraries
        QStringList cmds = {
                QString("$s=(New-Object -COM WScript.Shell).CreateShortcut('%0.lnk');")
                     .arg(QDir::toNativeSeparators(dst)),
                QString("$s.TargetPath='%0';")
                     .arg(QDir::toNativeSeparators(QCoreApplication::applicationFilePath())),
                QString("$s.WorkingDirectory='%0';")
                     .arg(QDir::toNativeSeparators(QCoreApplication::applicationDirPath()))
            };
        if(!args.isEmpty())     cmds << QString("$s.Arguments='%0';").arg(args);
        if(!iconPath.isEmpty()) cmds << QString("$s.IconLocation='%0, %1';")
                                        .arg(QDir::toNativeSeparators(iconPath), iconIndex);
        cmds << QString("$s.Save()");

        QProcess powershell(this);
        powershell.start("powershell", cmds);
        powershell.waitForFinished();

        if(powershell.exitCode() == 0) emit msgr->msg(d::SHORTCUT_CREATED_, Msgr::Info);
        else emit msgr->msg(d::FAILED_TO_CREATE_SHORTCUTc_X_.arg(powershell.errorString()), Msgr::Error);

        emit shortcutReady();
    }

/********************************************************************/
/*      THREAD CONTROLLER       *************************************/
/********************************************************************/

    Thread::Thread(const ThreadAction::Action &thrAction, const QString &modName,
                   const QString &pathMods, const QString &pathGame, Msgr *const msgr)
        : ThreadBase(),
          action(ThreadAction(thrAction, modName))
    {
        if(action == ThreadAction::NoAction)
        {
            qDebug() << "Thread::action == ThreadAction::NoAction\nDeleting Thread.";
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
            case ThreadAction::Scan: case ThreadAction::ScanMounted:
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

                if(action == ThreadAction::Unmount)
                    connect(progressDiag, &ProgressDiag::unmountForced, worker, &ThreadWorker::forceUnmount);

                progressDiag->show();
            }

            workerThread.start(); // From this point on, call mutex->lock() ... mutex->unlock() before accessing action
        }                         // (^if confirmWa!t/mutex/progressDiag != nullptr)
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

            if(!action.errors()) progressDiag->close();
            else
            {
                deleteThread = false;
                connect(progressDiag, &ProgressDiag::accepted, this, &Thread::deleteLater);
                connect(progressDiag, &ProgressDiag::rejected, this, &Thread::deleteLater);

                                         // show Force button? (only as last resort)
                progressDiag->showResult(!action.forced() && !action.aborted() && !action.get(ThreadAction::Success)
                                         && action == ThreadAction::Unmount);

                progressDiag->appendStatus(action.aborted()                      ? d::lABORTED+"."
                                           : action.forced() || action.success() ? d::lDONE_
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
