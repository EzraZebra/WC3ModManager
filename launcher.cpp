#include "_dic.h"
#include "thread.h"
#include "core.h"
#include "launcher.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>

Launcher::Launcher(Core *const core, const QString &_modName, const QString &version, const QString &args) : QObject(),
    editor(version == d::V_WE),
    args(args),
    core(core)
{
    core->setParent(this);

    core->showMsg(d::PROCESSING_ARGUMENTS___);

    bool doLaunch = true;

    if(!editor && (version == d::V_CLASSIC || version == d::V_EXPANSION)
               && (!core->setAllowOrVersion(version == d::V_EXPANSION, true)
                   && !confirmLaunch(d::FAILED_TO_SET_X_.arg(d::GAME_VERSION))))
        doLaunch = false;

    else if(!modName.isEmpty() && modName != d::L_NONE)
    {
        if(!core->setAllowOrVersion(true))
            doLaunch = !confirmLaunch(d::FAILED_TO_SET_X_.arg(d::ALLOW_FILES));
        else if(modName != core->cfg.getSetting(Config::kMounted))
        {
            const QFileInfo &fiMod(core->cfg.pathMods+"/"+modName);
            if(fiMod.isSymLink() || !fiMod.exists() || !fiMod.isDir())
                doLaunch = confirmLaunch(d::X_NOT_FOUND.arg("\""+modName+"\"")+".");
            else
            {
                modName = _modName;

                if(core->cfg.getSetting(Config::kMounted).isEmpty()) mountMod();
                else unmountMod();

                doLaunch = false;
            }
        }
    }
    else if(!core->setAllowOrVersion(false)) doLaunch = confirmLaunch(d::FAILED_TO_SET_X_.arg(d::ALLOW_FILES));

    if(doLaunch) launch();
}

void Launcher::close()
{
    core->showMsg(d::EXITING___, Msgr::Default);
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
    switch(core->mountMod(modName))
    {
    case Core::Mounted:      launch(); break;
    case Core::MountStarted: break;
    case Core::OtherMounted: unmountMod(); break;
    default:                 if(confirmLaunch(d::X_FAILED.arg(d::MOUNTING)+".")) launch();
    }
}

void Launcher::mountModReady(const ThreadAction &action)
{
    if(core->mountModReady(action)
       || confirmLaunch(QStringLiteral(u"%0: %1")
                            .arg(action.aborted() ? d::X_ABORTED.arg(d::MOUNTING)
                                                  : d::ERRORS_WHILE_X.arg(d::lMOUNTING),
                                 Core::a2s(action))))
        launch();
}

void Launcher::unmountMod()
{
    if(!core->unmountMod() && confirmLaunch(d::X_FAILED.arg(d::UNMOUNTING)+"."))
        launch();
}

void Launcher::unmountModReady(const ThreadAction &action)
{
    if(core->unmountModReady(action)) mountMod();
    else if(confirmLaunch(QStringLiteral(u"%0: %1")
                            .arg(action.aborted() ? d::X_ABORTED.arg(d::UNMOUNTING)
                                                  : d::X_FAILED.arg(d::UNMOUNTING),
                                 Core::a2s(action))))
      launch();
}
