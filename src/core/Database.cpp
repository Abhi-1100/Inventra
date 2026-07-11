#include "core/Database.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QCryptographicHash>
#include <QUuid>
#include <QDateTime>
#include <QDebug>

namespace Kirana {

static constexpr int kSchemaVersion = 1;

Database::Database(QObject* parent)
    : QObject(parent)
    , m_connectionName(QUuid::createUuid().toString(QUuid::WithoutBraces))
{}

Database::~Database() {
    if (m_open) {
        QSqlDatabase::removeDatabase(m_connectionName);
        m_open = false;
    }
}

// ─────────────────────────────────────────────
// initialize
// ─────────────────────────────────────────────

bool Database::initialize(const QString& dbPath) {
    m_dbPath = dbPath;

    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        m_lastError = db.lastError().text();
        qCritical() << "[Database] Failed to open:" << m_lastError;
        return false;
    }

    // Enable WAL mode for better concurrent write performance
    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL;"));
    q.exec(QStringLiteral("PRAGMA foreign_keys=ON;"));
    q.exec(QStringLiteral("PRAGMA synchronous=NORMAL;"));

    m_open = true;
    return runMigrations();
}

bool Database::isOpen() const { return m_open; }
QString Database::lastError() const { return m_lastError; }

// ─────────────────────────────────────────────
// execSQL helper
// ─────────────────────────────────────────────

bool Database::execSQL(const QString& sql) {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    if (!q.exec(sql)) {
        m_lastError = q.lastError().text();
        qWarning() << "[Database] SQL error:" << m_lastError;
        qWarning() << "[Database] Query was:" << sql.left(200);
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────
// runMigrations
// ─────────────────────────────────────────────

bool Database::runMigrations() {
    // schema_version table
    if (!execSQL(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS schema_version (version INTEGER PRIMARY KEY);")))
        return false;

    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery vq(QStringLiteral("SELECT version FROM schema_version LIMIT 1;"), db);
    int currentVersion = 0;
    if (vq.next())
        currentVersion = vq.value(0).toInt();

    if (currentVersion >= kSchemaVersion)
        return true;   // already up to date

    // ── Migration 1: initial schema ──────────────
    const QStringList ddl = {
        // Shop profile (single row)
        QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS shop (
            id         INTEGER PRIMARY KEY,
            name       TEXT NOT NULL DEFAULT '',
            owner_name TEXT NOT NULL DEFAULT '',
            phone      TEXT NOT NULL DEFAULT '',
            logo_path  TEXT DEFAULT ''
        );)"),

        // Staff users
        QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS users (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            name       TEXT    NOT NULL,
            role       TEXT    NOT NULL CHECK(role IN ('Owner','Staff')),
            pin_hash   TEXT    NOT NULL,
            created_at TEXT    DEFAULT (datetime('now'))
        );)"),

        // Product catalog
        QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS products (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            sku           TEXT    UNIQUE NOT NULL,
            name          TEXT    NOT NULL,
            category      TEXT    DEFAULT '',
            current_stock INTEGER DEFAULT 0,
            unit_cost     REAL    DEFAULT 0,
            reorder_point INTEGER DEFAULT 10,
            created_at    TEXT    DEFAULT (datetime('now')),
            updated_at    TEXT    DEFAULT (datetime('now'))
        );)"),

        // Daily entries (sold + wasted per product per day)
        QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS daily_entries (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            product_id   INTEGER REFERENCES products(id) ON DELETE CASCADE,
            entry_date   TEXT    NOT NULL,
            units_sold   INTEGER DEFAULT 0,
            units_wasted INTEGER DEFAULT 0,
            entered_by   INTEGER REFERENCES users(id) ON DELETE SET NULL,
            created_at   TEXT    DEFAULT (datetime('now'))
        );)"),

        // Stock movement ledger
        QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS stock_movements (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            product_id      INTEGER REFERENCES products(id) ON DELETE CASCADE,
            movement_type   TEXT    NOT NULL CHECK(movement_type IN ('IN','OUT')),
            quantity        INTEGER NOT NULL,
            supplier_name   TEXT    DEFAULT '',
            cost_per_unit   REAL    DEFAULT 0,
            reason          TEXT    DEFAULT '',
            movement_date   TEXT    NOT NULL,
            entered_by      INTEGER REFERENCES users(id) ON DELETE SET NULL,
            created_at      TEXT    DEFAULT (datetime('now'))
        );)"),

        // ML pipeline results history
        QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS pipeline_results (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            run_at       TEXT    NOT NULL,
            success      INTEGER DEFAULT 1,
            results_json TEXT    DEFAULT ''
        );)"),

        // App settings (key-value)
        QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS app_settings (
            key   TEXT PRIMARY KEY,
            value TEXT
        );)"),
    };

    for (const auto& stmt : ddl) {
        if (!execSQL(stmt)) return false;
    }

    // Insert initial schema version
    QSqlQuery uq(db);
    uq.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO schema_version(version) VALUES(:v);"));
    uq.bindValue(QStringLiteral(":v"), kSchemaVersion);
    if (!uq.exec()) {
        m_lastError = uq.lastError().text();
        return false;
    }

    qInfo() << "[Database] Schema migrated to version" << kSchemaVersion;
    return true;
}

