#include "_dic.h"
#include "_utils.h"
#include "thread.h"
#include "core.h"
#include "mainwindow.h"
#include "shortcuts.h"
#include "settings.h"

#include <QMenuBar>
#include <QToolBar>
#include <QLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QStatusBar>
#include <QLabel>
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QDialogButtonBox>

#include <winerror.h>

#include <QDebug>

static const int modItemMrg = 2;

/********************************************************************/
/*      MOD NAME ITEM       *****************************************/
/********************************************************************/
    ModNameItem::ModNameItem(const QString &modName) : QFrame()
    {
        QHBoxLayout *layout = new QHBoxLayout;
        setLayout(layout);
        layout->setSpacing(modItemMrg);
        layout->setContentsMargins(modItemMrg, modItemMrg, modItemMrg, modItemMrg);

                    iconLbl = new QLabel;
            QLabel *nameLbl = new QLabel(modName);
            layout->addWidget(iconLbl);
            layout->addWidget(nameLbl);
            iconLbl->hide();
            iconLbl->setPixmap(QIcon(":/icons/warning.png").pixmap(24, 24));
            iconLbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
            iconLbl->setAlignment(Qt::AlignCenter);
            nameLbl->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    }

    void ModNameItem::showIcon(const bool show)
    {
        if(show) iconLbl->show();
        else iconLbl->hide();
    }

/********************************************************************/
/*      MOD DATA ITEM       *****************************************/
/********************************************************************/
    ModDataItem::ModDataItem(const QString &zero, const bool alignRight) : QFrame()
    {
        QGridLayout *layout = new QGridLayout;
        setLayout(layout);
        layout->setSpacing(modItemMrg);
        layout->setContentsMargins(modItemMrg, modItemMrg, modItemMrg, modItemMrg);

            totalTitle   = new QLabel(d::TOTALc_);
            totalData    = new QLabel(zero);
            mountedTitle = new QLabel(d::MOUNTED+": ");
            mountedData  = new QLabel(zero);
            layout->addWidget(totalTitle,   0, 0);
            layout->addWidget(totalData,    0, 1);
            layout->addWidget(mountedTitle, 1, 0);
            layout->addWidget(mountedData,  1, 1);
            const Qt::AlignmentFlag &align = alignRight ? Qt::AlignRight : Qt::AlignLeft;
            totalTitle->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
            mountedTitle->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
            totalTitle->setAlignment(align|Qt::AlignVCenter);
            totalData->setAlignment(align|Qt::AlignVCenter);
            mountedTitle->setAlignment(align|Qt::AlignVCenter);
            mountedData->setAlignment(align|Qt::AlignVCenter);
            updateView(true);
    }

    bool ModDataItem::updateView(const bool hide)
    {
        const bool vis = totalTitle->isVisible();
        if(hide || (!isMounted && (mountedData->text() == d::ZERO_MB || mountedData->text() == d::ZERO_FILES)))
        {
            totalTitle->hide();
            mountedTitle->hide();
            mountedData->hide();

            return vis;
        }
        else
        {
            totalTitle->show();
            mountedTitle->show();
            mountedData->show();

            return !vis;
        }
    }

    bool ModDataItem::setTotalData(const QString &total)
    {
        totalData->setText(total);
        return updateView(total == mountedData->text());
    }

    bool ModDataItem::setMountedData(const QString &mounted)
    {
        mountedData->setText(mounted);
        return updateView(mounted == totalData->text());
    }

