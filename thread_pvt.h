#ifndef THREAD_PVT_H
#define THREAD_PVT_H

#include "_moddata.h"
#include "threadbase.h"
#include <QDialog>
#include <QMutex>
#include <QCoreApplication>
#include <QFileInfo>
#include <fstream>

#include <QDebug>

class Msgr;
class QLabel;
class QDialogButtonBox;
class QPlainTextEdit;
class QWaitCondition;

class ProgressDiag : public QDialog
{
    Q_OBJECT

               QLabel           *statusLbl, *infoLbl;
               QDialogButtonBox *buttonBox;
               QPushButton      *forceBtn = nullptr;
               QPlainTextEdit   *errorTxt = nullptr;

               QWaitCondition *confirmWait;

               bool doAbort = true;
               const QString status;

public:        ProgressDiag(QWidget *parent, const QString &status, QWaitCondition *confirmWait);

               void disableAbort() { doAbort = false; }

               void showResult(const bool enableForce);
public slots:  void showProgress(const QString &msg, const bool error=false);
               void appendStatus(const QString &msg);

private slots: void reject();
               void forceUnmount();
signals:       void aborted();
               void interrupted();
               void unmountForced();
};

class ThreadWorker : public ThreadBase
{
    Q_OBJECT

              static const QString extBackup;

              const QString     pathMods, pathGame;
              const std::string pathOutFiles    = QCoreApplication::applicationDirPath().toStdString()+"/out_files.txt",
                                pathBackupFiles = QCoreApplication::applicationDirPath().toStdString()+"/backup_files.txt";
              std::ofstream     outFilesIt, backupFilesIt;

              Msgr         *const msgr;
              ThreadAction &action;

              bool &paused;
              qint64 modSize=0;
              int fileCount=0;

public:       ThreadWorker(ThreadAction &action, bool &paused, const QString &pathMods, const QString &pathGame, Msgr *const msgr)
                : ThreadBase(),
                   pathMods(pathMods), pathGame(pathGame),
                   msgr(msgr), action(action),
                   paused(paused) {}

              void setWaitConditionEx(QWaitCondition *waitCond, QMutex *mutex)
              { confirmWait = waitCond; this->mutex = mutex; }

public slots: void init(const int index=0, const QString &data1=QString(), const QString &data2=QString(),
                        const QString &args=QString(), const mod_m &modData={});

              void forceUnmount();

private:      static QString B2MB(double size);

              ThreadAction::Result processFile(const QString &src, const QString &dst,
                                               const Mode &mode=Move, const bool logBackups=false);
              ThreadAction::Result deleteFile(const QString &path, const QString &stopPath=QString());
              void                 removePath(const QString &path, const QString &stopPath=QString());

              void modDataWorker(const mod_m &modData);
              void scanFile(const QFileInfo &fi, const bool subtract=false);
              void scanPath(const QString &path, const bool subtract=false);
              void scanModWorker();
              void scanMountedModWorker();
              void checkState();
              void mountModIterator(QString relativePath=QString());
              void mountModWorker();
              void unmountModWorker(QString modSize=QString(), QString fileCount=QString());
              void addModWorker(const QString &src, const QString &dst, const Mode &mode);
              void deleteModWorker();
              void shortcutWorker(const QString &dst, const QString &args, const QString &iconPath, const int iconIndex);

signals:      void progressUpdate(const QString &msg, const bool error=false);
              void statusUpdate  (const QString &msg);
};

#endif // THREAD_PVT_H
