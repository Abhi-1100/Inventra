#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QDate>
#include <QDateTime>
#include "core/ProductData.h"
#include "core/AuthData.h"

namespace Kirana {

// ─────────────────────────────────────────────
// Database
//
// Wraps Qt6::Sql / SQLite. Call initialize() once
// at startup before accessing any other method.
// All writes are synchronous on the calling thread.
// ─────────────────────────────────────────────

class Database : public QObject {
    Q_OBJECT

public:
    explicit Database(QObject* parent = nullptr);
    ~Database() override;

    // ── Lifecycle ─────────────────────────────
    bool initialize(const QString& dbPath);
    bool isOpen() const;
    QString lastError() const;

    // ── Shop profile ──────────────────────────
    ShopProfile getShop() const;
    bool saveShop(const ShopProfile& shop);
    bool isRegistered() const;   // true if any Owner user exists

    // ── Users / Staff ─────────────────────────
    QVector<StaffUser> getUsers() const;
    StaffUser getUserById(int id) const;
    // Returns the new user's id, or -1 on error
    int  createUser(const QString& name, const QString& role, const QString& pinHash);
    bool updateUserPin(int id, const QString& newPinHash);
    bool updateUserName(int id, const QString& name);
    bool deleteUser(int id);
    // Returns matching user or a default-constructed (id==-1) user on miss
    StaffUser findUserByPinHash(const QString& pinHash) const;

    // ── Product catalog ───────────────────────
    QVector<Product> getProducts() const;
    int  saveProduct(const Product& p);   // insert (id==0) or update
    bool deleteProduct(int id);

    // ── Daily entries ─────────────────────────
    QVector<DailyEntry> getDailyEntries(const QDate& date) const;
    QVector<DailyEntry> getDailyEntriesRange(const QDate& from, const QDate& to) const;
    int  saveDailyEntry(const DailyEntry& e);    // returns new id or -1
    bool updateDailyEntry(const DailyEntry& e);
    bool deleteDailyEntry(int id);

    // ── Stock movements ───────────────────────
    QVector<StockMovement> getStockMovements(
        const QDate& from, const QDate& to,
        const QString& typeFilter = QString()) const;   // typeFilter: "IN","OUT", or empty=all
    QVector<StockMovement> getRecentMovements(int limit = 5) const;
    int  saveStockMovement(const StockMovement& m);   // also updates product current_stock
    bool deleteStockMovement(int id);

    // ── Settings ──────────────────────────────
    QString getSetting(const QString& key, const QString& defaultVal = {}) const;
    bool    setSetting(const QString& key, const QString& value);
    QString getLatestPipelineResults() const;


private:
    bool runMigrations();
    bool execSQL(const QString& sql);

    QString m_dbPath;
    QString m_lastError;
    bool    m_open = false;
    QString m_connectionName;
};

} // namespace Kirana
