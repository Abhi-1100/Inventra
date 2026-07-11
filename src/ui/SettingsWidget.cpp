#include "ui/SettingsWidget.h"
#include "core/AppController.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QFrame>

namespace Kirana {

SettingsWidget::SettingsWidget(AppController* controller, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
{
    buildLayout();
    updateFormFromSettings();
}

void SettingsWidget::buildLayout() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Title
    auto* titleLabel = new QLabel("GLOBAL PIPELINE SETTINGS", this);
    titleLabel->setStyleSheet("font-family: 'Segoe UI', sans-serif; font-size: 16px; font-weight: bold; color: #e6edf3; letter-spacing: 0.5px;");
    mainLayout->addWidget(titleLabel);

    // Default Banner
    m_defaultBanner = new QLabel("⚠️ APPLICATION IS CURRENTLY RUNNING WITH DEFAULT CONFIGURATION PARAMETERS", this);
    m_defaultBanner->setStyleSheet("font-size: 10px; font-weight: bold; color: #d29922; background-color: rgba(210,153,34,0.12); padding: 8px; border-radius: 4px; border: 1px solid rgba(210,153,34,0.30);");
    mainLayout->addWidget(m_defaultBanner);

    // ── Grid Container ───────────────────────────
    auto* formFrame = new QFrame(this);
    formFrame->setObjectName("FormFrame");
    formFrame->setStyleSheet(
        "QFrame#FormFrame {"
        "  background: #161b22;"
        "  border: 1px solid #30363d;"
        "  border-radius: 6px;"
        "  padding: 20px;"
        "}"
        "QLabel { color: #8b949e; font-size: 11px; }"
        "QDoubleSpinBox, QSpinBox {"
        "  background-color: #0d1117;"
        "  border: 1px solid #30363d;"
        "  border-radius: 4px;"
        "  padding: 6px;"
        "  color: #e6edf3;"
        "  font-family: 'Consolas', monospace;"
        "}"
    );
    auto* grid = new QGridLayout(formFrame);
    grid->setSpacing(16);

    // Cost Assumptions
    auto* eoqHeader = new QLabel("ECONOMIC ORDER QUANTITY (EOQ) VARIABLES", formFrame);
    eoqHeader->setStyleSheet("font-weight: bold; color: #58a6ff; font-size: 11px;");
    grid->addWidget(eoqHeader, 0, 0, 1, 2);

    grid->addWidget(new QLabel("Ordering Cost per Shipment (K):", formFrame), 1, 0);
    m_orderingCostSpin = new QDoubleSpinBox(formFrame);
    m_orderingCostSpin->setPrefix("₹ ");
    m_orderingCostSpin->setRange(1.0, 10000.0);
    grid->addWidget(m_orderingCostSpin, 1, 1);

    grid->addWidget(new QLabel("Annual Holding Cost Rate (h %):", formFrame), 2, 0);
    m_holdingCostSpin = new QDoubleSpinBox(formFrame);
    m_holdingCostSpin->setSuffix(" %");
    m_holdingCostSpin->setRange(1.0, 100.0);
    m_holdingCostSpin->setDecimals(1);
    grid->addWidget(m_holdingCostSpin, 2, 1);

    grid->addWidget(new QLabel("Supplier Shipment Lead Time:", formFrame), 3, 0);
    m_leadTimeSpin = new QSpinBox(formFrame);
    m_leadTimeSpin->setSuffix(" days");
    m_leadTimeSpin->setRange(1, 90);
    grid->addWidget(m_leadTimeSpin, 3, 1);

    // Separator line
    auto* line = new QFrame(formFrame);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #30363d; max-height: 1px; border: none;");
    grid->addWidget(line, 4, 0, 1, 2);

    // ML Thresholds
    auto* mlHeader = new QLabel("CLASSIFICATION DECISION THRESHOLDS", formFrame);
    mlHeader->setStyleSheet("font-weight: bold; color: #58a6ff; font-size: 11px;");
    grid->addWidget(mlHeader, 5, 0, 1, 2);

    grid->addWidget(new QLabel("Critical / Reorder Confidence Threshold:", formFrame), 6, 0);
    m_threshHighSpin = new QDoubleSpinBox(formFrame);
    m_threshHighSpin->setSuffix(" %");
    m_threshHighSpin->setRange(10.0, 100.0);
    m_threshHighSpin->setDecimals(1);
    grid->addWidget(m_threshHighSpin, 6, 1);

    grid->addWidget(new QLabel("Medium / Warning Confidence Threshold:", formFrame), 7, 0);
    m_threshMediumSpin = new QDoubleSpinBox(formFrame);
    m_threshMediumSpin->setSuffix(" %");
    m_threshMediumSpin->setRange(10.0, 100.0);
    m_threshMediumSpin->setDecimals(1);
    grid->addWidget(m_threshMediumSpin, 7, 1);

    mainLayout->addWidget(formFrame);

    // ── Bottom Action Buttons ──────────────────
    auto* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(12);

    m_resetBtn = new QPushButton("Restore Defaults", this);
    m_resetBtn->setStyleSheet(
        "QPushButton {"
        "  background: transparent;"
        "  border: 1px solid #30363d;"
        "  color: #c9d1d9;"
        "  padding: 8px 16px;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background: #21262d; }"
    );
    connect(m_resetBtn, &QPushButton::clicked, this, &SettingsWidget::onResetClicked);

    m_saveBtn = new QPushButton("Apply & Save Config", this);
    m_saveBtn->setStyleSheet(
        "QPushButton {"
        "  background: #21262d;"
        "  border: 1px solid #30363d;"
        "  color: #58a6ff;"
        "  padding: 8px 16px;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background: #30363d; }"
    );
    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsWidget::onSaveClicked);

