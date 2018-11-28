#ifndef DIC_H
#define DIC_H

#include "_utils.h"
#include <unordered_map>

//****************************************************************************
//**    LEGEND      **********************************************************
//****************************************************************************
//  _       " "             FOO_BAR                     "Foo bar
//  _       "."             FOO_BAR_                    "Foo bar."
//  _       "-"             _FOO                        "-foo"
//  __      ". "            FOO__BAR                    "Foo. Bar"
//  ___     "..."           FOO_BAR___                  "Foo bar..."
//  l       (lowercase)     lFOO_BAR                    "foo bar"
//  u       (uppercase)     FOO_uBAR                    "Foo Bar"
//  q       "?"             FOO_BARq                    "Foo bar?"
//  c       ":"             FOOc_BAR                    "Foo: bar"
//  c_      ": "            FOOc_                       "Foo: "
//  a       "&"             FOO_aBAR                    "Foo &Bar"
//  X       (variable)      FOO_X_BAR_X.arg(lABC, ":)") "Foo abc bar :)"
//  d       (prefix)        dFOO_BAR                    "Foo bar"
//              --> to avoid conflicts with predefined constants/macros/keywords (eg DELETE, ERROR, FAILED)
//*****************************************************************************
//*****************************************************************************

namespace d {
static const QString

    X_X = QStringLiteral(u"%0 %1"),

    WC3MM = QStringLiteral(u"WC3 Mod Manager"),

//LAUNCH STRINGS
    WC3_EXE = QStringLiteral(u"Warcraft III.exe"),
    WE_EXE  = QStringLiteral(u"World Editor.exe"),

    L_NONE = QStringLiteral(u"<none>"),

    _X          = QStringLiteral(u"-%0"),
    C_LAUNCH    = QStringLiteral(u"launch"),
    C_VERSION   = QStringLiteral(u"version"),
    C_NATIVE    = QStringLiteral(u"native"),

    V_CLASSIC   = QStringLiteral(u"classic"),
    V_EXPANSION = QStringLiteral(u"expansion"),
    V_WE        = QStringLiteral(u"worldedit"),

//GAME STRINGS
    WC3         = QStringLiteral(u"Warcraft III"),
    ROC         = QStringLiteral(u"Reign of Chaos"),
    TFT         = QStringLiteral(u"The Frozen Throne"),
    CLASSIC     = QStringLiteral(u"Classic"),
    EXPANSION   = QStringLiteral(u"Expansion"),
    WE          = QStringLiteral(u"World Editor"),

// GENERAL NOUNS
    EXTRAc  = QStringLiteral(u"Extra:"),
    RESULTc = QStringLiteral(u"Result:"),
    TOTALc_ = QStringLiteral(u"Total: "),

    lGAME   = QStringLiteral(u"game"),
    GAME    = QStringLiteral(u"Game"),
    lMOD    = QStringLiteral(u"mod"),
    MOD     = QStringLiteral(u"Mod"),
    lMODS   = QStringLiteral(u"mods"),
    MODS    = QStringLiteral(u"Mods"),

    FOLDER          = QStringLiteral(u"Folder"),
    lFOLDER         = QStringLiteral(u"folder"),
    X_uFOLDER       = X_X.arg("%0", FOLDER),
    X_FOLDER        = X_X.arg("%0", lFOLDER),

