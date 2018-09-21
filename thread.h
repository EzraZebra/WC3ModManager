#ifndef THREAD_H
#define THREAD_H

#include "mainwindow.h"
#include "filestatus.h"
#include <utility>
#include <QThread>

class Worker : public QObject
{
    Q_OBJECT

    std::pair<int, std::string> moveFile(QString, QString, bool=false);
    Config *config;
    Utils *utils;
    QString mod;

public:
    Worker(Config *, QString);

public slots:
    void scanModWorker(int);
    void moveFolderWorker(QString, QString, bool=false, bool=false);
    void unmountModWorker();
    void deleteModWorker();

signals:
    void resultReady(QString, int, int, int);
    void scanModUpdate(int, QString, QString);
    void status(QString, bool=false);
    void appendStatus(QString);
};

class Controller : public QObject
{
    Q_OBJECT

    QThread workerThread;
    MainWindow *mw;
    FileStatus *fileStatus;
    QString action;
    QString mod;
    bool showStatus;

public:
    Controller(MainWindow *, QString, QString, bool=true);
    ~Controller();
    Worker *worker;

private slots:
    void result(QString, int, int, int);
    void status(QString, bool=false);
    void appendStatus(QString);
    void cancel();

signals:
    void scanMod(int);
    void moveFolder(QString, QString, bool=false, bool=false);
    void deleteMod();
    void unmountMod();
};

#endif // THREAD_H
