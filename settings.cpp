#include "settings.h"
#include "ui_settings.h"
#include <QtWidgets>

using namespace std;

Settings::Settings(QWidget *parent, Config *newConfig) :
    QDialog(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    config = newConfig;
    utils = config->utils;

    connect(ui->dirBtn, SIGNAL(clicked()), this, SLOT(browseGame()));
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(save()));
}

void Settings::loadSettings()
{
    ui->dirEdit->setText(QString::fromStdString(config->getSetting("GamePath")));

    if(config->getSetting("hideEmptyMods") == "1")
        ui->hideEmptyCbx->setChecked(true);
    else
        ui->hideEmptyCbx->setChecked(false);
}

void Settings::save()
{
    QString qsGamePath = ui->dirEdit->text();
    QFileInfo fiGamePath(qsGamePath);
    if (!fiGamePath.exists() || !fiGamePath.isDir())
    {
        QMessageBox::warning(this, tr("Error"), tr("Warcraft III Folder: invalid folder."));
        return;
    }
    string sGamePath = qsGamePath.toStdString();
    utils->valueCorrect("GamePath", &sGamePath);
    config->setSetting("GamePath", sGamePath);

    string hideEmpty = ui->hideEmptyCbx->isChecked() ? "1" : "0";
    config->setSetting("hideEmptyMods", hideEmpty);

    config->saveConfig();
    accept();
}

void Settings::browseGame()
{
    QString qsFolder = ui->dirEdit->text();

    qsFolder = QFileDialog::getExistingDirectory(this, tr("Warcraft III Folder"),
                                                    qsFolder,
                                                    QFileDialog::ShowDirsOnly | QFileDialog::HideNameFilterDetails);

    if(qsFolder != "")
    {
        string sFolder = qsFolder.toStdString();
        utils->valueCorrect("GamePath", &sFolder);
        ui->dirEdit->setText(QString::fromStdString(sFolder));
    }
}

Settings::~Settings()
{
    delete ui;
}