/********************************************************************/
/*      MOD TABLE       *********************************************/
/********************************************************************/
    ModTable::ModTable() : QTableWidget(0, 3)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setContextMenuPolicy(Qt::ActionsContextMenu);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setDragEnabled(false);
        setAlternatingRowColors(true);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        QFont tableFont = font();
        tableFont.setPointSizeF(tableFont.pointSize()*1.2);
        setFont(tableFont);

            verticalHeader()->hide();
            setHorizontalHeaderLabels({ d::MOD, d::SIZE, d::FILES });
            horizontalHeader()->setStretchLastSection(true);
            horizontalHeaderItem(0)->setTextAlignment(Qt::AlignVCenter|Qt::AlignLeft);
            horizontalHeaderItem(1)->setTextAlignment(Qt::AlignVCenter|Qt::AlignRight);
            horizontalHeaderItem(2)->setTextAlignment(Qt::AlignVCenter|Qt::AlignLeft);
    }

    bool ModTable::tryBusy(const QString &modName)
    {
        if(modData.find(modName) != modData.end() && !std::get<int(ModData::Busy)>(modData[modName]))
        {
            std::get<int(ModData::Busy)>(modData[modName]) = true;
            return true;
        }
        else return false;
    }

    void ModTable::resizeCR(const int row)
    {
        resizeColumnToContents(0);
        resizeColumnToContents(1); // Last column (2) is stretched
        resizeRowToContents(row);
    }

    int ModTable::addMod(const QString &modName)
    {
        const int row = rowCount();

        insertRow(row);

        ModNameItem *nameItem  = new ModNameItem(modName);
        ModDataItem *sizeItem  = new ModDataItem(d::ZERO_MB, true),
                    *filesItem = new ModDataItem(d::ZERO_FILES);
        nameItem->setFont(font());
        sizeItem->setFont(font());
        filesItem->setFont(font());
        setCellWidget(row, 0, nameItem);
        setCellWidget(row, 1, sizeItem);
        setCellWidget(row, 2, filesItem);

        resizeCR(row);

        return row;
    }

    void ModTable::setMounted(const QString &modName, const QString &errorList)
    {
        const int row = this->row(modName);

        if(row != -1)
        {
            QString style = "QFrame { border-top: 2px dashed #f7f500; border-bottom: 2px dashed #f7f500;"
                                     "background-color: #555; }"
                            "QLabel { border: 0; color: #eee; }";

            cellWidget(row, 0)->setStyleSheet(style);
            cellWidget(row, 1)->setStyleSheet(style);
            cellWidget(row, 2)->setStyleSheet(style);

            if(!errorList.isEmpty())
            {
                nameItem(row)->showIcon(true);
                cellWidget(row, 0)->setToolTip(errorList);
                cellWidget(row, 1)->setToolTip(errorList);
                cellWidget(row, 2)->setToolTip(errorList);
            }

            resizeCR(row);
        }
    }

    void ModTable::updateMod(const QString &modName, const QString &modSize, const QString &fileCount)
    {
        const int row = this->row(modName);

        if(row != -1)
        {
            bool resize = dataItem(row, false)->setTotalData(modSize);
            resize = dataItem(row, true)->setTotalData(fileCount) || resize;

            if(isRowHidden(row) && (modSize != d::ZERO_MB || fileCount != d::ZERO_FILES))
            {
                resize = true;
                showRow(row);
            }

            if(resize) resizeCR(row);
        }
    }

    void ModTable::updateMounted(const QString &modName, const QString &modSize, const QString &fileCount)
    {
        const int row = this->row(modName);

        if(row != -1)
        {
            bool resize = dataItem(row, false)->setMountedData(modSize);
            resize = dataItem(row, true)->setMountedData(fileCount) || resize;

            if(resize) resizeCR(row);
        }
    }

