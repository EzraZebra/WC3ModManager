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
    void moveFolderWorker(QString, QString, bool=false, bool=false);
    void deleteFolderWorker(QString);
    void unmountModWorker();

signals:
    void resultReady(QString, int, int, int);
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

public:
    Controller(MainWindow *mw, QString action, QString mod);
    ~Controller();
    Worker *worker;

private slots:
    void result(QString, int, int, int);
    void status(QString, bool=false);
    void appendStatus(QString);
    void cancel();

signals:
    void moveFolder(QString, QString, bool=false, bool=false);
    void deleteFolder(QString);
    void unmountMod();
};

#endif // THREAD_H
