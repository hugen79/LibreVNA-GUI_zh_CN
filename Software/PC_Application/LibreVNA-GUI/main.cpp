#include "appwindow.h"
#include <QtWidgets/QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#ifdef Q_OS_UNIX
#include <signal.h>
#endif

static QApplication *app;
static AppWindow *window;

#ifdef Q_OS_UNIX
static void tryExitGracefully(int s) {
    Q_UNUSE(s)
    window->close();
    app->quit();
}
#endif

// Loads application and Qt base translations.
// Priority:
//   1. --lang <locale> command line override (e.g. "zh_CN", "en_US")
//   2. QLocale::system()
static void installTranslations(QApplication &application)
{
    QString lang;
    const QStringList args = application.arguments();
    const int idx = args.indexOf(QStringLiteral("--lang"));
    if (idx >= 0 && idx + 1 < args.size())
        lang = args.at(idx + 1);

    const QLocale locale = lang.isEmpty() ? QLocale::system() : QLocale(lang);

    // Application translations: <appDir>/translations or :/translations
    const QStringList searchPaths = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/translations"),
        QStringLiteral(":/translations"),
    };

    auto *appTranslator = new QTranslator(&application);
    for (const QString &dir : searchPaths) {
        if (appTranslator->load(locale, QStringLiteral("LibreVNA"), QStringLiteral("_"), dir)) {
            application.installTranslator(appTranslator);
            break;
        }
    }

    // Qt's own translations (standard buttons, QMessageBox, ...).
    // Also look next to the app so it works on machines without a Qt install.
    auto *qtTranslator = new QTranslator(&application);
    QStringList qtPaths = searchPaths;
    qtPaths << QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    for (const QString &dir : qtPaths) {
        if (qtTranslator->load(locale, QStringLiteral("qtbase"), QStringLiteral("_"), dir)) {
            application.installTranslator(qtTranslator);
            break;
        }
    }
}

int main(int argc, char *argv[]) {

    qSetMessagePattern("%{time process}: [f{type}] %{message}");

    app = new QApplication(argc, argv);
    QCoreApplication::setOrganizationName("LibreVNA");
    QCoreApplication::setApplicationName("LibreVNA-GUI");
    installTranslations(*app);
    window = new AppWindow;
    QCoreApplication::setApplicationVersion(window->getAppVersion() + "-" +
                                            window->getAppGitHash().left(9));

#ifdef Q_OS_UNIX
    signal(SIGINT, tryExitGracefully);
#endif
    auto rc = app->exec();
    return rc;
}