    VERSIONc        = QStringLiteral(u"Version:"),
    INFO            = QStringLiteral(u"Information"),
    X_MB            = QStringLiteral(u"%0 MB"),
    SIZE            = QStringLiteral(u"Size"),
    FILES           = QStringLiteral(u"Files"),
    lFILES          = QStringLiteral(u"files"),
    lFILE           = QStringLiteral(u"file"),
    lFILEc_X        = QStringLiteral(u"%0: %1").arg(lFILE, "%0"),
    X_FILES         = QStringLiteral(u"%0 files"),
    lFILENAME       = QStringLiteral(u"filename"),
    lSYMLINK        = QStringLiteral(u"symbolic link"),
    SHORTCUT        = QStringLiteral(u"Shortcut"),
    lSHORTCUT       = QStringLiteral(u"shortcut"),
    SHORTCUT_X      = X_X.arg(SHORTCUT, "%0"),
    lLOC            = QStringLiteral(u"location"),
    LOC             = QStringLiteral(u"Location"),
    X_SETTING       = QStringLiteral(u"%0 setting"),
    lICON           = QStringLiteral(u"icon"),
    ICON            = QStringLiteral(u"Icon"),

// GENERAL VERBS
    OPEN_X              = QStringLiteral(u"Open %0"),
    lOPEN_X             = QStringLiteral(u"open %0"),
    OPENING_X_FOLDER___ = QStringLiteral(u"Opening %0...").arg(X_FOLDER),
    X_FOLDER_OPENED_    = QStringLiteral(u"%0 opened.").arg(X_FOLDER),
    CREATE_X            = QStringLiteral(u"Create %0"),
    lCREATE_X           = QStringLiteral(u"create %0"),
    lLAUNCHING_X        = QStringLiteral(u"launching %0"),
    LAUNCHING_X___      = QStringLiteral(u"Launching %0..."),
    X_LAUNCHED_         = QStringLiteral(u"%0 launched."),
    lLAUNCH_X           = QStringLiteral(u"launch %0"),
    LAUNCH_X            = QStringLiteral(u"Launch %0"),
    lSET                = QStringLiteral(u"set"),
    SELECT              = QStringLiteral(u"Select"),
    lSELECTED           = QStringLiteral(u"selected"),

//RESULT - SUCCESS
    READY_              = QStringLiteral(u"Ready."),
    X_ENABLED_          = QStringLiteral(u"%0 enabled."),
    X_DISABLED_         = QStringLiteral(u"%0 disabled."),
    lDONE_              = QStringLiteral(u"done."),
    X_FILES_SUCCEEDED   = QStringLiteral(u"%0 succeeded").arg(X_FILES),

//RESULT - ERROR
    dERROR          = QStringLiteral(u"Error"),
    ERRORS_WHILE_X  = QStringLiteral(u"%0(s) occurred while %1").arg(dERROR, "%0"),
    NO_X            = QStringLiteral(u"No %0"),
    NO_X_X          = NO_X.arg(X_X),
    lNO_X_SELECTED  = QStringLiteral(u"no %1 %0").arg(lSELECTED),

    WARNING     = QStringLiteral(u"Warning"),
    lINVALID_X  = QStringLiteral(u"invalid %0"),
    INVALID_X   = QStringLiteral(u"Invalid %0"),
    lEXISTS     = QStringLiteral(u"already exists"),

    X_MISSING       = QStringLiteral(u"%0 missing"),
    MISSING_FILE_X  = QStringLiteral(u"Missing %0").arg(lFILEc_X),
    NO_FILES_TO_X   = NO_X.arg(QStringLiteral(u"%0 to %1")).arg(lFILES),
    X_NOT_FOUND     = QStringLiteral(u"%0 not found"),
    SKIPPING_FILE_X = QStringLiteral(u"--> Skipping %0").arg(lFILEc_X),

    NO_MOD_X_        = QStringLiteral(u"%0.").arg(NO_X_X).arg(lMOD),
    MOD_EXISTS_      = QStringLiteral(u"A %0 with that name %1.").arg(lMOD, lEXISTS),

    FAILED_TO_X                   = QStringLiteral(u"Failed to %0"),
    FAILED_TO_X_                  = QStringLiteral(u"%0.").arg(FAILED_TO_X),
    FAILED_TO_CREATE_BACKUP_X     = FAILED_TO_X.arg(lCREATE_X).arg(QStringLiteral(u"backup of %0")),
    FAILED_TO_CREATE_SHORTCUTc_X_ = FAILED_TO_X_.arg(QStringLiteral(u"%0: %1")).arg(lCREATE_X).arg(lSHORTCUT, "%0"),
    FAILED_TO_OPEN_REGK_          = FAILED_TO_X_.arg(lOPEN_X).arg(QStringLiteral(u"registry key")),
    FAILED_TO_SET_X_              = FAILED_TO_X_.arg(X_X).arg(lSET, "%0"),
    FAILED_TO_GET_X_              = FAILED_TO_X_.arg(QStringLiteral(u"get %0")),
    lFAILED                       = QStringLiteral(u"failed"),
    X_FAILED                      = X_X.arg("%0", lFAILED),
    FILE_NO_ICONS_                = QStringLiteral(u"That %0 contains no icons.").arg(lFILE),

    PLS_REFRESH_ = QStringLiteral(u"Please refresh."),
    CHARACTERS_NOT_ALLOWED = QStringLiteral(u"The following characters aren't allowed:\n< > : \" / \\ | ? *"),

// THREAD ACTIONS
    PROCESSING      = QStringLiteral(u"Processing"),
    ABORT           = QStringLiteral(u"Abort"),
    lABORTED        = QStringLiteral(u"aborted"),
    X_ABORTED       = X_X.arg("%0", lABORTED),
    INVALID_ACTION_ = INVALID_X.arg(QStringLiteral(u"action.")),
    ZERO_MB         = X_MB.arg("0.00"),
    ZERO_FILES      = X_FILES.arg("0"),
    X_BUSY          = QStringLiteral(u"%0 is busy. Try again later."),

