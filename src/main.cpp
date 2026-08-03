#include <QApplication>
#include <QFile>
#include <QDir>
#include <QStyleFactory>
#include <QStandardPaths>
#include <QFileInfo>
#include "core/Database.h"
#include "core/AppController.h"
#include "core/AuthController.h"
#include "ui/MainWindow.h"
#include "core/ThemeManager.h"
#include "python/PythonBridge.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Inventra"));
    app.setOrganizationName(QStringLiteral("Inventra"));

    // Set styling hints
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // Initialize ThemeManager to apply the default theme
    Kirana::ThemeManager::instance();

    // ── Database path resolution ──────────────────────────────────────────
    // 1. Check applicationDirPath (works inside .app/Contents/MacOS and plain binary)
    // 2. Fallback: check parent of applicationDirPath (build root when running plain binary during development)
    // 3. Fallback: use app writable data location
    auto tryDbPath = [](const QString& dir) -> QString {
        QString p = QDir(dir).filePath(QStringLiteral("inventra.db"));
        if (QFileInfo::exists(p)) return p;
        return QString();
    };

    QString dbPath = tryDbPath(QCoreApplication::applicationDirPath());
    if (dbPath.isEmpty()) {
        dbPath = tryDbPath(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("..")));
    }
    if (dbPath.isEmpty()) {
        // Not found anywhere — use applicationDirPath so db gets created there
        dbPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("inventra.db"));
    }
    qInfo() << "[main] Database path:" << dbPath;

    Kirana::Database db;
    if (!db.initialize(dbPath)) {
        db.initialize(QStringLiteral(":memory:"));
    }

    // Setup core controllers & Auth
    Kirana::AppController controller(&db);
    controller.settings().dbPath = dbPath;
    Kirana::AuthController auth(&db);

    // Load actual product database list (seeds defaults if first run)
    controller.loadFromDatabase();

    // Initialize Python bridge — look in applicationDirPath first, then parent
    QString pythonDir = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("python"));
    if (!QDir(pythonDir).exists()) {
        pythonDir = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../python"));
    }
    Kirana::PythonBridge::instance().initialize(pythonDir);

    // Setup main GUI
    Kirana::MainWindow w(&controller, &auth);
    w.show();

    int result = app.exec();

    // Clean up python interpreter
    Kirana::PythonBridge::instance().shutdown();

    return result;
}

