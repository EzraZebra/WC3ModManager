#include "filestatus.h"
#include "ui_filestatus.h"
#include <QPushButton>
#include <QMessageBox>

FileStatus::FileStatus(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileStatus)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowCloseButtonHint, false);
    ui->okBtn->hide();
    ui->forceBtn->hide();

    connect(ui->abortBtn, SIGNAL(clicked()), this, SLOT(abort()));
}

void FileStatus::setText(QString msg)
{
    ui->msgLbl->setText(msg);
}

void FileStatus::setInfoText(QString msg)
{
    ui->infoLbl->setText(msg);
}

void FileStatus::addErrorText(QString msg)
{
    if(ui->detailsTxt->height() == 0)
    {
        setMinimumHeight(minimumHeight()+90);
        ui->detailsTxt->setMinimumHeight(90);
        ui->detailsTxt->setMaximumHeight(QWIDGETSIZE_MAX);
    }
    ui->detailsTxt->moveCursor(QTextCursor::End);
    ui->detailsTxt->insertPlainText(msg+"\n");
}

void FileStatus::result(bool enableForce)
{
    setWindowFlag(Qt::WindowCloseButtonHint);

    disconnect(ui->abortBtn, SIGNAL(clicked()), this, SLOT(abort()));
    ui->abortBtn->hide();

    connect(ui->okBtn, SIGNAL(clicked()), this, SLOT(accept()));
    ui->okBtn->show();

    if(enableForce)
    {
        connect(ui->forceBtn, SIGNAL(clicked()), this, SLOT(forceUnmountClicked()));
        ui->forceBtn->show();
    }
    else
    {
        disconnect(ui->forceBtn, SIGNAL(clicked()), this, SLOT(forceUnmountClicked()));
        ui->forceBtn->hide();
    }
}

void FileStatus::abort()
{
    emit rejected();
}

void FileStatus::forceUnmountClicked()
{
    if(QMessageBox::warning(this, "Force unmount?", "Are you sure you want to remove all record of mounted files and backups?",
                            QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
        emit forceUnmount(true);
}

FileStatus::~FileStatus()
{
    delete ui;
}
