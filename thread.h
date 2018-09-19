#ifndef THREAD_H
#define THREAD_H

#include "mainwindow.h"
#include "filestatus.h"
#include <utility>
#include <QThread>

class Worker : public QObject
{
    Q_OBJECT
    std::pair<int, std::string> moveFile2(QString, QString, bool=false);
    Config *config;
    Utils *utils;

public:
    Worker(Config *);

public slots:
    void moveFolderWorker(QString, QString, bool=false, bool=false);
    void deleteFolderWorker(QString);
    void unmountModWorker(QString);

signals:
    void moveFolderReady(int, int, int);
    void deleteFolderReady(int, int, int);
    void unmountModReady(int, int, int);
    void status(QString, bool=false);
    void appendStatus(QString);
};

class Controller : public QObject
{
    Q_OBJECT

    QThread workerThread;
    MainWindow *mw;
    FileStatus *fileStatus;

public:
    Controller(MainWindow *mw, QString action, QString mod);
    ~Controller();
    Worker *worker;
    QString action;
    QString mod;

private slots:
    void result(int, int, int);
    void status(QString, bool=false);
    void appendStatus(QString);
    void cancel();

signals:
    void moveFolder(QString, QString, bool=false, bool=false);
    void deleteFolder(QString);
    void unmountMod(QString);
};

#endif // THREAD_H
