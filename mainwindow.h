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

class ModNameItem : public QFrame
{
    Q_OBJECT

public: QLabel *icon;
        explicit ModNameItem(const QString &modName);
};

class ModDataItem : public QFrame
{
    Q_OBJECT

        const QString zero;

        QLabel *totalTitle, *mountedTitle;
public: QLabel *totalData,  *mountedData;

        explicit ModDataItem(const QString &zero, const bool alignRight=false);

        bool updateView(const QString &total, const QString &mounted);
        bool updateData(const QString &data, const bool isMounted);
};

class ModTable : public QTableWidget
{
    Q_OBJECT

public:       static const int modItemMrg = 2;

              md::modData modData;
              QStringList modNames;

              ModTable();

              ModDataItem* dataItem(const int row, const bool files) const
              { return dynamic_cast<ModDataItem *>(cellWidget(row, 1+files)); }

              ModNameItem* nameItem(const int row) const
              { return dynamic_cast<ModNameItem *>(cellWidget(row, 0)); }

              int row(const QString &modName) const
              { return md::exists(modData, modName) ? std::get<int(md::Row)>(modData.at(modName)) : -1; }
              void setSize(const QString &modName, const qint64 size)
              { std::get<int(md::Size)>(modData[modName]) = size; }

              bool tryBusy(const QString &modName);
              void setIdle(const QString &modName)
              { if(md::exists(modData, modName)) std::get<int(md::Busy)>(modData[modName]) = false; }

              void resizeCR(const int row=-1);
              bool modSelected() const { return currentRow() >= 0 && currentRow() < rowCount(); }

private:      void updateMod    (const QString &modName, const QString &modSize, const QString &fileCount,
                                 const qint64 size, const bool isMounted);
public slots: void addMod       (const QString &modName, const int row, const bool addData=false);
              void deleteMod    (const QString &modName);
              void updateTotal  (const QString &modName, const QString &modSize, const QString &fileCount, const qint64 size)
              { updateMod(modName, modSize, fileCount, size, false); }
              void updateMounted(const QString &modName, const QString &modSize, const QString &fileCount, const qint64 size)
              { updateMod(modName, modSize, fileCount, size, true); }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

               QAction      *launchGameAc, *launchEditorAc, *toggleMountAc;
               QCheckBox    *allowFilesCbx, *gameVersionCbx;
               QPushButton  *toggleMountBtn, *addModBtn, *refreshBtn;
               QLabel       *statusLbl=nullptr;

               Core *const core;
               Msgr msgr;
               ModTable *modTable;

               const std::array<const QIcon, 4> gameIcons;
               const std::array<const QIcon, 2> editIcons;

               int  scanCount=0;
               bool refreshing=false;

               QTableWidgetItem *renameModItem;
               QString           renameModName;

public:        explicit MainWindow(Core *const core);
               void show();

private slots: void showStatus(const QString &msg, const Msgr::Type &msgType=Msgr::Default);
private:       void showMsg   (const QString &msg, const Msgr::Type &msgType=Msgr::Default);

               bool modSelected();
               bool tryBusy(const QString &modName);

               void updateLaunchBtns();
               void updateAllowOrVersion(const bool version=false);
               void updateMountState(const QString &modName=QString());
private slots: void launchEditor();
               void setAllowOrVersion(const bool enable, const bool version=false);
               void setVersion(const bool enable){ setAllowOrVersion(enable, true); }

               void refresh(const bool silent=false);
               void modDataReady(const md::modData &modData, const QStringList &modNames);
               void scanModDone(const QString &modName);

               void mountMod();
               void unmountMod();
               void addMod();
               void deleteMod();
               void actionReady(const ThreadAction &action);

               void renameModAction();
               void renameModStart(QTableWidgetItem *item);
               void renameModSave (QTableWidgetItem *item);

               void openFolder(const QString &path, const QString &name, QString lName=QString());
               void openGameFolder();
               void openModsFolder();
               void openModFolder();

               void openShortcuts();
               void openSettings();
               void openAbout();
};

#endif // MAINWINDOW_H
