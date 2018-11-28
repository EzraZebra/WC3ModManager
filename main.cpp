#include "_dic.h"
#include "core.h"
#include "launcher.h"
#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setAttribute(Qt::AA_DisableWindowContextHelpButton);

    qRegisterMetaType<mod_t>("mod_t");
    qRegisterMetaType<mod_m>("mod_m");
    qRegisterMetaType<Msgr::Type>("Msgr::Type");

    Core core;

    const QStringList args = a.arguments();
    if(args.size() > 1)
    {
        QCommandLineParser parser;
        parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
        parser.addHelpOption();
        parser.addOptions({
            QCommandLineOption({ d::C_LAUNCH },
                               QStringLiteral(u"%0 to %1. Enter \"%2\" (including double-quotes) to %1 without %3 (default).")
                                .arg(d::MOD, d::C_LAUNCH, d::L_NONE, d::lMOD),
                               d::lMOD),
            QCommandLineOption({ d::C_VERSION },
                               QStringLiteral(u"%0: [%1|%2|%3].")
                                .arg(d::GAME_VERSION, d::V_CLASSIC, d::V_EXPANSION, d::V_WE),
                               d::C_VERSION),
            QCommandLineOption({ d::C_NATIVE },
                               QStringLiteral(u"Supply %0 %1 command line %2. Ignored when %3.")
                                .arg(d::C_NATIVE, d::WC3_EXE, d::lARGUMENTS, d::lLAUNCHING_X.arg(d::WE)),
                               d::lARGUMENTS)
        });
        parser.parse(args);

        if(!parser.value(d::C_LAUNCH).isEmpty() || !parser.value(d::C_VERSION).isEmpty()
                || !parser.value(d::C_NATIVE).isEmpty())
        {
            Launcher l(&core, parser.value(d::C_LAUNCH), parser.value(d::C_VERSION), parser.value(d::C_NATIVE));

            return a.exec();
        }
    }

    MainWindow w(&core);
    w.show();

    return a.exec();
}
