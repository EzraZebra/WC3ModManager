#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>

class Msgr;
class Config;
class QLineEdit;
class QCheckBox;

class Settings : public QDialog
{
    Q_OBJECT

               QLineEdit *dirEdit;
               QCheckBox *hideEmptyCbx;

               Config &cfg;
               Msgr   *const msgr;

public:        Settings(QWidget *parent, Config &cfg, Msgr *const msgr);

private slots: void browseGame();
               void accept();
};

#endif // SETTINGS_H
