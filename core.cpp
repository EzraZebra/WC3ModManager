#include "_dic.h"
#include "thread.h"
#include "core.h"

#include <QSplashScreen>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QFileInfo>

#include <winerror.h>

#include <QDebug>

const wchar_t *Core::regAllowFiles  = L"Allow Local Files",
              *Core::regGameVersion = L"Preferred Game Version";

Core::Core() : QObject(),
    splashScreen(new QSplashScreen(pxLogo))
{
    qRegisterMetaType<ThreadAction>("ThreadAction");

    splashScreen->setAttribute(Qt::WA_DeleteOnClose);

    QLabel *vrsLbl = new QLabel(splashScreen);
    vrsLbl->setFixedSize(350, 13);
    vrsLbl->move(0, 80);
    vrsLbl->setPixmap(pxVersion);
    vrsLbl->setAlignment(Qt::AlignHCenter);

    showMsg(d::STARTING___, Msgr::Busy);
    splashScreen->show();
}

Core::~Core()
{
    if(splashScreen)
    {
        if(splashScreen->isVisible()) closeSplashTimed();
        else splashScreen->deleteLater(); // In case it didn't get deleted when close()d
        splashScreen = nullptr;
    }
}

void Core::closeSplash(QWidget *finish)
{
    if(splashScreen)
    {
        QCoreApplication::processEvents();

        finishWgt = finish;
        QTimer::singleShot(2000-bool(finish)*1000, this, SLOT(closeSplashTimed()));

        QCoreApplication::processEvents();
    }
}

void Core::closeSplashTimed()
{
    if(finishWgt)
    {
        splashScreen->finish(finishWgt);
        finishWgt = nullptr;
    }
    else splashScreen->close();
    splashScreen = nullptr;
}

void Core::showMsg(const QString &_msg, const Msgr::Type &msgType, const bool propagate)
{
    if(propagate) emit msg(_msg, msgType);

    if(splashScreen)
    {
        splashScreen->raise();
        QCoreApplication::processEvents();
        splashScreen->showMessage(_msg, Qt::AlignHCenter|Qt::AlignBottom, "#bbb");
    }

    switch(msgType)
    {
    default:
        break;
    case Msgr::Info:     QMessageBox::information(QApplication::activeModalWidget(), d::INFO, _msg);
        break;
    case Msgr::Error:    QMessageBox::warning(QApplication::activeModalWidget(), d::dERROR, _msg);
        break;
    case Msgr::Critical: QMessageBox::critical(QApplication::activeModalWidget(), d::dERROR, _msg);
    }
}

void Core::launch(const bool editor, const QString &args)
{

    bool success;
    const QString &exe = editor ? d::WE_EXE : d::WC3_EXE;

    showMsg(d::LAUNCHING_X___.arg(editor ? d::WE : d::GAME), Msgr::Busy);

    if(editor || args.isEmpty())
        success = QDesktopServices::openUrl(QUrl::fromLocalFile(cfg.getSetting(Config::kGamePath)+"/"+exe));
    else
    {
        QProcess war3;
        war3.setProgram(cfg.getSetting(Config::kGamePath)+"/"+exe);
        if(!editor) war3.setNativeArguments(args);
        success = war3.startDetached();
    }

    if(success) showMsg(d::X_LAUNCHED_.arg(editor ? d::WE : d::GAME));
    else showMsg(d::FAILED_TO_X_.arg(d::lLAUNCH_X).arg(exe), Msgr::Error);
}

bool Core::setAllowOrVersion(const bool enable, const bool version)
{
    HKEY hKey;
    if(Config::regOpenWC3(KEY_ALL_ACCESS, hKey))
    {
        DWORD value(enable);
        if(RegSetValueEx(hKey, version ? regGameVersion : regAllowFiles, 0, REG_DWORD,
                         reinterpret_cast<const BYTE*>(&value), sizeof(value))
                == ERROR_SUCCESS)
        {
            showMsg((enable ? d::X_ENABLED_ : d::X_DISABLED_).arg(version ? d::EXPANSION : d::ALLOW_FILES));
            RegCloseKey(hKey);
            return true;
        }
        else showMsg(d::FAILED_TO_SET_X_.arg(version ? d::GAME_VERSION : d::ALLOW_FILES), Msgr::Error);
    }
    else showMsg(d::FAILED_TO_OPEN_REGK_, Msgr::Error);

    RegCloseKey(hKey);
    return false;
}

Core::MountResult Core::mountMod(const QString &modName, const bool startThread)
{
    if(!cfg.getSetting(Config::kMounted).isEmpty())
    {
        if(cfg.getSetting(Config::kMounted) == modName) return Mounted;
        else
        {
            showMsg(d::ALREADY_MOUNTEDc_X_.arg(cfg.getSetting(Config::kMounted)), Msgr::Error);
            return OtherMounted;
        }
    }
    else
    {
        const QFileInfo &fiGamePath(cfg.getSetting(Config::kGamePath));
        if(fiGamePath.isSymLink() || !fiGamePath.exists() || !fiGamePath.isDir())
        {
            showMsg(d::INVALID_X.arg(d::X_FOLDER).arg(d::WC3)+".", Msgr::Error);
            return MountFailed;
        }
        else if(startThread)
        {
            mountModThread(modName)->init();
            return MountStarted;
        }
        else return MountReady;
    }
}

