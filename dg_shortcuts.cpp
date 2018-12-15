#include "_dic.h"
#include "_utils.h"
#include "_msgr.h"
#include "thread.h"
#include "dg_shortcuts.h"
#include "dg_shortcuts_pvt.h"

#include <QMouseEvent>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QMimeDatabase>
#include <QtWinExtras/qwinfunctions.h>
#include <QScrollBar>
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include <QRadioButton>
#include <QCoreApplication>
#include <QDesktopServices>

#include <cmath>

/********************************************************************/
/*      ICON BUTTON     *********************************************/
/********************************************************************/
    IconButton::IconButton(const QIcon &icon) : QPushButton(icon, QString())
    {
        setCheckable(true);
        setAutoDefault(false);
        setFlat(true);
        setFixedSize(48, 48);
        setContentsMargins(8, 8, 8, 8);
        setIconSize(QSize(32, 32));
        setStyleSheet(":checked, :hover { background: #c4e5f6; border: 1 solid #2c628b; }");
    }

    void IconButton::mouseDoubleClickEvent(QMouseEvent *e)
    { if(e->button() == Qt::LeftButton) emit doubleClicked(); }

/********************************************************************/
/*      ICONSELECT DIAG     *****************************************/
/********************************************************************/
    IconSelect::IconSelect(QWidget *parent, QAbstractButton *const btn, const int btnId, Msgr *const msgr)
        : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint),
        btn(btn), msgr(msgr), btnId(btnId)
    {
        btn->setText(d::SELECT+"...");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("padding-bottom: 1; padding-right: 40;");

        connect(btn, SIGNAL(clicked(bool)), this, SLOT(getIcons(bool)));
    }

    void IconSelect::setupUi()
    {
        disconnect(btn, SIGNAL(clicked(bool)), this, SLOT(getIcons(bool)));

        setWindowTitle(d::SELECT+" "+d::ICON);

        layout = new QVBoxLayout;
        setLayout(layout);

            QHBoxLayout *browseCont = new QHBoxLayout;
            layout->addLayout(browseCont);

                browseEdit = new QLineEdit;
                browseCont->insertWidget(0, browseEdit);

                QPushButton *browseBtn = new QPushButton(d::BROWSE___);
                browseCont->addWidget(browseBtn);

            newScrollArea();

            QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
            layout->addWidget(buttonBox);

        connect(browseBtn,  SIGNAL(clicked(bool)),             SLOT(getIcons(bool)));
        connect(browseEdit, &QLineEdit::returnPressed,   this, &IconSelect::browseReturn);
        connect(buttonBox,  &QDialogButtonBox::accepted, this, &IconSelect::accept);
        connect(buttonBox,  &QDialogButtonBox::rejected, this, &IconSelect::reject);

        connect(btn, &QAbstractButton::clicked, this, &IconSelect::exec);
    }

    void IconSelect::newScrollArea()
    {
        scrollArea = new QScrollArea;
        layout->insertWidget(1, scrollArea);
        scrollArea->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

            frame = new QFrame;
            scrollArea->setWidget(frame);
            scrollArea->setWidgetResizable(true);

                grid = new QGridLayout;
                frame->setLayout(grid);

                    btnGroup.setParent(grid);
    }

    void IconSelect::browseReturn()
    {
        const QFileInfo &fi(browseEdit->text().simplified());
        getIcons(!fi.isSymLink() && (!fi.exists() || !fi.isFile()), false);
    }

    void IconSelect::accept()
    {
        selectedPath = loadedPath;
        selectedIndex = btnGroup.checkedId();
        btn->setStyleSheet(QString());
        btn->setIcon(btnGroup.checkedButton()->icon());

        QDialog::accept();
    }

    void IconSelect::getIcons(bool maybeBrowse, const bool browseMaybe)
    {
        QString browsePath = isVisible() ? browseEdit->text().simplified() : QString();

        do
        {
            if(maybeBrowse || browseMaybe || (!QFileInfo(browsePath).isSymLink() && !QFileInfo().exists(browsePath)))
                browsePath = QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, d::SELECT+" "+d::ICON, browsePath,
                                "Icon files (*.ico;*.icl;*.exe;*.dll);;Programs (*.exe);;Libraries (*.dll);;Icons (*.ico);;All files (*.*)"));
            else maybeBrowse = true;

            if(browsePath.isEmpty()) break;

            QList<QIcon> icons;

            QMimeType mime = QMimeDatabase().mimeTypeForFile(browsePath);
            if(mime.inherits("image/x-icon") || mime.inherits("image/vnd.microsoft.icon") || mime.inherits("image/ico")
                    || mime.inherits("image/icon") || mime.inherits("text/ico") || mime.inherits("application/ico"))
                icons << u::largestIcon(QIcon(browsePath));

            if(icons.length() == 0)
            {
                // QString::toWCharArray does not produce 0-terminated string
                std::vector<WCHAR> path(unsigned(browsePath.length())+1); // so reserve space for 0
                browsePath.toWCharArray(path.data());
                path.at(path.size()-1) = 0; // and add 0

                // Get number of icons in selected file
                UINT nIcons = ExtractIconEx(path.data(), -1, nullptr, nullptr, 0);

                if(nIcons == 0)
                {
                    // Try to find associated exe that contains icon(s)
                    WORD index=0;
                    DestroyIcon(ExtractAssociatedIcon(GetModuleHandle(nullptr), path.data(), &index));
                    // Get number of icons in assoc file
                    nIcons = ExtractIconEx(path.data(), -1, nullptr, nullptr, 0);
                }

                if(nIcons > 0)
                {
                    // Get array of HICONs
                    std::vector<HICON> iconHandles(nIcons);
                    nIcons = ExtractIconEx(path.data(), 0, iconHandles.data(), nullptr, nIcons);
                    if(nIcons != iconHandles.size()) iconHandles.resize(nIcons);

                    for(UINT i=0; i<nIcons; ++i) icons << u::largestIcon(QtWin::fromHICON(iconHandles[i]));
                }
            }

            if(icons.length() <= 0) emit msgr->msg(d::FILE_NO_ICONS_, Msgr::Error);
            else
            {
                if(isVisible())
                {
                    delete scrollArea;
                    newScrollArea();
                }
                else setupUi();

                const int maxcol = std::min(int(ceil(sqrt(icons.length()))), 6);
                for(int i=0; i<icons.length(); ++i)
                {
                    IconButton *iconBtn = new IconButton(icons[i]);

                    const int row = int(floor(i/maxcol));
                    grid->addWidget(iconBtn, row, i-row*maxcol);
                    btnGroup.addButton(iconBtn, i);

                    connect(iconBtn, &IconButton::doubleClicked, this, &IconSelect::accept);
                }

                adjustSize();
                frame->adjustSize();

                if(frame->height() > scrollArea->height())
                    scrollArea->setMinimumWidth(frame->width()+scrollArea->verticalScrollBar()->height());

                btnGroup.button(0)->setChecked(true);
                browseEdit->setText(browsePath);
                loadedPath = browsePath;

                if(!isVisible()) exec();

                break;
            }
        } while(!browsePath.isEmpty());
    }

