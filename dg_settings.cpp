#include "_dic.h"
#include "_msgr.h"
#include "config.h"
#include "dg_settings.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>

Settings::Settings(QWidget *parent, Config &cfg, Msgr *const msgr) : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint),
    cfg(cfg),
    msgr(msgr)
{
    setWindowTitle(d::SETTINGS);
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

        QFormLayout *formLayout = new QFormLayout;
        layout->addLayout(formLayout);

            QHBoxLayout *dirLayout = new QHBoxLayout;
            formLayout->addRow(d::X_uFOLDER.arg(d::WC3)+":", dirLayout);

                dirEdit = new QLineEdit;
                dirLayout->addWidget(dirEdit);
                dirEdit->setPlaceholderText(d::NO_X_X.arg(d::X_FOLDER.arg(d::lGAME), d::lSET));
                dirEdit->setText(QDir::toNativeSeparators(cfg.getSetting(Config::kGamePath)));
                QPushButton *dirBtn = new QPushButton(d::BROWSE___);
                dirLayout->addWidget(dirBtn);

            hideEmptyCbx = new QCheckBox(d::HIDE_EMPTY);
            formLayout->addRow(hideEmptyCbx);
            hideEmptyCbx->setChecked(cfg.getSetting(Config::kHideEmpty) == Config::vOn);


        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        layout->addWidget(buttonBox);

    connect(dirBtn,    &QPushButton::clicked,       this, &Settings::browseGame);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &Settings::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &Settings::reject);
}

void Settings::browseGame()
{
    QString path = dirEdit->text().simplified();

    path = QFileDialog::getExistingDirectory(this, d::X_uFOLDER.arg(d::WC3), path, QFileDialog::ShowDirsOnly);

    if(!path.isEmpty()) dirEdit->setText(QDir::toNativeSeparators(path));
}

void Settings::accept()
{
    emit msgr->msg(d::SAVING_SETTINGS___, Msgr::Busy);

    const QFileInfo &fiDir(dirEdit->text().simplified());
    if(fiDir.isSymLink() || !fiDir.exists() || !fiDir.isDir())
        emit msgr->msg(d::X_FOLDER.arg(d::WC3)+": "+d::lINVALID_X.arg(d::lFOLDER)+".", Msgr::Error);
    else
    {
        cfg.saveSetting(Config::kGamePath, dirEdit->text().simplified());
        cfg.saveSetting(Config::kHideEmpty, hideEmptyCbx->isChecked() ? Config::vOn : Config::vOff);
        cfg.saveConfig();

        emit msgr->msg(d::SETTINGS_SAVED_, Msgr::Default);
        QDialog::accept();
    }
}
