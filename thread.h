#ifndef THREAD_H
#define THREAD_H

#include "_moddata.h"
#include "threadbase.h"
#include <QThread>

class Msgr;
class ProgressDiag;

class Thread : public ThreadBase
{
    Q_OBJECT

               QThread        workerThread;
               ProgressDiag   *progressDiag = nullptr;
               ThreadAction   action;
               bool paused=false;

public:        Thread(const ThreadAction::Action &thrAction, const QString &modName, // Scan, Mount, Unmount, Add, Delete
                      const QString &pathMods, const QString &pathGame=QString(), Msgr *const msgr=nullptr);
               Thread(const ThreadAction::Action &thrAction, const QString &modName) // ScanEx
                   : Thread(thrAction, modName, QString()) {}
               Thread(const ThreadAction::Action &thrAction, Msgr *const msgr)       // Shortcut
                   : Thread(thrAction, QString(), QString(), QString(), msgr) {}
               
               ~Thread();

               void start() { emit init(); }                                                                      // Scan, Mount, Unmount
               void start(const md::modData &modData, const QString &mountedMod)                                  // ModData
               { emit init(0, mountedMod, QString(), QString(), modData); }
               void start(const QString &modPath) { emit init(0, modPath); }                                      // ScanEx
               void start(const QString &src, const QString &dst, const bool copy) { emit init(copy, src, dst); } // Add
               void start(const qint64 size, const QString &fileCount) { emit init(size, fileCount); }            // Delete
               void start(const QString &dst, const QString &args, const QString &iconPath, const int iconIndex)  // Shortcut
               { emit init(iconIndex, dst, iconPath, args); }

signals:       void init(const qint64 index=0, const QString &data1=QString(), const QString &data2=QString(),
                         const QString &args=QString(), const md::modData &modData={});

private slots: void abort();
               void pause();
               void processResult();
};

#endif // THREAD_H
