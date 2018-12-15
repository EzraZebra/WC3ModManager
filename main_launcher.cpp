#include "_dic.h"
#include "thread.h"
#include "main_core.h"
#include "main_launcher.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>

Launcher::Launcher(Core *const core, const QString &_modName, const QString &version, const QString &args) : QObject(),
    editor(version == d::V_WE), args(args), core(core)
{
    core->setParent(this);

    core->showMsg(d::PROCESSING_ARGUMENTS___, Msgr::Busy);

    bool doLaunch = true;

    if(!editor && (version == d::V_CLASSIC || version == d::V_EXPANSION)
               && (!core->setAllowOrVersion(version == d::V_EXPANSION, true)
                   && !confirmLaunch(d::FAILED_TO_SET_X_.arg(d::GAME_VERSION))))
        doLaunch = false;

    else if(!_modName.isEmpty() && _modName != d::L_NONE && _modName != core->mountedMod)
    {
        const QFileInfo &fiMod(core->cfg.pathMods+"/"+_modName);
        if(fiMod.isSymLink() || !fiMod.exists() || !fiMod.isDir())
            doLaunch = confirmLaunch(d::X_NOT_FOUND.arg("\""+_modName+"\"")+".");
        else
        {
            modName = _modName;

            if(core->mountedMod.isEmpty()) mountMod();
            else unmountMod();

            doLaunch = false;
        }
    }

    if(doLaunch) launch();
}

void Launcher::close()
{
    core->showMsg(d::EXITING___, Msgr::Busy);
    core->closeSplash();
    deleteLater();
}

void Launcher::launch()
{
    core->launch(editor, args);
    close();
}

bool Launcher::confirmLaunch(const QString &msg)
{
    if(QMessageBox::warning(QApplication::activeModalWidget(), d::LAUNCH_X.arg(d::lGAME)+"?",
                            (msg.isEmpty() ? QStringLiteral(u"%0")
                                           : QStringLiteral(u"%0\n%1").arg(msg))
                                .arg(d::CONTINUE_LAUNCHING_GAMEq),
                            QMessageBox::Yes|QMessageBox::No)
            == QMessageBox::Yes) return true;
    else close();
    return false;
}

void Launcher::mountMod()
{
    switch(core->mountModCheck(modName))
    {
    case Core::Mounted: launch(); break;
    case Core::MountReady:
    {
        Thread *thr = core->mountModThread(modName);
        connect(thr, &Thread::resultReady, this, &Launcher::mountModDone);
        thr->start();
        break;
    }
    case Core::MountFailed:
        if(confirmLaunch(d::X_FAILED.arg(d::MOUNTING)+".")) launch();
        break;
    case Core::OtherMounted: unmountMod();
    }
}

void Launcher::mountModDone(const ThreadAction &action)
{
    if(core->actionDone(action)
       || confirmLaunch(QStringLiteral(u"%0: %1")
                            .arg(action.aborted() ? d::X_ABORTED.arg(d::MOUNTING)
                                                  : d::ERRORS_WHILE_X.arg(d::lMOUNTING),
                                 Core::a2s(action))))
        launch();
}

void Launcher::unmountMod()
{
    if(core->unmountModCheck())
    {
        Thread *thr = core->unmountModThread();
        connect(thr, &Thread::resultReady, this, &Launcher::unmountModDone);
        thr->start();
    }
    else if(confirmLaunch(d::X_FAILED.arg(d::UNMOUNTING)+"."))
        launch();
}

void Launcher::unmountModDone(const ThreadAction &action)
{
    if(core->actionDone(action)) mountMod();
    else if(confirmLaunch(QStringLiteral(u"%0: %1")
                            .arg(action.aborted() ? d::X_ABORTED.arg(d::UNMOUNTING)
                                                  : d::X_FAILED.arg(d::UNMOUNTING),
                                 Core::a2s(action))))
      launch();
}
