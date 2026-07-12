#pragma once

#include <QObject>
#include <QString>
#include "core/AuthData.h"
#include "core/Database.h"
#include <QDateTime>

namespace Kirana {

// ─────────────────────────────────────────────
// AuthController
//
// Manages shop registration, PIN-based login,
// staff CRUD, and current session user.
// ─────────────────────────────────────────────

class AuthController : public QObject {
    Q_OBJECT

public:
    explicit AuthController(Database* db, QObject* parent = nullptr);

    // ── State ─────────────────────────────────
    bool isRegistered() const;
    bool isLoggedIn()   const { return m_currentUser.isValid(); }

    const StaffUser& currentUser() const { return m_currentUser; }
    const ShopProfile& shopProfile() const { return m_shop; }
    QDateTime loginTime() const { return m_loginTime; }

    // ── Registration ──────────────────────────
    // Returns true on success; emits registrationSucceeded
    bool registerShop(const ShopProfile& shop, const QString& ownerName, const QString& pin);

    // ── Login ─────────────────────────────────
    // Returns true on success; emits loginSucceeded / loginFailed
    bool authenticate(const QString& pin);
    void logout();
    void lockSession();
    bool unlockSession(const QString& pin);

    // ── Staff management (Owner only) ─────────
    QVector<StaffUser> getStaff() const;
    bool addStaff(const QString& name, const QString& pin);
    bool updateStaffPin(int userId, const QString& newPin);
    bool updateStaffName(int userId, const QString& newName);
    bool removeStaff(int userId);

    // ── Shop profile ──────────────────────────
    bool updateShopProfile(const ShopProfile& shop);

    // ── Utilities ─────────────────────────────
    static QString hashPin(const QString& pin);

signals:
    void registrationSucceeded(const Kirana::StaffUser& user);
    void loginSucceeded(const Kirana::StaffUser& user);
    void loginFailed();
    void loggedOut();
    void sessionLocked();
    void sessionUnlocked();
    void staffChanged();
    void shopProfileChanged(const Kirana::ShopProfile& shop);

private:
    Database*   m_db;
    StaffUser   m_currentUser;
    ShopProfile m_shop;
    QDateTime   m_loginTime;
};

} // namespace Kirana
