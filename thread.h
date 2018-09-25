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
    void removePath(QString, QString="");
    Config *config;
    Utils *utils;
    QString mod;

public:
    Worker(Config *, QString);
    bool abort = false;

public slots:
    void scanModWorker(int);
    void moveFolderWorker(QString, QString, bool=false, bool=false);
    void unmountModWorker(bool=false);
    void deleteModWorker();

signals:
    void resultReady(QString, int, int, int, bool, bool=false);
    void scanModUpdate(int, QString, QString);
    void status(QString, bool=false);
    void appendAction(QString);
};

class Controller : public QObject
{
    Q_OBJECT

    QThread workerThread;
    MainWindow *mw;
    QString action;
    QString mod;
    bool showStatus;

public:
    Controller(MainWindow *, QString, QString, bool=true);
    ~Controller();
    Worker *worker;
    FileStatus *fileStatus;

private slots:
    void result(QString, int, int, int, bool, bool=false);
    void status(QString, bool=false);
    void appendAction(QString);
    void abort();

signals:
    void scanMod(int);
    void moveFolder(QString, QString, bool=false, bool=false);
    void deleteMod();
    void unmountMod(bool=false);
};

#endif // THREAD_H