/********************************************************************/
/*      MAIN WINDOW     *********************************************/
/********************************************************************/
MainWindow::MainWindow(Core *const core) : QMainWindow(),
    core(core),
    gameIcons({ u::largestIcon(QIcon(":/icons/war3.ico")),      u::largestIcon(QIcon(":/icons/war3x.ico")),
                u::largestIcon(QIcon(":/icons/war3_mod.ico")),  u::largestIcon(QIcon(":/icons/war3x_mod.ico")) }),
    editIcons({ u::largestIcon(QIcon(":/icons/worldedit.ico")), u::largestIcon(QIcon(":/icons/worldedit_mod.ico")) })
{
    core->setParent(this);
    msgr.setParent(this);

    showMsg(d::SETUP_UI___, Msgr::Busy);

    setWindowTitle(d::WC3MM);
    setMinimumSize(420, 350);

    // MENUBAR
        QMenu *fileMenu  = new QMenu(d::aFILE),
              *toolsMenu = new QMenu(d::aTOOLS),
              *aboutMenu = new QMenu(d::aHELP);
        menuBar()->addMenu(fileMenu);
        menuBar()->addMenu(toolsMenu);
        menuBar()->addMenu(aboutMenu);

            QAction *acOpenGameFolder = new QAction(d::OPEN_X.arg(d::X_FOLDER).arg(d::aX).arg(d::GAME)),
                    *acOpenModsFolder = new QAction(d::OPEN_X.arg(d::X_FOLDER).arg(d::aX).arg(d::MODS)),
                    *acOpenShortcuts  = new QAction(d::aX.arg(d::CREATE_uSHORTCUTS)),
                    *acOpenSettings   = new QAction(d::aX.arg(d::SETTINGS)),
                    *acOpenAbout      = new QAction(d::aX.arg(d::ABOUT));
            fileMenu->addActions({ acOpenGameFolder, acOpenModsFolder });
            toolsMenu->addActions({ acOpenShortcuts, acOpenSettings });
            aboutMenu->addAction(acOpenAbout);

    // TOOLBARS
    QToolBar *gameToolBar = new QToolBar,
             *modsToolBar = new QToolBar;
    addToolBar(gameToolBar);
    addToolBarBreak();
    addToolBar(modsToolBar);
    gameToolBar->setIconSize(QSize(32, 32));
    // QAction with iconSize 32x32 appears to have default size of 35x35 (including hover background+border)
    const int tbMargins = 5, gtbItemMargins = 3, gtbMargins = tbMargins-gtbItemMargins; // <--^ so adjust margins/spacing
    gameToolBar->layout()->setSpacing(gtbMargins);
    gameToolBar->layout()->setContentsMargins(gtbMargins, gtbMargins, gtbMargins, gtbMargins);
    modsToolBar->layout()->setSpacing(tbMargins);
    modsToolBar->layout()->setContentsMargins(tbMargins, tbMargins, tbMargins, tbMargins);

        // GAME TOOLBAR WIDGETS
        launchGameAc   = new QAction; // Default visual margin == gtbItemMargins
        launchEditorAc = new QAction;
        gameToolBar->addAction(launchGameAc);
        gameToolBar->addAction(launchEditorAc);

        gameToolBar->addSeparator();

        QWidget *regTools = new QWidget;
        gameToolBar->addWidget(regTools);
        QVBoxLayout *regLayout = new QVBoxLayout;
        regTools->setLayout(regLayout);
        regLayout->setSpacing(0);
        regLayout->setContentsMargins(gtbItemMargins, gtbItemMargins, gtbItemMargins, gtbItemMargins);

            allowFilesCbx  = new QCheckBox(d::ALLOW_aLOCAL_FILES);
            gameVersionCbx = new QCheckBox(d::aX.arg(d::EXPANSION));
            regLayout->addWidget(allowFilesCbx);
            regLayout->addWidget(gameVersionCbx);

        // MODS TOOLBAR WIDGETS
        toggleMountBtn = new QPushButton;
        addModBtn      = new QPushButton(d::aX.arg(d::ADD_uMOD));
        refreshBtn     = new QPushButton(d::aREFRESH);
        modsToolBar->addWidget(toggleMountBtn);
        modsToolBar->addWidget(addModBtn);
        modsToolBar->addWidget(refreshBtn);
        toggleMountBtn->setCheckable(true);

    // MOD LIST
    modTable = new ModTable;
    setCentralWidget(modTable);

                toggleMountAc = new QAction;
        QAction *actionOpen   = new QAction(d::OPEN_X.arg(d::FOLDER)),
                *actionRename = new QAction(d::RENAME),
                *actionDelete = new QAction(d::dDELETE);
        modTable->addAction(toggleMountAc);
        modTable->addAction(actionOpen);
        modTable->addAction(actionRename);
        modTable->addAction(actionDelete);

    // STATUSBAR
    setStatusBar(new QStatusBar);
    statusBar()->setSizeGripEnabled(false);
    statusBar()->setStyleSheet("QLabel { margin: 3; }");

        statusLbl = new QLabel;
        statusBar()->addWidget(statusLbl);

    // initialize conditional UI & fetch mods
    refresh(true);

    // MESSAGES
    connect(core,  &Core::msg, this, &MainWindow::showStatus);
    connect(&msgr, &Msgr::msg, this, &MainWindow::showMsg);

    // MENUBAR
    connect(acOpenGameFolder, &QAction::triggered, this, &MainWindow::openGameFolder);
    connect(acOpenModsFolder, &QAction::triggered, this, &MainWindow::openModsFolder);
    connect(acOpenShortcuts,  &QAction::triggered, this, &MainWindow::openShortcuts);
    connect(acOpenSettings,   &QAction::triggered, this, &MainWindow::openSettings);
    connect(acOpenAbout,      &QAction::triggered, this, &MainWindow::openAbout);
    // TOOLBAR
    connect(launchGameAc,    SIGNAL(triggered()),   core, SLOT(launch()));
    connect(launchEditorAc,  &QAction::triggered,   this, &MainWindow::launchEditor);
    connect(allowFilesCbx,   SIGNAL(toggled(bool)),       SLOT(setAllowOrVersion(bool)));
    connect(gameVersionCbx,  &QCheckBox::toggled,   this, &MainWindow::setVersion);
    connect(addModBtn,       &QPushButton::clicked, this, &MainWindow::addMod);
    connect(refreshBtn,      SIGNAL(clicked()),           SLOT(refresh()));
    // MOD LIST
    //connect(modTable,      &QTableWidget::itemDoubleClicked, this, &MainWindow::renameModStart);
    connect(actionOpen,    &QAction::triggered,              this, &MainWindow::openModFolder);
    connect(actionRename,  &QAction::triggered,              this, &MainWindow::renameModAction);
    connect(actionDelete,  &QAction::triggered,              this, &MainWindow::deleteMod);
}

