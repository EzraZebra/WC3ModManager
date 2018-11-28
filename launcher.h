#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QObject>

class Core;
class ThreadAction;

class Launcher : public QObject
{
    Q_OBJECT

               const bool    editor;
               const QString args;
               QString       modName;

               Core *const core;

public:        Launcher(Core *const core, const QString &modName, const QString &version, const QString &args);
private:       void close();

               void launch();
               bool confirmLaunch(const QString &msg);

               void scanMod();
               void unmountMod();
private slots: void mountMod();
               void mountModReady  (const ThreadAction &action);
               void unmountModReady(const ThreadAction &action);
};

#endif // LAUNCHER_H
