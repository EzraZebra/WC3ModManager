#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "_msgr.h"
#include "_moddata.h"
#include <QMainWindow>
#include <QTableWidget>
#include <set>
#include <tuple>

class Core;
class ThreadAction;
class QCheckBox;
class QPushButton;
class QLabel;
class QTableWidgetItem;

#include <QDebug>

class ModTable : public QTableWidget
{
    Q_OBJECT

public:        md::modData modData;
               QStringList modNames;

               ModTable();

               QLabel* cellLabel(const int row, const int column) const;

               bool modSelected() const { return currentRow() >= 0 && currentRow() < rowCount(); }

               int row(const QString &modName) const
               { return md::exists(modData, modName) ? std::get<int(md::Row)>(modData.at(modName)) : -1; }
               void setSize(const QString &modName, const qint64 size)
               { std::get<int(md::Size)>(modData[modName]) = size; }
               void setIdle(const QString &modName)
               { if(md::exists(modData, modName)) std::get<int(md::Busy)>(modData[modName]) = false; }

public slots:  void updateMod(const QString &modName, const QString &modSize, const QString &fileCount, const qint64 size);
               void addMod   (const QString &modName, const int row, const bool addData=false);
               void deleteMod(const QString &modName);
               void renameMod(const QString &modName, const QString &newName);

               void resizeCR(const int row=-1);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

               QAction      *launchGameAc, *launchEditorAc, *toggleMountAc;
               QCheckBox    *allowFilesCbx, *gameVersionCbx;
               QPushButton  *toggleMountBtn, *addModBtn, *refreshBtn;
               QDialog      *renameDg;
               QLineEdit    *renameEdit;
               QLabel       *statusLbl=nullptr;

               Core *const core;
               Msgr msgr;
               ModTable *modTable;

               const std::array<const QIcon, 4> gameIcons;
               const std::array<const QIcon, 2> editIcons;

               int  scanCount=0;
               bool refreshing=false;

public:        explicit MainWindow(Core *const core);
               void show();

private slots: void showStatus(const QString &msg, const Msgr::Type &msgType=Msgr::Default);
private:       void showMsg   (const QString &msg, const Msgr::Type &msgType=Msgr::Default);

               bool tryBusy(const QString &modName);
               bool isExternal(const QString &modName);

               void updateLaunchBtns();
               void updateAllowOrVersion(const bool version=false);
               void updateMountState(const QString &modName=QString(), const bool enableBtn=true);
private slots: void launchEditor();
               void setAllowOrVersion(const bool enable, const bool version=false);
               void setVersion(const bool enable){ setAllowOrVersion(enable, true); }

               void refresh(const bool silent=false);
               void scanMods(const md::modData &modData, const QStringList &modNames);
               void scanModDone(const QString &modName);

               void mountMod();
               void unmountMod();
               void addMod();
               void deleteMod();
               void actionDone(const ThreadAction &action);

               void renameMod();
               void renameModSave();
               void renameModDone();

               void openFolder(const QString &path, const QString &name, QString lName=QString());
               void openGameFolder();
               void openModsFolder();
               void openModFolder();

               void openShortcuts();
               void openSettings();
               void openAbout();
};

#endif // MAINWINDOW_H

/********************************************************************/
/*      OBSOLETE - may be useful later      *************************/
/********************************************************************

    class ModDataItem : public QFrame
    {
        Q_OBJECT

                  const QString zero;
                  const int row;
                  bool detailsVisible = false;
                  QTimer *timer;

                  QLabel *totalTitle, *mountedTitle;
    public:       QLabel *totalData,  *mountedData;

                  explicit ModDataItem(const int row, const QString &zero, const bool alignRight=false);

                  void updateData(const QString &data, const bool isMounted);
    public slots: void updateView();
    signals:      void viewUpdated(const int row);
    }
 ********************************************************************/
/********************************************************************/
/********************************************************************/