    // SCAN
    SCANNING        = QStringLiteral(u"Scanning"),
    SCANNING_X___   = QStringLiteral(u"%0 %1...").arg(SCANNING, "%0"),
    lSCANNING_MODS  = QStringLiteral(u"scanning %0").arg(lMODS),

    // MOUNT
    MOUNTING                  = QStringLiteral(u"Mounting"),
    lMOUNTING                 = QStringLiteral(u"mounting"),
    MOUNTING_X___             = QStringLiteral(u"%0 %1...").arg(MOUNTING, "%0"),
    lMOUNT                    = QStringLiteral(u"mount"),
    MOUNT                     = QStringLiteral(u"Mount"),
    MOUNTED                   = QStringLiteral(u"Mounted"),
    lMOUNTED                  = QStringLiteral(u"mounted"),
    SELECT_MOD_TO_MOUNT_      = QStringLiteral(u"%0 a %1 to %2.").arg(SELECT, lMOD, lMOUNT),
    ALREADY_MOUNTEDc_X_       = QStringLiteral(u"Already %0: %1.").arg(lMOUNTED, "%0"),
    FAILED_TO_FIND_MOUNTED_X_ = FAILED_TO_X_.arg(QStringLiteral(u"find %0 %1: %2")).arg(lMOUNTED, lMOD, "%0"),
    lCREATE_SYMLINK_TO        = QStringLiteral(u"%0 to").arg(lCREATE_X).arg(lSYMLINK),

    // UNMOUNT
    UNMOUNTING              = QStringLiteral(u"Unmounting"),
    UNMOUNTING_X___         = QStringLiteral(u"%0 %1...").arg(UNMOUNTING, "%0"),
    UNMOUNT                 = QStringLiteral(u"Unmount"),
    lUNMOUNT                = QStringLiteral(u"unmount"),

    NOT_A_SYMLINKc_X        = QStringLiteral(u"Not a %0: %1").arg(lSYMLINK, "%0"),
    lRESTORING_BACKUPS___   = QStringLiteral(u"restoring backups..."),
    FORCE_X                 = QStringLiteral(u"Force %0"),

    // ADD
    ADDING      = QStringLiteral(u"Adding"),
    ADDING_X___ = QStringLiteral(u"%0 %1...").arg(ADDING, "%0"),
    ADD_uMOD    = QStringLiteral(u"Add %0").arg(MOD),
    lADD        = QStringLiteral(u"add"),

    lMOVE       = QStringLiteral(u"move"),
    MOVE        = QStringLiteral(u"Move"),
    lCOPY       = QStringLiteral(u"copy"),
    COPY        = QStringLiteral(u"Copy"),

    // DELETE
    DELETING        = QStringLiteral(u"Deleting"),
    dDELETE         = QStringLiteral(u"Delete"),
    DELETING_X___   = QStringLiteral(u"%0 %1...").arg(DELETING),
    lDELETE         = QStringLiteral(u"delete"),
    lDELETE_X       = X_X.arg(lDELETE, "%0"),
    FAILED_TO_DELETE_EMPTY_FOLDERc_X_
                    = FAILED_TO_X_.arg(QStringLiteral(u"%0: %1")).arg(lDELETE_X).arg(X_FOLDER)
                                                                 .arg(QStringLiteral(u"empty"), "%0"),

    // SHORTCUT
    CREATING_SHORTCUT___ = QStringLiteral(u"Creating %0...").arg(lSHORTCUT),
    CREATE_uSHORTCUTS    = CREATE_X.arg(QStringLiteral(u"Shortcuts")),
    SHORTCUT_CREATED_    = SHORTCUT_X.arg(QStringLiteral(u"created.")),

// MOD ACTIONS
    // LAUNCHING
    lARGUMENTS              = QStringLiteral(u"arguments"),
    PROCESSING_ARGUMENTS___ = QStringLiteral(u"%0 %1...").arg(PROCESSING, lARGUMENTS),