/********************************************************************/
/*      SHORTCUTS DIAG       ****************************************/
/********************************************************************/
    Shortcuts::Shortcuts(QWidget *parent, const QStringList &modNames, const QString &gamePath, Msgr *const msgr)
        : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint),
          msgr(msgr),
          // values correspond to index in `icons`:
          mainIconsIndex({{ { 1, 2, 3 }, { 5, 6, 7 } }}), // { war3, war3x, worldedit }, { war3_mod, war3x_mod, worldedit_mod }
          // int values correspond to index in `RC_ICONS` (.pro file):
          icons({ { 0, "iconSelect" },            { 6, ":/icons/war3.ico" },          { 7, ":/icons/war3x.ico" },
                  { 8, ":/icons/worldedit.ico" }, { 9, ":/icons/war3z.ico" },         { 1, ":/icons/war3_mod.ico" },
                  { 2, ":/icons/war3x_mod.ico" }, { 3, ":/icons/worldedit_mod.ico" }, { 0, ":/icons/icon.ico" },
                  { 4, ":/icons/war2.ico" },      { 5, ":/icons/war3d.ico" },         { 10, ":/icons/wow.ico" },
                  { 11, ":/icons/hive.ico" }
                }),
          gamePath(gamePath)
    {
        setWindowTitle(d::CREATE_uSHORTCUTS);

        setMinimumWidth(450);
        QVBoxLayout *layout = new QVBoxLayout;
        setLayout(layout);

            QFormLayout *argForm = new QFormLayout;
            layout->addLayout(argForm);

            // MOD SELECT
                modSelect = new QComboBox;
                argForm->addRow(d::MOD+":", modSelect);
                modSelect->addItem(d::L_NONE);
                modSelect->addItems(modNames);

            // VERSION SELECT
                QLabel *versionLbl = new QLabel(d::VERSIONc);
                QHBoxLayout *versionLayout = new QHBoxLayout;
                argForm->addRow(versionLbl, versionLayout);
                versionLbl->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

                    versionGroup.setParent(versionLayout);
                    versionGroup.addButton(new QRadioButton(d::DONT_SET),  3);
                    versionGroup.addButton(new QRadioButton(d::CLASSIC),   0);
                    versionGroup.addButton(new QRadioButton(d::EXPANSION), 1);
                    versionGroup.addButton(new QRadioButton(d::WE),        2);
                    versionLayout->addWidget(versionGroup.button(3));
                    versionLayout->addWidget(versionGroup.button(0));
                    versionLayout->addWidget(versionGroup.button(1));
                    versionLayout->addWidget(versionGroup.button(2));
                    versionGroup.button(3)->setChecked(true);

            // EXTRA EDIT
                QHBoxLayout *extraLayout = new QHBoxLayout;
                argForm->addRow(d::EXTRAc, extraLayout);

                    extraEdit = new QLineEdit;
                    extraLayout->addWidget(extraEdit);

                    QPushButton *infoBtn = new QPushButton(QIcon(":/icons/info.png"), QString());
                    extraLayout->addWidget(infoBtn);
                    infoBtn->setFlat(true);
                    infoBtn->setAutoDefault(false);
                    infoBtn->setFixedSize(16, 16);
                    infoBtn->setIconSize(QSize(16, 16));
                    infoBtn->setToolTip(d::WC3_CMD_GUIDE);
                    infoBtn->setStyleSheet("background-color: transparent !important;");
                    infoBtn->setCursor(Qt::PointingHandCursor);

            // RESULT
                resultEdit = new QLineEdit;
                argForm->addRow(d::RESULTc, resultEdit);
                resultEdit->setStyleSheet("border: 1px solid #adb2b5; color: #848484;");
                resultEdit->setReadOnly(true);

            QFormLayout *scForm = new QFormLayout;
            layout->addLayout(scForm);

            // NAME EDIT
                nameEdit = new QLineEdit;
                scForm->addRow(d::SHORTCUT_uNAMEc, nameEdit);

            // LOCATION EDIT
                QHBoxLayout *locLayout = new QHBoxLayout;
                scForm->addRow(d::SHORTCUT_X.arg(d::LOC)+":", locLayout);

                    locEdit = new QLineEdit(
                                QDir::toNativeSeparators(
                                    QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)));
                    locLayout->addWidget(locEdit);

                    QPushButton *browseBtn = new QPushButton(d::BROWSE___);
                    locLayout->addWidget(browseBtn);
                    browseBtn->setAutoDefault(false);

            // ICON SELECT
                QLabel *iconLbl = new QLabel(d::SHORTCUT_uICONc);
                QGridLayout *iconLayout = new QGridLayout;
                scForm->addRow(iconLbl, iconLayout);
                iconLbl->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

                    iconGroup.setParent(iconLayout);
                    const int maxcol = 4;
                    for(int i=0, totColspan=0; i<int(icons.size()); ++i)
                    {
                        QRadioButton *btn = new QRadioButton;
                        iconGroup.addButton(btn, int(i));
                        btn->setFixedHeight(40);
                        btn->setIconSize(QSize(32, 32));

                        const int iSpan = i+totColspan,
                                  &row = int(floor(iSpan/maxcol)), col = iSpan-row*maxcol;
                        if(icons[size_t(i)].second == "iconSelect")
                        {
                            const int colspan = maxcol-col; // span till end of row
                            iconLayout->addWidget(btn, row, col, 1, colspan);
                            iconSelect = new IconSelect(this, btn, int(i), msgr);
                            totColspan += colspan-1;
                        }
                        else
                        {
                            iconLayout->addWidget(btn, row, col);
                            btn->setIcon(u::largestIcon(QIcon(icons[size_t(i)].second)));
                        }
                    }

            // "CREATE" BUTTON
            QHBoxLayout *createLayout = new QHBoxLayout;
            layout->addLayout(createLayout);

                createLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

                createBtn = new QPushButton(d::CREATE_X.arg(d::SHORTCUT));
                createLayout->addWidget(createBtn);
                createBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

                createLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

            // CLOSE BUTTON
            QDialogButtonBox *closeBtn = new QDialogButtonBox(QDialogButtonBox::Close);
            layout->addWidget(closeBtn);

        connect(modSelect,     SIGNAL(currentIndexChanged(int)),  SLOT(updateResult()));
        connect(extraEdit,     SIGNAL(textChanged(QString)),      SLOT(updateResult()));
        connect(nameEdit,      SIGNAL(textEdited(QString)),       SLOT(disableAutoName()));
        connect(&versionGroup, SIGNAL(buttonToggled(int, bool)),  SLOT(updateResult(int, bool)));
        connect(&iconGroup,    SIGNAL(buttonClicked(int)),        SLOT(disableAutoIcon()));
        connect(infoBtn,       &QPushButton::clicked,       this, &Shortcuts::openGuide);
        connect(browseBtn,     &QPushButton::clicked,       this, &Shortcuts::browseLocation);
        connect(createBtn,     &QPushButton::clicked,       this, &Shortcuts::create);
        connect(closeBtn,      &QDialogButtonBox::rejected, this, &Shortcuts::reject);

        updateResult();
    }

    QString Shortcuts::getArgs(QString *const name, int *const iIcon)
    {
        const int vrsInd = versionGroup.checkedId();
        const QString &mod = modSelect->currentText(),
                      &version = vrsInd == 0   ? d::V_CLASSIC
                                 : vrsInd == 1 ? d::V_EXPANSION
                                 : vrsInd == 2 ? d::V_WE
                                               : QString();

        QString result = QStringLiteral(u"%0 \"%1\"").arg(d::_X.arg(d::C_LAUNCH), mod)
                         +(vrsInd < 3 ? QStringLiteral(u" %0 %1").arg(d::_X.arg(d::C_VERSION), version) : QString());

        if(vrsInd == 2) extraEdit->setEnabled(false);
        else
        {
            extraEdit->setEnabled(true);
            if(!extraEdit->text().simplified().isEmpty())
                result += QStringLiteral(u" %0 \"%1\"").arg(d::_X.arg(d::C_NATIVE), extraEdit->text().simplified());
        }

        if(name != nullptr)
            *name = mod == d::L_NONE ? d::WC3+(vrsInd == 0   ? " - "+d::ROC
                                               : vrsInd == 1 ? " - "+d::TFT
                                               : vrsInd == 2 ? " - "+d::WE
                                                             : QString())
                                     : mod+(vrsInd == 0   ? " - "+d::CLASSIC
                                            : vrsInd == 1 ? " - "+d::EXPANSION
                                            : vrsInd == 2 ? " - "+d::WE
                                                          : QString());

        if(iIcon != nullptr) *iIcon = mainIconsIndex[mod != d::L_NONE][vrsInd < 0 || vrsInd > 2 ? 0 : size_t(vrsInd)];

        return result;
    }

    void Shortcuts::create()
    {
        createBtn->setEnabled(false);
        emit msgr->msg(d::CREATING_SHORTCUT___, Msgr::Busy);

        bool error = true;
        const QString &name = nameEdit->text().simplified();

        if(!u::isValidFileName(name))
            emit msgr->msg(d::FAILED_TO_CREATE_SHORTCUTc_X_.arg(d::lINVALID_X).arg(d::lFILENAME)
                           +"\n"+d::CHARACTERS_NOT_ALLOWED, Msgr::Error);
        else
        {
            const QString &path = locEdit->text().simplified();
            const QFileInfo &fiPath(path);

            if(fiPath.isSymLink() || !fiPath.exists() || !fiPath.isDir())
                emit msgr->msg(d::FAILED_TO_CREATE_SHORTCUTc_X_.arg(d::lINVALID_X).arg(d::lLOC), Msgr::Error);

            else if(QFileInfo().exists(path+"/"+name+".lnk"))
                emit msgr->msg(d::FAILED_TO_CREATE_SHORTCUTc_X_.arg(d::lEXISTS), Msgr::Error);

            else
            {
                QString iconPath;
                int iconIndex = 0;

                const bool isRoc    = iconGroup.checkedId() == mainIconsIndex[0][0],
                           &isWe     = iconGroup.checkedId() == mainIconsIndex[0][2],
                           &isCustom = iconGroup.checkedId() == iconSelect->btnId;
                if(isRoc || isWe || iconGroup.checkedId() == mainIconsIndex[0][1]) // [0][1] == Tft
                {
                    const QString &exePath = gamePath+"/"+(isWe ? d::WE_EXE : d::WC3_EXE);
                    const QFileInfo &fiExe(exePath);
                    if(!fiExe.isSymLink() && fiExe.exists() && fiExe.isExecutable())
                    {
                        iconPath = exePath;
                        iconIndex = isRoc || isWe ? 1 : 2;
                    }
                }
                else if(isCustom)
                {
                    iconPath = iconSelect->selectedPath;
                    iconIndex = iconSelect->selectedIndex;
                }

                if(!isCustom && iconPath.isEmpty())
                {
                    iconPath = QCoreApplication::applicationFilePath();
                    iconIndex = icons[size_t(iconGroup.checkedId())].first;
                }

                if(iconPath.isEmpty())
                    emit msgr->msg(d::FAILED_TO_CREATE_SHORTCUTc_X_.arg(d::lNO_X_SELECTED).arg(d::lICON), Msgr::Error);
                else if(!QFileInfo().exists(iconPath) || !QFileInfo(iconPath).isFile())
                    emit msgr->msg(d::FAILED_TO_CREATE_SHORTCUTc_X_.arg(d::X_NOT_FOUND).arg(d::lICON), Msgr::Error);
                else
                {
                    error = false;
                    Thread *thr = new Thread(ThreadAction::Shortcut, msgr);
                    connect(thr, &Thread::shortcutReady, this, &Shortcuts::shortcutReady);
                    thr->start(path+"/"+name, getArgs(), iconPath, iconIndex);
                }
            }
        }

        if(error) createBtn->setEnabled(true);
    }

    void Shortcuts::shortcutReady()
    {
        createBtn->setEnabled(true);
        autoName = true;
        autoIcon = true;
    }

    void Shortcuts::updateResult(const int, const bool filter)
    {
        if(filter)
        {
            QString name;
            int iIcon;
            const QString &args = getArgs(&name, &iIcon);

            resultEdit->setText(QFileInfo(QCoreApplication::applicationFilePath()).fileName()+" "+args);
            if(autoName) nameEdit->setText(name);
            if(autoIcon) iconGroup.buttons().at(iIcon)->setChecked(true);
        }
    }

    void Shortcuts::browseLocation()
    {
        const QString &path = QFileDialog::getExistingDirectory(this, d::SHORTCUT_X.arg(d::LOC),
                                                               locEdit->text().simplified(), QFileDialog::ShowDirsOnly);

        if(!path.isEmpty()) locEdit->setText(QDir::toNativeSeparators(path));
    }

    void Shortcuts::openGuide()
    { QDesktopServices::openUrl(QString("https://www.hiveworkshop.com/threads/complete-command-line-arguments-guide.288224/")); }