void MainWindow::show()
{
    QMainWindow::show();
    showMsg(d::READY_, Msgr::Permanent);
    core->closeSplash(this);
}

void MainWindow::showStatus(const QString &msg, const Msgr::Type &msgType)
{
    if(statusLbl && (msgType == Msgr::Permanent || msgType == Msgr::Critical))
        statusLbl->setText(msg);

    statusBar()->showMessage(msg, msgType == Msgr::Busy ? 0 : 10000);
}

void MainWindow::showMsg(const QString &msg, const Msgr::Type &msgType)
{
    showStatus(msg, msgType);
    core->showMsg(msg, msgType, false);
}

bool MainWindow::modSelected()
{
    if(!modTable->modSelected())
    {
        showMsg(d::NO_MOD_X_.arg(d::lSELECTED));
        return false;
    }
    else return true;
}

bool MainWindow::tryBusy(const QString &modName)
{
    if(!modTable->tryBusy(modName))
    {
        showMsg(d::X_BUSY.arg(modName), Msgr::Info);
        return false;
    }
    else return true;
}

void MainWindow::updateLaunchBtns()
{
    launchGameAc->setEnabled(false);
    launchEditorAc->setEnabled(false);

    const bool modEnabled = allowFilesCbx->isChecked() && !core->cfg.getSetting(Config::kMounted).isEmpty(),
               exp        = gameVersionCbx->isChecked();
    launchGameAc->setIcon(gameIcons[size_t(modEnabled<<1)|exp]);
    launchGameAc->setToolTip(d::LAUNCH_X.arg(modEnabled ? core->cfg.getSetting(Config::kMounted)
                                                          +" ("+(exp ? d::EXPANSION : d::CLASSIC)+")"
                                                        : exp ? d::TFT : d::ROC));
    launchEditorAc->setIcon(editIcons[modEnabled]);
    launchEditorAc->setToolTip(d::LAUNCH_X.arg(d::WE)+(modEnabled ? " ("+core->cfg.getSetting(Config::kMounted)+")"
                                                                  : QString()));

    launchGameAc->setEnabled(true);
    launchEditorAc->setEnabled(true);
}

void MainWindow::updateAllowOrVersion(const bool version)
{
    if(version) gameVersionCbx->setEnabled(false);
    else allowFilesCbx->setEnabled(false);

    HKEY hKey;
    if(!Config::regOpenWC3(KEY_READ, hKey)) showMsg(d::FAILED_TO_OPEN_REGK_, Msgr::Error);
    else
    {
        DWORD type = REG_DWORD, size = 1024, value;
        LSTATUS result = RegQueryValueEx(hKey, version ? Core::regGameVersion : Core::regAllowFiles,
                                         nullptr, &type, LPBYTE(&value), &size);

        if(result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) // ERROR_FILE_NOT_FOUND: value does not exist
            showMsg(d::FAILED_TO_GET_X_.arg(version ? d::GAME_VERSION : d::X_SETTING.arg(d::ALLOW_FILES)), Msgr::Error);

        else if(version) gameVersionCbx->setChecked(QString::number(value) == Config::vOn);
        else             allowFilesCbx->setChecked(QString::number(value) == Config::vOn);
    }

    RegCloseKey(hKey);

    updateLaunchBtns();
    if(version) gameVersionCbx->setEnabled(true);
    else allowFilesCbx->setEnabled(true);
}

