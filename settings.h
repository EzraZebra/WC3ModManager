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

private slots:
    void browseGame();
    void save();

public:
    explicit Settings(QWidget *parent = 0, Config* newConfig = 0);
    ~Settings();
    Config* config;
    Utils* utils;
    void loadSettings();

private:
    Ui::Settings *ui;
};

#endif // SETTINGS_H