    // RENAME
    RENAME          = QStringLiteral(u"Rename"),
    lRENAME_MOD     = QStringLiteral(u"rename %0").arg(lMOD),
    X_RENAMED_X_    = QStringLiteral(u"%0 renamed to %1."),

//DIALOGS
    // MAIN INIT
    STARTING___  = QStringLiteral(u"Starting..."),
    SETUP_UI___         = QStringLiteral(u"Setting up UI..."),
    EXITING___   = QStringLiteral(u"Exiting..."),
    // MAINWINDOW
    REFRESHING___ = QStringLiteral(u"Refreshing..."),
    REFRESHED_    = QStringLiteral(u"%0 refreshed.").arg(WC3MM),
    ALLOW_FILES   = QStringLiteral(u"Allow Local Files"),
    GAME_VERSION  = QStringLiteral(u"Preferred Game Version"),
    // SETTINGS
    SETTINGS           = QStringLiteral(u"Settings"),
    SAVING_SETTINGS___ = QStringLiteral(u"Saving settings..."),
    SETTINGS_SAVED_    = QStringLiteral(u"%0 saved.").arg(SETTINGS),
    BROWSE___          = QStringLiteral(u"Browse..."),
    HIDE_EMPTY         = QStringLiteral(u"Hide Empty %0").arg(MODS),
    // CREATE SHORTCUT
    DONT_SET            = QStringLiteral(u"Don't Set"),
    WC3_CMD_GUIDE       = QStringLiteral(u"%0 Command Line Arguments Guide").arg(WC3),
    SHORTCUT_uNAMEc     = SHORTCUT_X.arg(QStringLiteral(u"Name:")),
    SHORTCUT_uICONc     = SHORTCUT_X.arg(QStringLiteral(u"Icon:")),
    //ABOUT
    ABOUT     = QStringLiteral(u"About"),
    DOWNLOADc = QStringLiteral(u"Download:"),
    SOURCEc   = QStringLiteral(u"Source:"),
    LICENSEc  = QStringLiteral(u"License:"),

    //QUESTIONS
    lYOU_WANT_TO_Xq             = QStringLiteral(u"you want to %0?"),
    DO_WANT_TO_Xq               = QStringLiteral(u"Do %0").arg(lYOU_WANT_TO_Xq),
    CONTINUE_LAUNCHING_GAMEq    = DO_WANT_TO_Xq.arg(QStringLiteral(u"continue %0"))
                                               .arg(lLAUNCHING_X).arg(QStringLiteral(u"the %0")).arg(lGAME),
    ARE_YOU_SURE_Xq             = QStringLiteral(u"Are you sure %0").arg(lYOU_WANT_TO_Xq),

        //ADD MOD
    COPY_MOVEq          = QStringLiteral(u"%0 or %1?").arg(COPY, lMOVE),
    COPY_MOVE_LONGq     = DO_WANT_TO_Xq.arg(QStringLiteral(u"%0 or %1 %2"))
                                       .arg(lCOPY, lMOVE, X_FOLDER.arg(QStringLiteral(u"this"))),
        //DELETE MOD
    PERM_DELETE_Xq      = QStringLiteral(u"Permanently %0?").arg(lDELETE_X),
    PERM_DELETE_X_LONGq = QStringLiteral(u"%0 (%1 / %2)")
                            .arg(ARE_YOU_SURE_Xq).arg(QStringLiteral(u"PERMANENTLY %0")).arg(lDELETE_X),

    //UI STRINGS WITH ALT &SHORTCUT
    aX = QStringLiteral(u"&%0"),

        //MAIN WINDOW
    aFILE               = QStringLiteral(u"&File"),
    aTOOLS              = QStringLiteral(u"&Tools"),
    aHELP               = QStringLiteral(u"&Help"),
    ALLOW_aLOCAL_FILES  = QString(ALLOW_FILES).insert(ALLOW_FILES.indexOf('L'), '&'),
    UNaMOUNT            = QString(UNMOUNT).insert(UNMOUNT.indexOf('m'), '&'),
    aREFRESH            = QStringLiteral(u"&Refresh");

    enum class Ac
         { PROCESSED,                    X_PROCESSED,                                 lPROCESS, lPROCESS_X, Ac_Size };

    typedef std::array<const QString, size_t(Ac::Ac_Size)> ac_t;

    static const std::unordered_map<QString, const ac_t > ac
    ({ { MOUNTING,
         { MOUNTED,                      X_X.arg("%0", lMOUNTED),                     lMOUNT,   X_X.arg(lMOUNT, "%0") } },
       { UNMOUNTING,
         { QStringLiteral(u"Unmounted"), X_X.arg("%0", QStringLiteral(u"unmounted")), lUNMOUNT, X_X.arg(lUNMOUNT, "%0") } },
       { ADDING,
         { QStringLiteral(u"Added"),     X_X.arg("%0", QStringLiteral(u"added")),     lADD,     X_X.arg(lADD, "%0") } },
       { DELETING,
         { QStringLiteral(u"Deleted"),   X_X.arg("%0", QStringLiteral(u"deleted")),   lDELETE,  lDELETE_X } } });
}

#endif // DIC_H
