#pragma once

#include <QString>
#include <QMetaType>

namespace Kirana {

// ─────────────────────────────────────────────
// ShopProfile
// ─────────────────────────────────────────────

struct ShopProfile {
    int     id        = 0;
    QString name;
    QString ownerName;
    QString phone;
    QString logoPath;
};

// ─────────────────────────────────────────────
// StaffUser
// ─────────────────────────────────────────────

struct StaffUser {
    int     id      = -1;        // -1 = not found / invalid
    QString name;
    QString role;                // "Owner" or "Staff"
    QString pinHash;             // SHA-256 hex
    QString createdAt;

    bool isValid()  const { return id > 0; }
    bool isOwner()  const { return role == QLatin1String("Owner"); }
    bool isStaff()  const { return role == QLatin1String("Staff"); }
};

} // namespace Kirana

Q_DECLARE_METATYPE(Kirana::ShopProfile)
Q_DECLARE_METATYPE(Kirana::StaffUser)
Q_DECLARE_METATYPE(QVector<Kirana::StaffUser>)