    m_runBtn = new QPushButton("▶ Re-run Pipeline", this);
    m_runBtn->setStyleSheet(
        "QPushButton {"
        "  background: rgba(88,166,255,0.15);"
        "  border: 1px solid rgba(88,166,255,0.40);"
        "  color: #58a6ff;"
        "  padding: 8px 16px;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background: rgba(88,166,255,0.25); }"
    );
    connect(m_runBtn, &QPushButton::clicked, this, &SettingsWidget::onRunNowClicked);

    actionLayout->addWidget(m_resetBtn);
    actionLayout->addStretch();
    actionLayout->addWidget(m_runBtn);
    actionLayout->addWidget(m_saveBtn);

    mainLayout->addLayout(actionLayout);
}

void SettingsWidget::updateFormFromSettings() {
    const auto& s = m_controller->settings();
    m_orderingCostSpin->setValue(s.orderingCost);
    m_holdingCostSpin->setValue(s.holdingCostRate * 100.0);
    m_leadTimeSpin->setValue(s.leadTimeDays);
    m_threshHighSpin->setValue(s.reorderThreshHigh * 100.0);
    m_threshMediumSpin->setValue(s.reorderThreshMedium * 100.0);

    // Hide banner if custom cost assumptions are saved
    bool usingDefaults = (s.orderingCost == 20.0 && s.holdingCostRate == 0.25);
    m_defaultBanner->setVisible(usingDefaults);
}

void SettingsWidget::onSaveClicked() {
    AppSettings s;
    s.orderingCost = m_orderingCostSpin->value();
    s.holdingCostRate = m_holdingCostSpin->value() / 100.0;
    s.leadTimeDays = m_leadTimeSpin->value();
    s.reorderThreshHigh = m_threshHighSpin->value() / 100.0;
    s.reorderThreshMedium = m_threshMediumSpin->value() / 100.0;

    emit settingsChanged(s);
    updateFormFromSettings();
}

void SettingsWidget::onResetClicked() {
    AppSettings s; // Defaults
    emit settingsChanged(s);
    updateFormFromSettings();
}

void SettingsWidget::onRunNowClicked() {
    emit reRunRequested();
}

} // namespace Kirana
