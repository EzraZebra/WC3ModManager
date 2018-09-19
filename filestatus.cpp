#include "filestatus.h"
#include "ui_filestatus.h"

FileStatus::FileStatus(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileStatus)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowCloseButtonHint, false);

    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(cancel()));
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

void FileStatus::result(QString msg)
{
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok);
    ui->msgLbl->setText(ui->msgLbl->text()+" done.");
    ui->infoLbl->setText(msg);
    setWindowFlag(Qt::WindowCloseButtonHint);
}

void FileStatus::cancel()
{
    emit rejected();
}

FileStatus::~FileStatus()
{
    delete ui;
}