void MainWindow::updateMountState(const bool setFocus)
{
    toggleMountBtn->setEnabled(false);

    disconnect(toggleMountBtn, &QPushButton::clicked, this, &MainWindow::unmountMod);
    disconnect(toggleMountBtn, &QPushButton::clicked, this, &MainWindow::mountMod);
    disconnect(toggleMountAc,  &QAction::triggered,   this, &MainWindow::unmountMod);
    disconnect(toggleMountAc,  &QAction::triggered,   this, &MainWindow::mountMod);

    if(core->cfg.getSetting(Config::kMounted).isEmpty())
    {
        toggleMountBtn->setText(d::aX.arg(d::MOUNT));
        toggleMountAc->setText(d::aX.arg(d::MOUNT));
        toggleMountBtn->setChecked(false);
        connect(toggleMountBtn, &QPushButton::clicked, this, &MainWindow::mountMod);
        connect(toggleMountAc,  &QAction::triggered,   this, &MainWindow::mountMod);

        if(setFocus) modTable->setFocus();
    }
    else
    {
        toggleMountBtn->setText(d::UNaMOUNT);
        toggleMountAc->setText(d::UNaMOUNT);
        toggleMountBtn->setChecked(true);
        connect(toggleMountBtn, &QPushButton::clicked, this, &MainWindow::unmountMod);
        connect(toggleMountAc,  &QAction::triggered,   this, &MainWindow::unmountMod);

        const QStringList &errorList = core->cfg.getSetting(Config::kMountedError).split(";", QString::SkipEmptyParts);
        QString errorString;
        if(errorList.length() > 0)
        {
            errorString = d::WARNING+":";
            for(int i=0; i < errorList.length(); ++i)
                if(i == 0 || errorList.at(i) != errorList.at(i-1))
                    errorString += "\n("+QString::number(i+1)+") "+errorList.at(i);
        }

        modTable->setMounted(core->cfg.getSetting(Config::kMounted), errorString);
    }

    updateLaunchBtns();
    toggleMountBtn->setEnabled(true);
}

void MainWindow::launchEditor()
{
    core->launch(true);
}

void MainWindow::setAllowOrVersion(const bool enable, const bool version)
{
    if(!core->setAllowOrVersion(enable, version))
        updateAllowOrVersion(version);
}

void MainWindow::refresh(const bool silent)
{
    refreshBtn->setEnabled(false);

    refreshing = !silent;
    if(!silent) showMsg(d::REFRESHING___, Msgr::Busy);

    ThreadModData *thr = new ThreadModData(core->cfg.pathMods, modTable->modData);
    connect(thr, &ThreadModData::modDataReady, this, &MainWindow::modDataReady);
    thr->init();

    updateAllowOrVersion();
    updateAllowOrVersion(true);
}

void MainWindow::modDataReady(const mod_m &modData, const QStringList &modNames)
{
    const QString &selectedMod = modTable->modSelected() && modTable->currentRow() < modTable->modNames.length()
                                    ? modTable->modNames[modTable->currentRow()] : QString();
    int selectedRow = -1;
    bool mountedFound = false;

    modTable->setRowCount(0);
    modTable->modData.clear();
    modTable->modData = modData;
    modTable->modNames.clear();
    modTable->modNames = modNames;

    scanCount = modTable->modNames.length();

    for(const QString &modName : modTable->modNames)
    {
        const int row = modTable->addMod(modName);
        if(modName == selectedMod) selectedRow = row;

        if(mountedFound || modName != core->cfg.getSetting(Config::kMounted))
        {
            if(core->cfg.getSetting(Config::kHideEmpty) == Config::vOn)
                modTable->hideRow(row);
        }
        else
        {
            mountedFound = true;

            if(modTable->tryBusy(modName))
            {
                ThreadScan *thr = new ThreadScan(modName, core->cfg.pathMods, true);
                connect(thr, &ThreadScan::scanModUpdate, modTable, &ModTable::updateMounted);
                connect(thr, &ThreadScan::scanModReady,  modTable, &ModTable::setIdle);
                thr->init();
            }
        }

        ThreadScan *thr = new ThreadScan(modName, core->cfg.pathMods);
        connect(thr, &ThreadScan::scanModUpdate, modTable, &ModTable::updateMod);
        connect(thr, &ThreadScan::scanModReady,  this,     &MainWindow::scanModDone);
        thr->init();
    }

    if(selectedRow >= 0 && selectedRow < modTable->rowCount()) modTable->selectRow(selectedRow);

    if(!core->cfg.getSetting(Config::kMounted).isEmpty() && !mountedFound)
        showMsg(d::FAILED_TO_FIND_MOUNTED_X_.arg(core->cfg.getSetting(Config::kMounted)), Msgr::Critical);

    updateMountState();
}

