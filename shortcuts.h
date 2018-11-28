#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <QDialog>
#include <QButtonGroup>

class Msgr;
class IconSelect;
class QComboBox;
class QLineEdit;

class Shortcuts : public QDialog
{
    Q_OBJECT

               QComboBox    *modSelect;
               QLineEdit    *extraEdit, *resultEdit, *nameEdit, *locEdit;
               IconSelect   *iconSelect;
               QPushButton  *createBtn;
               QButtonGroup versionGroup, iconGroup;

               Msgr *const msgr;

               bool autoName = true, autoIcon = true;

               const std::array<const std::array<const int, 3>, 2>     mainIconsIndex;
               const std::vector<std::pair<const int, const QString> > icons;
               const QString gamePath;

public:        Shortcuts(QWidget *parent, const QStringList &modNames, const QString &gamePath, Msgr *const msgr);

private:       QString getArgs(QString *const modName=nullptr, int *const iIcon=nullptr);

private slots: void create();
               void shortcutReady();

               void updateResult(const int row=0, const bool filter=true);
               void browseLocation();

               void disableAutoName() { autoName = false; }
               void disableAutoIcon() { autoIcon = false; }

               static void openGuide();
};

#endif // SHORTCUTS_H
