#include "settings.h"
#include "ui_settings.h"
#include "utils.h"

Settings::Settings(QWidget *parent, Config *newConfig) :
    QDialog(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    config = newConfig;

    connect(ui->dirBtn, SIGNAL(clicked()), this, SLOT(browseGame()));
    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(save(QAbstractButton *)));
}

void Settings::loadSettings()
{
    ui->dirEdit->setText(QString::fromStdString(config->getSetting("GamePath")));

    if(config->getSetting("hideEmptyMods") == "1") ui->hideEmptyCbx->setChecked(true);
    else ui->hideEmptyCbx->setChecked(false);
}

void Settings::save(QAbstractButton *btn)
{
    if(ui->buttonBox->standardButton(btn) == QDialogButtonBox::Apply)
    {
        QString qsGamePath = ui->dirEdit->text();
        QFileInfo fiGamePath(qsGamePath);
        if(!fiGamePath.exists() || !fiGamePath.isDir())
            QMessageBox::warning(this, tr("Error"), tr("Warcraft III Folder: invalid folder."));
        else
        {
            std::string sGamePath = qsGamePath.toStdString();
            utils::valueCorrect("GamePath", &sGamePath);
            config->setSetting("GamePath", sGamePath);

            config->setSetting("hideEmptyMods", ui->hideEmptyCbx->isChecked() ? "1" : "0");

            config->saveConfig();
            accept();
        }
    }
}

void Settings::browseGame()
{
    QString qsFolder = ui->dirEdit->text();

    qsFolder = QFileDialog::getExistingDirectory(this, tr("Warcraft III Folder"), qsFolder,
                                                 QFileDialog::ShowDirsOnly | QFileDialog::HideNameFilterDetails);

    if(qsFolder != "")
    {
        std::string sFolder = qsFolder.toStdString();
        utils::valueCorrect("GamePath", &sFolder);
        ui->dirEdit->setText(QString::fromStdString(sFolder));
    }
}

int Settings::exec()
{
    loadSettings();
    return QDialog::exec();
}

Settings::~Settings()
{
    delete ui;
}
