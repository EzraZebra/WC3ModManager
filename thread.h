#ifndef THREAD_H
#define THREAD_H

#include "_moddata.h"
#include "threadbase.h"
#include <QThread>

class Msgr;
class ProgressDiag;
class ThreadWorker;

class Thread : public ThreadBase
{
    Q_OBJECT

               QThread        workerThread;
               ProgressDiag   *progressDiag = nullptr;
               ThreadAction   action;
               bool paused=false;

protected:     Thread(const ThreadAction::Action &thrAction, const QString &modName,
                      const QString &pathMods, const QString &pathGame=QString(), Msgr *const msgr=nullptr);

               // ModData
               Thread(const ThreadAction::Action &thrAction, const QString &pathMods)
                   : Thread(thrAction, QString(), pathMods, QString()) {}
               // Shortcut
               Thread(const ThreadAction::Action &thrAction, Msgr *const msgr)
                   : Thread(thrAction, QString(), QString(), QString(), msgr) {}

               ~Thread();

signals:       void init(const int index=0, const QString &path1=QString(), const QString &path2=QString(),
                         const QString &args=QString(), const mod_m &modData={});

private slots: void abort();
               void pause();
               void processResult();
};

/************************************/
/**** PUBLIC THREAD CONSTRUCTORS ****/
/************************************/
    /**** MOUNT ****/
    class ThreadMount : public Thread
    {
        Q_OBJECT

    public: ThreadMount(const QString &modName, const QString &pathMods, const QString &pathGame)
                : Thread(ThreadAction::Mount, modName, pathMods, pathGame) {}
            void init() { emit Thread::init(); }
    };

    /**** UNMOUNT ****/
    class ThreadUnmount : public Thread
    {
        Q_OBJECT

    public: ThreadUnmount(const QString &modName, const QString &pathMods, const QString &pathGame)
                : Thread(ThreadAction::Unmount, modName, pathMods, pathGame) {}
            void init() { emit Thread::init(); }
    };

    /**** MOD DATA ****/
    class ThreadModData : public Thread
    {
        Q_OBJECT

            const mod_m modData;
    public: ThreadModData(const QString &pathMods, const mod_m &modData)
                : Thread(ThreadAction::ModData, pathMods),
                  modData(modData) {}
            void init() { emit Thread::init(0, QString(), QString(), QString(), modData); }
    };

    /**** DELETE ****/
    class ThreadDelete : public Thread
    {
        Q_OBJECT

    public: ThreadDelete(const QString &modName, const QString &pathMods, const QString &pathGame)
                : Thread(ThreadAction::Delete, modName, pathMods, pathGame) {}
            void init() { emit Thread::init(); }
    };

    /**** ADD ****/
    class ThreadAdd : public Thread
    {
        Q_OBJECT

            const QString src, dst;
            const Mode mode;
    public: ThreadAdd(const QString &modName, const QString &src, const QString &dst, const Mode &mode,
                      const QString &pathMods, const QString &pathGame)
                : Thread(ThreadAction::Add, modName, pathMods, pathGame),
                  src(src), dst(dst), mode(mode) {}
            void init() { emit Thread::init(mode, src, dst); }
    };

    /**** SCAN ****/
    class ThreadScan : public Thread
    {
        Q_OBJECT

    public: ThreadScan(const QString &modName, const QString &pathMods, const bool mounted=false)
                : Thread(mounted ? ThreadAction::ScanMounted : ThreadAction::Scan, modName, pathMods) {}
            void init() { emit Thread::init(); }
    };

    /**** SHORTCUT ****/
    class ThreadShortcut : public Thread
    {
        Q_OBJECT

            const QString dst, args, iconPath;
            const int iconIndex;
    public: ThreadShortcut(const QString &dst, const QString &args,
                           const QString &iconPath, const int iconIndex, Msgr *const msgr)
                : Thread(ThreadAction::Shortcut, msgr),
                  dst(dst), args(args), iconPath(iconPath), iconIndex(iconIndex) {}
            void init() { emit Thread::init(iconIndex, dst, iconPath, args); }
    };

#endif // THREAD_H
