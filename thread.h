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

public:        Thread(const ThreadAction::Action &thrAction, const QString &modName,
                      const QString &pathMods, const QString &pathGame=QString(), Msgr *const msgr=nullptr);
               void start() { emit init(); }
 /* Unmount */ void start(const QString &modSize, const QString &fileCount)
               { emit init(0, modSize, fileCount); }
     /* Add */ void start(const QString &src, const QString &dst, const Mode &mode)
               { emit init(mode, src, dst); }

 /* ModData */ Thread(const ThreadAction::Action &thrAction, const QString &pathMods)
                   : Thread(thrAction, QString(), pathMods) {}
               void start(const mod_m &modData)
               { emit init(0, QString(), QString(), QString(), modData); }

/* Shortcut */ Thread(const ThreadAction::Action &thrAction, Msgr *const msgr)
                   : Thread(thrAction, QString(), QString(), QString(), msgr) {}
               void start(const QString &dst, const QString &args, const QString &iconPath, const int iconIndex)
               { emit init(iconIndex, dst, iconPath, args); }

               ~Thread();

signals:       void init(const int index=0, const QString &data1=QString(), const QString &data2=QString(),
                         const QString &args=QString(), const mod_m &modData={});

private slots: void abort();
               void pause();
               void processResult();
};

#endif // THREAD_H
