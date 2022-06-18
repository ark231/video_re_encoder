#include <QApplication>
#include <QLocale>
#include <QTranslator>

#include "mainwindow.hpp"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();  // NOLINT(readability-identifier-naming)
    for (const QString &locale : uiLanguages) {
        const QString baseName = "video_cutter_" + QLocale(locale).name();  // NOLINT(readability-identifier-naming)
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}
