#ifndef FILESTATUS_H
#define FILESTATUS_H

#include <QDialog>

namespace Ui {
class FileStatus;
}

class FileStatus : public QDialog
{
    Q_OBJECT

    Ui::FileStatus *ui;

public:
    explicit FileStatus(QWidget *parent = nullptr);
    ~FileStatus();
    void setText(QString);
    void setInfoText(QString);
    void addErrorText(QString);
    void result(bool=false);

private slots:
    void abort();
    void forceUnmountClicked();

signals:
    void forceUnmount(bool=true);
};

#endif // FILESTATUS_H