// ─────────────────────────────────────────────
// Shop profile
// ─────────────────────────────────────────────

ShopProfile Database::getShop() const {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(QStringLiteral("SELECT id,name,owner_name,phone,logo_path FROM shop LIMIT 1;"), db);
    ShopProfile s;
    if (q.next()) {
        s.id        = q.value(0).toInt();
        s.name      = q.value(1).toString();
        s.ownerName = q.value(2).toString();
        s.phone     = q.value(3).toString();
        s.logoPath  = q.value(4).toString();
    }
    return s;
}

bool Database::saveShop(const ShopProfile& shop) {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(R"(
        INSERT INTO shop(id,name,owner_name,phone,logo_path)
        VALUES(1,:name,:owner,:phone,:logo)
        ON CONFLICT(id) DO UPDATE SET
            name=excluded.name,
            owner_name=excluded.owner_name,
            phone=excluded.phone,
            logo_path=excluded.logo_path;
    )"));
    q.bindValue(QStringLiteral(":name"),  shop.name);
    q.bindValue(QStringLiteral(":owner"), shop.ownerName);
    q.bindValue(QStringLiteral(":phone"), shop.phone);
    q.bindValue(QStringLiteral(":logo"),  shop.logoPath);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

bool Database::isRegistered() const {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(QStringLiteral(
        "SELECT COUNT(*) FROM users WHERE role='Owner';"), db);
    if (q.next()) return q.value(0).toInt() > 0;
    return false;
}

// ─────────────────────────────────────────────
// Users
// ─────────────────────────────────────────────

static StaffUser rowToUser(QSqlQuery& q) {
    StaffUser u;
    u.id        = q.value(0).toInt();
    u.name      = q.value(1).toString();
    u.role      = q.value(2).toString();
    u.pinHash   = q.value(3).toString();
    u.createdAt = q.value(4).toString();
    return u;
}

QVector<StaffUser> Database::getUsers() const {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(QStringLiteral(
        "SELECT id,name,role,pin_hash,created_at FROM users ORDER BY role DESC,name ASC;"), db);
    QVector<StaffUser> list;
    while (q.next()) list.append(rowToUser(q));
    return list;
}

StaffUser Database::getUserById(int id) const {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id,name,role,pin_hash,created_at FROM users WHERE id=:id;"));
    q.bindValue(QStringLiteral(":id"), id);
    q.exec();
    if (q.next()) return rowToUser(q);
    return {};
}

int Database::createUser(const QString& name, const QString& role, const QString& pinHash) {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO users(name,role,pin_hash) VALUES(:name,:role,:hash);"));
    q.bindValue(QStringLiteral(":name"), name);
    q.bindValue(QStringLiteral(":role"), role);
    q.bindValue(QStringLiteral(":hash"), pinHash);
    if (!q.exec()) { m_lastError = q.lastError().text(); return -1; }
    return q.lastInsertId().toInt();
}

bool Database::updateUserPin(int id, const QString& newPinHash) {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE users SET pin_hash=:hash WHERE id=:id;"));
    q.bindValue(QStringLiteral(":hash"), newPinHash);
    q.bindValue(QStringLiteral(":id"),   id);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

bool Database::updateUserName(int id, const QString& name) {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE users SET name=:name WHERE id=:id;"));
    q.bindValue(QStringLiteral(":name"), name);
    q.bindValue(QStringLiteral(":id"),   id);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

bool Database::deleteUser(int id) {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM users WHERE id=:id AND role!='Owner';"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

StaffUser Database::findUserByPinHash(const QString& pinHash) const {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id,name,role,pin_hash,created_at FROM users WHERE pin_hash=:hash LIMIT 1;"));
    q.bindValue(QStringLiteral(":hash"), pinHash);
    q.exec();
    if (q.next()) return rowToUser(q);
    return {};
}

// ─────────────────────────────────────────────
// Products
// ─────────────────────────────────────────────

QVector<Product> Database::getProducts() const {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(QStringLiteral(R"(
        SELECT id,sku,name,category,current_stock,unit_cost,reorder_point
        FROM products ORDER BY name ASC;)"), db);
    QVector<Product> list;
    while (q.next()) {
        Product p;
        p.id           = q.value(0).toInt();
        p.sku          = q.value(1).toString();
        p.name         = q.value(2).toString();
        p.category     = q.value(3).toString();
        p.currentStock = q.value(4).toInt();
        p.unitCost     = q.value(5).toDouble();
        list.append(p);
    }
    return list;
}

int Database::saveProduct(const Product& p) {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    if (p.id == 0) {
        q.prepare(QStringLiteral(R"(
            INSERT INTO products(sku,name,category,current_stock,unit_cost)
            VALUES(:sku,:name,:cat,:stock,:cost);)"));
    } else {
        q.prepare(QStringLiteral(R"(
            UPDATE products
            SET sku=:sku,name=:name,category=:cat,
                current_stock=:stock,unit_cost=:cost,
                updated_at=datetime('now')
            WHERE id=:id;)"));
        q.bindValue(QStringLiteral(":id"), p.id);
    }
    q.bindValue(QStringLiteral(":sku"),   p.sku);
    q.bindValue(QStringLiteral(":name"),  p.name);
    q.bindValue(QStringLiteral(":cat"),   p.category);
    q.bindValue(QStringLiteral(":stock"), p.currentStock);
    q.bindValue(QStringLiteral(":cost"),  p.unitCost);
    if (!q.exec()) { m_lastError = q.lastError().text(); return -1; }
    return p.id == 0 ? q.lastInsertId().toInt() : p.id;
}

bool Database::deleteProduct(int id) {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM products WHERE id=:id;"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

// ─────────────────────────────────────────────
// Daily entries
// ─────────────────────────────────────────────

static DailyEntry rowToDailyEntry(QSqlQuery& q) {
    DailyEntry e;
    e.id           = q.value(0).toInt();
    e.productId    = q.value(1).toInt();
    e.entryDate    = QDate::fromString(q.value(2).toString(), Qt::ISODate);
    e.unitsSold    = q.value(3).toInt();
    e.unitsWasted  = q.value(4).toInt();
    e.enteredBy    = q.value(5).toInt();
    e.productName  = q.value(6).toString();
    e.productSku   = q.value(7).toString();
    e.createdAt    = QDateTime::fromString(q.value(8).toString(), Qt::ISODate);
    return e;
}

static const char* kDailyEntrySelect = R"(
    SELECT de.id, de.product_id, de.entry_date, de.units_sold, de.units_wasted,
           de.entered_by, p.name, p.sku, de.created_at
    FROM daily_entries de
    LEFT JOIN products p ON p.id = de.product_id
)";

QVector<DailyEntry> Database::getDailyEntries(const QDate& date) const {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QString::fromLatin1(kDailyEntrySelect)
              + QStringLiteral("WHERE de.entry_date=:d ORDER BY de.created_at DESC;"));
    q.bindValue(QStringLiteral(":d"), date.toString(Qt::ISODate));
    q.exec();
    QVector<DailyEntry> list;
    while (q.next()) list.append(rowToDailyEntry(q));
    return list;
}

QVector<DailyEntry> Database::getDailyEntriesRange(const QDate& from, const QDate& to) const {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QString::fromLatin1(kDailyEntrySelect)
              + QStringLiteral("WHERE de.entry_date BETWEEN :f AND :t ORDER BY de.entry_date DESC,de.created_at DESC;"));
    q.bindValue(QStringLiteral(":f"), from.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":t"), to.toString(Qt::ISODate));
    q.exec();
    QVector<DailyEntry> list;
    while (q.next()) list.append(rowToDailyEntry(q));
    return list;
}

int Database::saveDailyEntry(const DailyEntry& e) {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(R"(
        INSERT INTO daily_entries(product_id,entry_date,units_sold,units_wasted,entered_by)
        VALUES(:pid,:date,:sold,:wasted,:by);)"));
    q.bindValue(QStringLiteral(":pid"),    e.productId);
    q.bindValue(QStringLiteral(":date"),   e.entryDate.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":sold"),   e.unitsSold);
    q.bindValue(QStringLiteral(":wasted"), e.unitsWasted);
    q.bindValue(QStringLiteral(":by"),     e.enteredBy > 0 ? QVariant(e.enteredBy) : QVariant());
    if (!q.exec()) { m_lastError = q.lastError().text(); return -1; }
    return q.lastInsertId().toInt();
}

bool Database::updateDailyEntry(const DailyEntry& e) {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(R"(
        UPDATE daily_entries SET units_sold=:sold,units_wasted=:wasted WHERE id=:id;)"));
    q.bindValue(QStringLiteral(":sold"),   e.unitsSold);
    q.bindValue(QStringLiteral(":wasted"), e.unitsWasted);
    q.bindValue(QStringLiteral(":id"),     e.id);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

bool Database::deleteDailyEntry(int id) {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM daily_entries WHERE id=:id;"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

// ─────────────────────────────────────────────
// Stock movements
// ─────────────────────────────────────────────

static StockMovement rowToMovement(QSqlQuery& q) {
    StockMovement m;
    m.id            = q.value(0).toInt();
    m.productId     = q.value(1).toInt();
    m.movementType  = q.value(2).toString();
    m.quantity      = q.value(3).toInt();
    m.supplierName  = q.value(4).toString();
    m.costPerUnit   = q.value(5).toDouble();
    m.reason        = q.value(6).toString();
    m.movementDate  = QDate::fromString(q.value(7).toString(), Qt::ISODate);
    m.enteredBy     = q.value(8).toInt();
    m.productName   = q.value(9).toString();
    m.staffName     = q.value(10).toString();
    m.createdAt     = QDateTime::fromString(q.value(11).toString(), Qt::ISODate);
    return m;
}

static const char* kMovementSelect = R"(
    SELECT sm.id, sm.product_id, sm.movement_type, sm.quantity,
           sm.supplier_name, sm.cost_per_unit, sm.reason, sm.movement_date,
           sm.entered_by, p.name, u.name, sm.created_at
    FROM stock_movements sm
    LEFT JOIN products p ON p.id = sm.product_id
    LEFT JOIN users    u ON u.id = sm.entered_by
)";

QVector<StockMovement> Database::getStockMovements(
    const QDate& from, const QDate& to, const QString& typeFilter) const
{
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    QString sql = QString::fromLatin1(kMovementSelect)
                  + QStringLiteral("WHERE sm.movement_date BETWEEN :f AND :t");
    if (!typeFilter.isEmpty())
        sql += QStringLiteral(" AND sm.movement_type=:type");
    sql += QStringLiteral(" ORDER BY sm.movement_date DESC,sm.created_at DESC;");

    q.prepare(sql);
    q.bindValue(QStringLiteral(":f"), from.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":t"), to.toString(Qt::ISODate));
    if (!typeFilter.isEmpty())
        q.bindValue(QStringLiteral(":type"), typeFilter);

    q.exec();
    QVector<StockMovement> list;
    while (q.next()) list.append(rowToMovement(q));
    return list;
}

QVector<StockMovement> Database::getRecentMovements(int limit) const {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QString::fromLatin1(kMovementSelect)
              + QStringLiteral("ORDER BY sm.created_at DESC LIMIT :n;"));
    q.bindValue(QStringLiteral(":n"), limit);
    q.exec();
    QVector<StockMovement> list;
    while (q.next()) list.append(rowToMovement(q));
    return list;
}

int Database::saveStockMovement(const StockMovement& mv) {
    auto db = QSqlDatabase::database(m_connectionName);

    // Begin transaction: insert movement + update stock
    db.transaction();

    QSqlQuery q(db);
    q.prepare(QStringLiteral(R"(
        INSERT INTO stock_movements
            (product_id,movement_type,quantity,supplier_name,cost_per_unit,reason,movement_date,entered_by)
        VALUES(:pid,:type,:qty,:supplier,:cost,:reason,:date,:by);)"));
    q.bindValue(QStringLiteral(":pid"),      mv.productId);
    q.bindValue(QStringLiteral(":type"),     mv.movementType);
    q.bindValue(QStringLiteral(":qty"),      mv.quantity);
    q.bindValue(QStringLiteral(":supplier"), mv.supplierName);
    q.bindValue(QStringLiteral(":cost"),     mv.costPerUnit);
    q.bindValue(QStringLiteral(":reason"),   mv.reason);
    q.bindValue(QStringLiteral(":date"),     mv.movementDate.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":by"),       mv.enteredBy > 0 ? QVariant(mv.enteredBy) : QVariant());
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        db.rollback();
        return -1;
    }
    int newId = q.lastInsertId().toInt();

    // Update current_stock on the product
    QSqlQuery sq(db);
    if (mv.movementType == QLatin1String("IN")) {
        sq.prepare(QStringLiteral(
            "UPDATE products SET current_stock=current_stock+:qty,updated_at=datetime('now') WHERE id=:pid;"));
    } else {
        sq.prepare(QStringLiteral(
            "UPDATE products SET current_stock=MAX(0,current_stock-:qty),updated_at=datetime('now') WHERE id=:pid;"));
    }
    sq.bindValue(QStringLiteral(":qty"),  mv.quantity);
    sq.bindValue(QStringLiteral(":pid"),  mv.productId);
    if (!sq.exec()) {
        m_lastError = sq.lastError().text();
        db.rollback();
        return -1;
    }

    db.commit();
    return newId;
}

bool Database::deleteStockMovement(int id) {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM stock_movements WHERE id=:id;"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

// ─────────────────────────────────────────────
// Settings
// ─────────────────────────────────────────────

QString Database::getSetting(const QString& key, const QString& defaultVal) const {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT value FROM app_settings WHERE key=:k;"));
    q.bindValue(QStringLiteral(":k"), key);
    q.exec();
    if (q.next()) return q.value(0).toString();
    return defaultVal;
}

bool Database::setSetting(const QString& key, const QString& value) {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO app_settings(key,value) VALUES(:k,:v) ON CONFLICT(key) DO UPDATE SET value=excluded.value;"));
    q.bindValue(QStringLiteral(":k"), key);
    q.bindValue(QStringLiteral(":v"), value);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

QString Database::getLatestPipelineResults() const {
    auto db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT results_json FROM pipeline_results WHERE success=1 ORDER BY run_at DESC LIMIT 1;"));
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return QString();
}

} // namespace Kirana

