#ifndef SHORTCUTS_PVT_H
#define SHORTCUTS_PVT_H

#include <QPushButton>
#include <QDialog>
#include <QButtonGroup>

class Msgr;
class QVBoxLayout;
class QLineEdit;
class QScrollArea;
class QFrame;
class QGridLayout;

class IconButton : public QPushButton
{
    Q_OBJECT

public:  explicit IconButton(const QIcon &icon);
private: void mouseDoubleClickEvent(QMouseEvent *e);
signals: void doubleClicked();
};

class IconSelect : public QDialog
{
    Q_OBJECT

               QAbstractButton *btn;
               QVBoxLayout     *layout;
               QLineEdit       *browseEdit;
               QScrollArea     *scrollArea;
               QFrame          *frame;
               QGridLayout     *grid;
               QButtonGroup    btnGroup;

               Msgr *const msgr;

               QString loadedPath;
public:        QString selectedPath;
               int     selectedIndex;

               const int btnId;

               IconSelect(QWidget *parent, QAbstractButton *const btn, const int btnId, Msgr *const msgr);

private:       void setupUi();
               void newScrollArea();

private slots: void browseReturn();
               void accept();
               void getIcons(bool maybeBrowse, const bool browseMaybe=true);
};

#endif // SHORTCUTS_PVT_H
