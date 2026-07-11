#pragma once

#include <QWidget>
#include "core/AppController.h"

class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QLabel;

namespace Kirana {

class AppController;

// ─────────────────────────────────────────────
// SettingsWidget
//
// Form fields for EOQ parameters and threshold levels.
// ─────────────────────────────────────────────

class SettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWidget(AppController* controller, QWidget* parent = nullptr);
    ~SettingsWidget() override = default;

signals:
    void settingsChanged(const Kirana::AppSettings& settings);
    void reRunRequested();

private slots:
    void onSaveClicked();
    void onResetClicked();
    void onRunNowClicked();
    void updateFormFromSettings();

private:
    void buildLayout();

    AppController* m_controller = nullptr;

    QDoubleSpinBox* m_orderingCostSpin  = nullptr;
    QDoubleSpinBox* m_holdingCostSpin   = nullptr;
    QSpinBox*       m_leadTimeSpin      = nullptr;
    QDoubleSpinBox* m_threshHighSpin    = nullptr;
    QDoubleSpinBox* m_threshMediumSpin  = nullptr;

    QPushButton* m_saveBtn   = nullptr;
    QPushButton* m_resetBtn  = nullptr;
    QPushButton* m_runBtn    = nullptr;

    QLabel* m_defaultBanner  = nullptr;
};

} // namespace Kirana
