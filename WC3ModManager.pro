#-------------------------------------------------
#
# Project created by QtCreator 2018-09-04T07:18:19
#
#-------------------------------------------------
QT       += core gui widgets winextras

TARGET = WC3ModManager
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# !! Ordered like this: 1, 10, 11, 12, 2, 3, ...
RC_ICONS +=  \
    icons/icon.ico \        # 1 [0], 10-12 [1-3]
    icons/war2.ico \        # 2-9 [4-11]
    icons/war3d.ico \
    icons/war3.ico \
    icons/war3x.ico \
    icons/worldedit.ico \
    icons/war3z.ico \
    icons/wow.ico \
    icons/hive.ico \
    icons/war3_mod.ico \    # 10-12 [1-3]
    icons/war3x_mod.ico \
    icons/worldedit_mod.ico

SOURCES += \
    config.cpp \
    main.cpp \
    settings.cpp \
    thread.cpp \
    shortcuts.cpp \
    core.cpp \
    launcher.cpp \
    mainwindow.cpp

HEADERS += \
    config.h \
    settings.h \
    thread.h \
    shortcuts.h \
    shortcuts_pvt.h \
    thread_pvt.h \
    threadbase.h \
    _dic.h \
    _utils.h \
    launcher.h \
    mainwindow.h \
    core.h \
    _msgr.h \
    _moddata.h \
    _uo_map_qs.h

RESOURCES += \
    icons.qrc \
    img.qrc