ThreadMount* Core::mountModThread(const QString &modName)
{
    showMsg(d::MOUNTING_X___.arg(modName), Msgr::Busy);

    cfg.saveSetting(Config::kMounted, modName);
    cfg.saveConfig();

    ThreadMount *thr = new ThreadMount(modName, cfg.pathMods, cfg.getSetting(Config::kGamePath));
    connect(thr, SIGNAL(resultReady(const ThreadAction&)), parent(), SLOT(mountModReady(const ThreadAction&)));
    return thr;
}

bool Core::mountModReady(const ThreadAction &action)
{
    if(action.success())
    {
        if(action.errors())
        {
            cfg.saveSetting(Config::kMountedError, a2e(action));
            cfg.saveConfig();
        }
    }
    else
    {
        cfg.deleteSetting(Config::kMounted);
        cfg.deleteSetting(Config::kMountedError);
        cfg.saveConfig();
    }

    showMsg(a2s(action));

    return action.success() && !action.errors();
}

bool Core::unmountMod(const bool startThread)
{
    if(!cfg.getSetting(Config::kMounted).isEmpty())
    {
        if(startThread) unmountModThread()->init();

        return true;
    }
    else
    {
        showMsg(d::NO_MOD_X_.arg(d::lMOUNTED), Msgr::Error);

        return false;
    }
}

ThreadUnmount* Core::unmountModThread()
{
    showMsg(d::UNMOUNTING_X___.arg(cfg.getSetting(Config::kMounted)), Msgr::Busy);

    ThreadUnmount *thr = new ThreadUnmount(cfg.getSetting(Config::kMounted), cfg.pathMods, cfg.getSetting(Config::kGamePath));
    connect(thr, SIGNAL(resultReady(const ThreadAction&)), parent(), SLOT(unmountModReady(const ThreadAction&)));

    return thr;
}

bool Core::unmountModReady(const ThreadAction &action)
{
    if(!action.success())
    {
        QString errorMsg = a2e(action);
        if(!cfg.getSetting(Config::kMountedError).isEmpty()) errorMsg = cfg.getSetting(Config::kMountedError)+";"+errorMsg;
        cfg.saveSetting(Config::kMountedError, errorMsg);
    }
    else
    {
        cfg.deleteSetting(Config::kMounted);
        cfg.deleteSetting(Config::kMountedError);
    }

    cfg.saveConfig();

    showMsg(a2s(action));

    return action.success();
}

QString Core::a2s(const ThreadAction &action)
{
    const d::ac_t &dac = d::ac.find(action.PROCESSING)->second;
    return QStringLiteral(u"%0.%1").arg(
       /* %0 */ action.aborted()           ? d::X_ABORTED.arg(action.PROCESSING+" "+action.modName+": ")
                : !action.filesProcessed() ? d::NO_FILES_TO_X.arg(dac[size_t(d::Ac::lPROCESS)])
                : action.success()         ? dac[size_t(d::Ac::X_PROCESSED)].arg(action.modName)
                                            : d::FAILED_TO_X.arg(dac[size_t(d::Ac::lPROCESS_X)]).arg(action.modName),

       /* %1 */ action.errors() ? QStringLiteral(u" [%0%1%2]")
                                    .arg(dac[size_t(d::Ac::X_PROCESSED)].arg(d::X_FILES).arg(action.get(ThreadAction::Success)),    // %0
                                         action.get(ThreadAction::Failed)
                                            ? ", "+d::X_FAILED.arg(d::X_FILES).arg(action.get(ThreadAction::Failed)) : QString(),   // %1
                                         action.get(ThreadAction::Missing)
                                            ? ", "+d::X_MISSING.arg(d::X_FILES).arg(action.get(ThreadAction::Missing)) : QString()) // %2
                                : QString());
}

QString Core::a2e(const ThreadAction &action)
{
    const d::ac_t &dac = d::ac.find(action.PROCESSING)->second;
    return QStringLiteral(u"%0: %1%2%3").arg(
   /* %0 */   action.aborted()           ? d::X_ABORTED.arg(action.PROCESSING)
              : action.forced()          ? d::FORCE_X.arg(dac[size_t(d::Ac::lPROCESS)])
              : !action.filesProcessed() ? d::NO_FILES_TO_X.arg(dac[size_t(d::Ac::lPROCESS)])
              : action.success()         ? dac[size_t(d::Ac::PROCESSED)]
                                          : d::X_FAILED.arg(action.PROCESSING),
   /* %1 */   dac[size_t(d::Ac::X_PROCESSED)].arg(d::X_FILES).arg(action.get(ThreadAction::Success)),
   /* %2 */   action.get(ThreadAction::Failed)  ? ", "+d::X_FAILED.arg(d::X_FILES).arg(action.get(ThreadAction::Failed))   : QString(),
   /* %3 */   action.get(ThreadAction::Missing) ? ", "+d::X_MISSING.arg(d::X_FILES).arg(action.get(ThreadAction::Missing)) : QString());
}
