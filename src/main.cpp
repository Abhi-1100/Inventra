#include <QApplication>
#include <QFile>
#include <QDir>
#include <QStyleFactory>
#include "core/Database.h"
#include "core/AppController.h"
#include "core/AuthController.h"
#include "ui/MainWindow.h"
#include "core/ThemeManager.h"
#include "python/PythonBridge.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set styling hints
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // Initialize ThemeManager to apply the default theme
    Kirana::ThemeManager::instance();

    // Initialize Database
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("inventra.db"));
    Kirana::Database db;
    if (!db.initialize(dbPath)) {
        // Fallback to in-memory db if write fails
        db.initialize(QStringLiteral(":memory:"));
    }

    // Setup core controllers & Auth
    Kirana::AppController controller(&db);
    controller.settings().dbPath = dbPath;
    Kirana::AuthController auth(&db);

    // Load actual product database list (seeds defaults if first run)
    controller.loadFromDatabase();

    // Initialize Python bridge if embedded pipeline mode is compiled/enabled
    QString pythonPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("python"));
    Kirana::PythonBridge::instance().initialize(pythonPath);

    // Setup main GUI
    Kirana::MainWindow w(&controller, &auth);
    w.show();

    int result = app.exec();

    // Clean up python interpreter
    Kirana::PythonBridge::instance().shutdown();

    return result;
}
