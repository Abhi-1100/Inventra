#pragma once

#include <QWidget>
#include "core/AppController.h"
#include "core/AuthData.h"

class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QListWidget;

namespace Kirana {

class AppController;
class AuthController;

// ─────────────────────────────────────────────
// SettingsWidget
//
// Form fields for EOQ parameters, shop profile,
// and staff PIN management (Owner only).
// ─────────────────────────────────────────────

class SettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWidget(AppController* controller,
                            AuthController* auth,
                            QWidget* parent = nullptr);
    ~SettingsWidget() override = default;

signals:
    void settingsChanged(const Kirana::AppSettings& settings);
    void reRunRequested();

private slots:
    void onSaveClicked();
    void onResetClicked();
    void onRunNowClicked();
    void updateFormFromSettings();

    // Staff slots
    void refreshStaffList();
    void onAddStaffClicked();
    void onDeleteStaffClicked();

    // Shop slots
    void onSaveShopClicked();
    void onSelectLogoClicked();

private:
    void buildLayout();

    AppController*  m_controller = nullptr;
    AuthController* m_auth       = nullptr;

    // EOQ Settings
    QDoubleSpinBox* m_orderingCostSpin  = nullptr;
    QDoubleSpinBox* m_holdingCostSpin   = nullptr;
    QSpinBox*       m_leadTimeSpin      = nullptr;
    QDoubleSpinBox* m_threshHighSpin    = nullptr;
    QDoubleSpinBox* m_threshMediumSpin  = nullptr;

    QPushButton* m_saveBtn   = nullptr;
    QPushButton* m_resetBtn  = nullptr;
    QPushButton* m_runBtn    = nullptr;

    QLabel* m_defaultBanner  = nullptr;

    // Shop Profile fields
    QLineEdit* m_shopNameEdit  = nullptr;
    QLineEdit* m_ownerNameEdit = nullptr;
    QLineEdit* m_phoneEdit     = nullptr;
    QLabel*    m_logoPathLabel = nullptr;
    QString    m_logoPath;

    // Staff Management fields
    QListWidget* m_staffListWidget = nullptr;
    QPushButton* m_addStaffBtn     = nullptr;
    QPushButton* m_deleteStaffBtn  = nullptr;
};

} // namespace Kirana