void MainWindow::scanModDone(const QString &modName)
{
    const int row = modTable->row(modName);

    if(row != -1)
    {
        modTable->resizeCR(row);
        if(row == modTable->currentRow())
        {
            //modTable->scrollToItem(modTable->item(row, 0));
            modTable->setFocus();
        }
    }

    if(--scanCount <= 0)
    {
        refreshBtn->setEnabled(true);
        if(refreshing)
        {
            refreshing = false;
            showMsg(d::REFRESHED_);
        }
    }
}

void MainWindow::mountMod()
{
    if(!modTable->modSelected()) showMsg(d::SELECT_MOD_TO_MOUNT_, Msgr::Info);
    else
    {
        const QString &modName = modTable->modNames[modTable->currentRow()];

        if(tryBusy(modName) && core->mountMod(modName, false) == Core::MountReady)
        {
            toggleMountBtn->setEnabled(false);

            ThreadMount *thr = core->mountModThread(modName);
            connect(thr, &ThreadMount::scanModUpdate, modTable, &ModTable::updateMounted);
            thr->init();
        }

        updateMountState(true);
    }
}

void MainWindow::mountModReady(const ThreadAction &action)
{
    modTable->setIdle(action.modName);
    core->mountModReady(action);
    updateMountState(true);
}

void MainWindow::unmountMod()
{

    if(tryBusy(core->cfg.getSetting(Config::kMounted)) && core->unmountMod())
    {
        toggleMountBtn->setEnabled(false);

        ThreadUnmount *thr = core->unmountModThread();
        connect(thr, &ThreadUnmount::scanModUpdate, modTable, &ModTable::updateMounted);
        thr->init();
    }
    else updateMountState(true);
}

void MainWindow::unmountModReady(const ThreadAction &action)
{
    if(core->unmountModReady(action))
    {
      /*  modList->item(row, 0)->setIcon(QIcon());
        modList->item(row, 0)->setToolTip(QString());*/
    }

    updateMountState(true);
}

void MainWindow::addMod()
{
    addModBtn->setEnabled(false);

    showMsg(d::ADDING_X___.arg(d::lMOD), Msgr::Busy);

    const QString &src = QFileDialog::getExistingDirectory(this, d::ADD_uMOD, QString(),
                                                           QFileDialog::ShowDirsOnly|QFileDialog::HideNameFilterDetails);

    if(!src.isEmpty())
    {
        QMessageBox copyMove(this);
        copyMove.setWindowTitle(d::COPY_MOVEq);
        copyMove.setText(d::COPY_MOVE_LONGq+"\n"+src);
        QPushButton *moveBtn = copyMove.addButton(d::MOVE, QMessageBox::ActionRole),
                    *copyBtn = copyMove.addButton(d::COPY, QMessageBox::ActionRole);
        copyMove.addButton(QMessageBox::Cancel);
        copyMove.setIcon(QMessageBox::Question);

        copyMove.exec();

        if(copyMove.clickedButton() == copyBtn || copyMove.clickedButton() == moveBtn)
        {
            const QString &modName = QDir(src).dirName(),
                          &dst     = core->cfg.pathMods+"/"+modName;

            if(QFileInfo().exists(dst)) showMsg(d::MOD_EXISTS_, Msgr::Error);
            else
            {
                showMsg(d::ADDING_X___.arg(modName), Msgr::Busy);

                ThreadAdd *thr = new ThreadAdd(modName, src, dst, copyMove.clickedButton() == copyBtn ? ThreadBase::Copy
                                                                                                      : ThreadBase::Move,
                                               core->cfg.pathMods, core->cfg.getSetting(Config::kGamePath));

                connect(thr, &ThreadAdd::resultReady, this, &MainWindow::addModReady);
                thr->init();

                return; //dont enable btn
            }
        }
    }

    addModBtn->setEnabled(true);
}

