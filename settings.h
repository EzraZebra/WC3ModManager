#ifndef SETTINGS_H
#define SETTINGS_H

#include "config.h"
#include <QDialog>

namespace Ui {
class Settings;
}

class Settings : public QDialog
{
    Q_OBJECT

    Ui::Settings *ui;
    Config *config;

public:
    explicit Settings(QWidget *parent = nullptr, Config *newConfig = nullptr);
    ~Settings();
    void loadSettings();

private slots:
    void browseGame();
    void save();
};

#endif // SETTINGS_H
