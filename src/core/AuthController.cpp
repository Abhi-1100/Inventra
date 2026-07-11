#include "core/AuthController.h"
#include <QCryptographicHash>
#include <QDebug>

namespace Kirana {

AuthController::AuthController(Database* db, QObject* parent)
    : QObject(parent)
    , m_db(db)
{
    // Cache shop profile on startup
    m_shop = m_db->getShop();

    qRegisterMetaType<Kirana::StaffUser>();
    qRegisterMetaType<Kirana::ShopProfile>();
    qRegisterMetaType<QVector<Kirana::StaffUser>>();
}

// ─────────────────────────────────────────────
// hashPin — SHA-256 hex of the 4-digit PIN
// ─────────────────────────────────────────────

QString AuthController::hashPin(const QString& pin) {
    return QString::fromLatin1(
        QCryptographicHash::hash(pin.toUtf8(), QCryptographicHash::Sha256).toHex());
}

// ─────────────────────────────────────────────
// State
// ─────────────────────────────────────────────

bool AuthController::isRegistered() const {
    return m_db->isRegistered();
}

// ─────────────────────────────────────────────
// registerShop
// ─────────────────────────────────────────────

bool AuthController::registerShop(const ShopProfile& shop,
                                   const QString& ownerName,
                                   const QString& pin)
{
    if (pin.length() != 4) {
        qWarning() << "[AuthController] PIN must be 4 digits";
        return false;
    }

    // Save shop profile
    if (!m_db->saveShop(shop)) {
        qWarning() << "[AuthController] Failed to save shop:" << m_db->lastError();
        return false;
    }

    // Create owner user
    const QString hash = hashPin(pin);
    int userId = m_db->createUser(ownerName.isEmpty() ? shop.ownerName : ownerName,
                                   QStringLiteral("Owner"),
                                   hash);
    if (userId < 0) {
        qWarning() << "[AuthController] Failed to create owner:" << m_db->lastError();
        return false;
    }

    // Update cached shop
    m_shop = m_db->getShop();

    // Auto-login the newly registered owner
    m_currentUser = m_db->getUserById(userId);
    emit registrationSucceeded(m_currentUser);
    emit loginSucceeded(m_currentUser);
    emit shopProfileChanged(m_shop);

    return true;
}

// ─────────────────────────────────────────────
// authenticate
// ─────────────────────────────────────────────

bool AuthController::authenticate(const QString& pin) {
    const QString hash = hashPin(pin);
    StaffUser user = m_db->findUserByPinHash(hash);

    if (!user.isValid()) {
        emit loginFailed();
        return false;
    }

    m_currentUser = user;
    emit loginSucceeded(m_currentUser);
    return true;
}

// ─────────────────────────────────────────────
// logout
// ─────────────────────────────────────────────

void AuthController::logout() {
    m_currentUser = {};
    emit loggedOut();
}

// ─────────────────────────────────────────────
// Staff management
// ─────────────────────────────────────────────

QVector<StaffUser> AuthController::getStaff() const {
    return m_db->getUsers();
}

bool AuthController::addStaff(const QString& name, const QString& pin) {
    if (pin.length() != 4) return false;
    const QString hash = hashPin(pin);
    int id = m_db->createUser(name, QStringLiteral("Staff"), hash);
    if (id < 0) return false;
    emit staffChanged();
    return true;
}

bool AuthController::updateStaffPin(int userId, const QString& newPin) {
    if (newPin.length() != 4) return false;
    const QString hash = hashPin(newPin);
    if (!m_db->updateUserPin(userId, hash)) return false;
    emit staffChanged();
    return true;
}

bool AuthController::updateStaffName(int userId, const QString& newName) {
    if (!m_db->updateUserName(userId, newName)) return false;
    emit staffChanged();
    return true;
}

bool AuthController::removeStaff(int userId) {
    if (!m_db->deleteUser(userId)) return false;
    emit staffChanged();
    return true;
}

// ─────────────────────────────────────────────
// Shop profile
// ─────────────────────────────────────────────

bool AuthController::updateShopProfile(const ShopProfile& shop) {
    if (!m_db->saveShop(shop)) return false;
    m_shop = shop;
    emit shopProfileChanged(m_shop);
    return true;
}

} // namespace Kirana