void MainWindow::addModReady(const ThreadAction &action)
{
    refresh(true);
    showMsg(Core::a2s(action));

    addModBtn->setEnabled(true);
}

void MainWindow::deleteMod()
{
    if(modSelected())
    {
        const QString &modName = modTable->modNames[modTable->currentRow()];

        if(QMessageBox::warning(this, d::PERM_DELETE_Xq.arg(modName),
                                d::PERM_DELETE_X_LONGq.arg(modName,
                                                          /* dynamic_cast<QLabel *>(modList->cellWidget(modList->currentRow(), 1))->text(),
                                                           dynamic_cast<QLabel *>(modList->cellWidget(modList->currentRow(), 2))->text()),*/
                                                           "tt", "tt"),
                                QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
        {
            showMsg(d::DELETING_X___.arg(modName), Msgr::Busy);

            ThreadDelete *thr = new ThreadDelete(modName, core->cfg.pathMods, core->cfg.getSetting(Config::kGamePath));
            connect(thr, &ThreadDelete::resultReady, this, &MainWindow::deleteModReady);
            thr->init();
        }
    }
}

void MainWindow::deleteModReady(const ThreadAction &action)
{
    // if errors -> rescan mod + popup
    showMsg(Core::a2s(action));
}

void MainWindow::renameModAction()
{
   // if(modSelected()) renameModStart(modList->item(modList->currentRow(), 0));
}

void MainWindow::renameModStart(QTableWidgetItem *item)
{
  /*  if(item->column() != 0) item = modList->item(item->row(), 0);
    renameModName = item->text();
    renameModItem = item;
    connect(modList, &QTableWidget::itemChanged, this, &MainWindow::renameModSave);
    modList->editItem(item);*/
}

void MainWindow::renameModSave(QTableWidgetItem *item)
{/*
    disconnect(modList, &QTableWidget::itemChanged, this, &MainWindow::renameModSave);

    QString statusMsg;
    bool error = true;

    if(item != renameModItem) statusMsg = d::FAILED_TO_X_.arg(d::lRENAME_MOD);
    else
    {
        const QString &srcPath = core->cfg.pathMods+"/"+renameModName;
        const QFileInfo &fiSrc(srcPath);

        if(fiSrc.isSymLink() || !fiSrc.exists() || !fiSrc.isDir())
            statusMsg = d::X_NOT_FOUND.arg(d::MOD)+".";
        else
        {
            const QString &newModName = item->text().simplified();

            if(!u::isValidFileName(newModName))
                statusMsg = d::INVALID_X.arg(d::lFILENAME)+".\n"+d::CHARACTERS_NOT_ALLOWED;
            else
            {
                const QString &dstPath = core->cfg.pathMods+"/"+newModName;

                if(QFileInfo().exists(dstPath)) statusMsg = d::MOD_EXISTS_;
                else if(!QDir().rename(srcPath, dstPath)) statusMsg = d::FAILED_TO_X_.arg(d::lRENAME_MOD);
                else
                {
                    error = false;
                    statusMsg = d::X_RENAMED_X_.arg(renameModName, newModName);
                }
            }
        }

        if(error) item->setText(renameModName);
    }

    renameModName = QString();
    renameModItem = nullptr;

    showMsg(statusMsg, error ? Msgr::Error : Msgr::Default);*/
}

void MainWindow::openFolder(const QString &path, const QString &name, QString lName)
{
    if(lName.isEmpty()) lName = name;
    showMsg(d::OPENING_X_FOLDER___.arg(lName), Msgr::Busy);

    if(QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        showMsg(d::X_FOLDER_OPENED_.arg(name));
    else showMsg(d::FAILED_TO_X_.arg(d::lOPEN_X).arg(d::X_FOLDER).arg(lName), Msgr::Error);
}

void MainWindow::openGameFolder()
{
    openFolder(core->cfg.getSetting(Config::kGamePath), d::GAME, d::lGAME);
}

void MainWindow::openModsFolder()
{
    openFolder(core->cfg.pathMods, d::MODS, d::lMODS);
}

void MainWindow::openModFolder()
{
    if(modTable->modSelected())
    {
        const QString &modName = modTable->modNames[modTable->currentRow()];
        openFolder(core->cfg.pathMods+"/"+modName, modName);
    }
    else openModsFolder();
}

void MainWindow::openShortcuts()
{
    Shortcuts shortcuts(this, QStringList(), core->cfg.getSetting(Config::kGamePath), &msgr);
    shortcuts.exec();
}

void MainWindow::openSettings()
{
    Settings settings(this, core->cfg, &msgr);
    if(settings.exec()) refresh(true); //limit refresh: "request" scan in settings when hideempty changed
}

void MainWindow::openAbout()
{
    QDialog about(this, Qt::FramelessWindowHint|Qt::MSWindowsFixedSizeDialogHint);
    about.setWindowTitle(d::ABOUT);
    about.setWindowOpacity(0.9);
    about.setFixedWidth(350);
    about.setStyleSheet("QDialog { background-color: #000; border: 2px solid #fac805; }"
                        "QLabel { color: #bbb; }");

    QVBoxLayout aboutLayout;
    about.setLayout(&aboutLayout);
    aboutLayout.setContentsMargins(0, 0, 0, 0);
    aboutLayout.setSpacing(0);

        QLabel logoLbl;
        aboutLayout.addWidget(&logoLbl);
        logoLbl.setAlignment(Qt::AlignTop);
        logoLbl.setFixedSize(350, 110);
        logoLbl.setPixmap(core->pxLogo);
        QLabel versionLbl(&about);
        versionLbl.setAlignment(Qt::AlignHCenter);
        versionLbl.setFixedSize(350, 13);
        versionLbl.move(0, 83);
        versionLbl.setPixmap(core->pxVersion);

        QHBoxLayout textLayout;
        aboutLayout.addLayout(&textLayout);
        textLayout.setContentsMargins(2, 0, 0, 0);
            textLayout.addItem(new QSpacerItem(40, 0, QSizePolicy::Expanding));

            QGridLayout textGrid;
            textLayout.addLayout(&textGrid);
            textGrid.setVerticalSpacing(0);

                QLabel dlLbl(d::DOWNLOADc), srcLbl(d::SOURCEc), lcnsLbl(d::LICENSEc),
                       dlLink  ("<a style=\"color: #66f;\" href=\"https://www.hiveworkshop.com/threads/wc3-mod-manager.308948/\">"
                                    "Hive Workshop</a>"),
                       srcLink ("<a style=\"color: #66f;\" href=\"https://github.com/EzraZebra/WC3ModManager\">"
                                    "GitHub</a>"),
                       lcnsLink("<a style=\"color: #66f;\" href=\"https://www.gnu.org/licenses/gpl-3.0.html\">"
                                    "GPLv3</a>");
                textGrid.addWidget(&dlLbl,    0, 0);
                textGrid.addWidget(&srcLbl,   1, 0);
                textGrid.addWidget(&lcnsLbl,  2, 0);
                textGrid.addItem(new QSpacerItem(35, 0, QSizePolicy::Expanding), 0, 1);
                textGrid.addWidget(&dlLink,   0, 2);
                textGrid.addWidget(&srcLink,  1, 2);
                textGrid.addWidget(&lcnsLink, 2, 2);
                QFont font("Calibri", 14);
                dlLbl.setFont(font);
                srcLbl.setFont(font);
                lcnsLbl.setFont(font);
                dlLink.setFont(font);
                dlLink.setOpenExternalLinks(true);
                srcLink.setFont(font);
                srcLink.setOpenExternalLinks(true);
                lcnsLink.setFont(font);
                lcnsLink.setOpenExternalLinks(true);

            textLayout.addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

        QDialogButtonBox closeBtn(QDialogButtonBox::Close);
        aboutLayout.addWidget(&closeBtn);
        closeBtn.setContentsMargins(0, 0, 12, 12);

    connect(&closeBtn, &QDialogButtonBox::rejected, &about, &QDialog::reject);

    about.exec();
}
