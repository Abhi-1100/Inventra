#include <QApplication>
#include <QFile>
#include <QDir>
#include <QStyleFactory>
#include "core/AppController.h"
#include "ui/MainWindow.h"
#include "python/PythonBridge.h"

int main(int argc, char *argv[]) {
    // Enable high-DPI scaling if supported in Qt6 (enabled by default)
    QApplication app(argc, argv);

    // Set styling hints
    app.setStyle(QStyleFactory::create("Fusion"));

    // Load global QSS Dark Theme
    QFile styleFile(":/styles/terminal.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
    }

    // Initialize Python bridge if embedded pipeline mode is compiled/enabled
    // Pass application directory /python to search path
    QString pythonPath = QDir(QCoreApplication::applicationDirPath()).filePath("python");
    Kirana::PythonBridge::instance().initialize(pythonPath);

    // Setup core controller & main GUI
    Kirana::AppController controller;
    Kirana::MainWindow w(&controller);
    w.show();

    int result = app.exec();

    // Clean up python interpreter
    Kirana::PythonBridge::instance().shutdown();

    return result;
}
