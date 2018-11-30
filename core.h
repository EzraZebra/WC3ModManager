#ifndef CORE_H
#define CORE_H

#include "_msgr.h"
#include "config.h"

#include <QPixmap>

class ThreadAction;
class Thread;
class ThreadUnmount;
class QSplashScreen;

class Core : public QObject
{
    Q_OBJECT

public:        enum MountResult { MountReady, Mounted, MountFailed, OtherMounted };

               static const wchar_t *regAllowFiles, *regGameVersion; //registry

               const QPixmap pxLogo    = QPixmap(":/img/logo.png"),
                             pxVersion = QPixmap(":/img/version.png");
private:       QSplashScreen *splashScreen;
               QWidget *finishWgt=nullptr;

public:        Config cfg;

               Core();
               ~Core();

               void closeSplash(QWidget *finish=nullptr);
private slots: void closeSplashTimed();
signals:       void msg    (const QString &msg, const Msgr::Type &msgType=Msgr::Default);
public:        void showMsg(const QString &msg, const Msgr::Type &msgType=Msgr::Default, const bool propagate=true);

public slots:  void launch(const bool editor=false, const QString &args=QString());

public:        bool setAllowOrVersion(const bool enable, const bool version=false);

               MountResult mountModCheck(const QString &modName);
               Thread*     mountModThread(const QString &modName);
               bool        mountModReady(const ThreadAction &action);
               bool        unmountModCheck();
               Thread*     unmountModThread();
               bool        unmountModReady(const ThreadAction &action);

               static QString a2s(const ThreadAction &action);
private:       static QString a2e(const ThreadAction &action);
};

#endif // CORE_H
