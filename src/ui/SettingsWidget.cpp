#include "ui/SettingsWidget.h"
#include "core/AppController.h"
#include "core/AuthController.h"
#include "core/Database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QListWidget>
#include <QFrame>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QTabWidget>
#include <QComboBox>
#include "core/ThemeManager.h"

namespace Kirana {

SettingsWidget::SettingsWidget(AppController* controller, AuthController* auth, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_auth(auth)
{
    buildLayout();
    updateFormFromSettings();
    refreshStaffList();

    connect(m_auth, &AuthController::staffChanged, this, &SettingsWidget::refreshStaffList);
}

void SettingsWidget::buildLayout() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Title
    auto* titleLabel = new QLabel(QStringLiteral("Settings"), this);
    titleLabel->setStyleSheet(QStringLiteral(
        "font-family:'Hanken Grotesk','Segoe UI',sans-serif;"
        "font-size:28px;font-weight:700;color:#e0e2ea;"
        "background:transparent;border:none;"));
    mainLayout->addWidget(titleLabel);

    // ── Tab Control for Settings Categories ──
    auto* tabWidget = new QTabWidget(this);
    tabWidget->setDocumentMode(true);

    auto makeFieldLabel = [](const QString& text) -> QLabel* {
        auto* l = new QLabel(text);
        l->setStyleSheet(QStringLiteral(
            "color:#8c90a0;font-size:11px;font-weight:600;"
            "letter-spacing:0.05em;background:transparent;border:none;"));
        return l;
    };

    // ── TAB 1: Pipeline Parameters ──
    auto* pipelineTab = new QWidget(tabWidget);
    auto* pLayout = new QVBoxLayout(pipelineTab);
    pLayout->setContentsMargins(12, 12, 12, 12);
    pLayout->setSpacing(16);

    m_defaultBanner = new QLabel(QStringLiteral("⚠  APPLICATION IS CURRENTLY RUNNING WITH DEFAULT CONFIGURATION PARAMETERS"), pipelineTab);
    m_defaultBanner->setStyleSheet(QStringLiteral(
        "font-size:12px;font-weight:600;color:#c0c7d3;"
        "background-color:rgba(192,199,211,0.08);"
        "padding:10px 14px;border-radius:6px;"
        "border:1px solid rgba(192,199,211,0.25);"
        "border-left:3px solid #c0c7d3;"));
    pLayout->addWidget(m_defaultBanner);

    auto* formFrame = new QFrame(pipelineTab);
    formFrame->setObjectName(QStringLiteral("FormFrame"));
    auto* grid = new QGridLayout(formFrame);
    grid->setSpacing(16);

    auto* eoqHeader = new QLabel(QStringLiteral("ECONOMIC ORDER QUANTITY (EOQ) VARIABLES"), formFrame);
    eoqHeader->setStyleSheet(QStringLiteral(
        "font-family:'Hanken Grotesk',sans-serif;"
        "font-weight:700;color:#afc6ff;font-size:11px;"
        "letter-spacing:0.06em;background:transparent;border:none;"));
    grid->addWidget(eoqHeader, 0, 0, 1, 2);

    grid->addWidget(makeFieldLabel(QStringLiteral("Ordering Cost per Shipment (K):")), 1, 0);
    m_orderingCostSpin = new QDoubleSpinBox(formFrame);
    m_orderingCostSpin->setPrefix(QStringLiteral("₹ "));
    m_orderingCostSpin->setRange(1.0, 10000.0);
    grid->addWidget(m_orderingCostSpin, 1, 1);

    grid->addWidget(makeFieldLabel(QStringLiteral("Annual Holding Cost Rate (h %):")), 2, 0);
    m_holdingCostSpin = new QDoubleSpinBox(formFrame);
    m_holdingCostSpin->setSuffix(QStringLiteral(" %"));
    m_holdingCostSpin->setRange(1.0, 100.0);
    m_holdingCostSpin->setDecimals(1);
    grid->addWidget(m_holdingCostSpin, 2, 1);

    grid->addWidget(makeFieldLabel(QStringLiteral("Supplier Shipment Lead Time:")), 3, 0);
    m_leadTimeSpin = new QSpinBox(formFrame);
    m_leadTimeSpin->setSuffix(QStringLiteral(" days"));
    m_leadTimeSpin->setRange(1, 90);
    grid->addWidget(m_leadTimeSpin, 3, 1);

    auto* line = new QFrame(formFrame);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("background-color: #30363d; max-height: 1px; border: none;"));
    grid->addWidget(line, 4, 0, 1, 2);

    auto* mlHeader = new QLabel(QStringLiteral("CLASSIFICATION DECISION THRESHOLDS"), formFrame);
    mlHeader->setStyleSheet(QStringLiteral(
        "font-family:'Hanken Grotesk',sans-serif;"
        "font-weight:700;color:#afc6ff;font-size:11px;"
        "letter-spacing:0.06em;background:transparent;border:none;"));
    grid->addWidget(mlHeader, 5, 0, 1, 2);

    grid->addWidget(makeFieldLabel(QStringLiteral("Critical / Reorder Confidence Threshold:")), 6, 0);
    m_threshHighSpin = new QDoubleSpinBox(formFrame);
    m_threshHighSpin->setSuffix(QStringLiteral(" %"));
    m_threshHighSpin->setRange(10.0, 100.0);
    m_threshHighSpin->setDecimals(1);
    grid->addWidget(m_threshHighSpin, 6, 1);

    grid->addWidget(makeFieldLabel(QStringLiteral("Medium / Warning Confidence Threshold:")), 7, 0);
    m_threshMediumSpin = new QDoubleSpinBox(formFrame);
    m_threshMediumSpin->setSuffix(QStringLiteral(" %"));
    m_threshMediumSpin->setRange(10.0, 100.0);
    m_threshMediumSpin->setDecimals(1);
    grid->addWidget(m_threshMediumSpin, 7, 1);

    pLayout->addWidget(formFrame);

    auto* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(12);

    m_resetBtn = new QPushButton(QStringLiteral("Restore Defaults"), pipelineTab);
    connect(m_resetBtn, &QPushButton::clicked, this, &SettingsWidget::onResetClicked);

    m_saveBtn = new QPushButton(QStringLiteral("Apply & Save Config"), pipelineTab);
    m_saveBtn->setObjectName(QStringLiteral("PrimaryBtn"));
    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsWidget::onSaveClicked);

    m_runBtn = new QPushButton(QStringLiteral("▶ Re-run Pipeline"), pipelineTab);
    connect(m_runBtn, &QPushButton::clicked, this, &SettingsWidget::onRunNowClicked);

    actionLayout->addWidget(m_resetBtn);
    actionLayout->addStretch();
    actionLayout->addWidget(m_runBtn);
    actionLayout->addWidget(m_saveBtn);
    pLayout->addLayout(actionLayout);

    tabWidget->addTab(pipelineTab, QStringLiteral("Global Pipeline"));

    // ── TAB 2: Shop Profile ──
    auto* shopTab = new QWidget(tabWidget);
    auto* sLayout = new QVBoxLayout(shopTab);
    sLayout->setContentsMargins(12, 12, 12, 12);
    sLayout->setSpacing(16);

    auto* shopFrame = new QFrame(shopTab);
    shopFrame->setObjectName(QStringLiteral("FormFrame"));
    auto* sGrid = new QGridLayout(shopFrame);
    sGrid->setSpacing(16);

    sGrid->addWidget(makeFieldLabel(QStringLiteral("Shop Name:")), 0, 0);
    m_shopNameEdit = new QLineEdit(shopFrame);
    m_shopNameEdit->setText(m_auth->shopProfile().name);
    sGrid->addWidget(m_shopNameEdit, 0, 1);

    sGrid->addWidget(makeFieldLabel(QStringLiteral("Owner Name:")), 1, 0);
    m_ownerNameEdit = new QLineEdit(shopFrame);
    m_ownerNameEdit->setText(m_auth->shopProfile().ownerName);
    sGrid->addWidget(m_ownerNameEdit, 1, 1);

    sGrid->addWidget(makeFieldLabel(QStringLiteral("Contact Phone:")), 2, 0);
    m_phoneEdit = new QLineEdit(shopFrame);
    m_phoneEdit->setText(m_auth->shopProfile().phone);
    sGrid->addWidget(m_phoneEdit, 2, 1);

    sGrid->addWidget(makeFieldLabel(QStringLiteral("Shop Logo:")), 3, 0);
    auto* logoUploadRow = new QHBoxLayout();
    auto* logoBtn = new QPushButton(QStringLiteral("Browse Logo"), shopFrame);
    connect(logoBtn, &QPushButton::clicked, this, &SettingsWidget::onSelectLogoClicked);
    m_logoPathLabel = new QLabel(m_auth->shopProfile().logoPath.isEmpty() ? QStringLiteral("No logo set") : QFileInfo(m_auth->shopProfile().logoPath).fileName(), shopFrame);
    m_logoPathLabel->setStyleSheet(QStringLiteral("color: #8b949e;"));
    m_logoPath = m_auth->shopProfile().logoPath;
    logoUploadRow->addWidget(logoBtn);
    logoUploadRow->addWidget(m_logoPathLabel);
    logoUploadRow->addStretch();
    sGrid->addLayout(logoUploadRow, 3, 1);

    sLayout->addWidget(shopFrame);

    auto* shopActionLayout = new QHBoxLayout();
    auto* saveShopBtn = new QPushButton(QStringLiteral("Save Shop Profile"), shopTab);
    saveShopBtn->setObjectName(QStringLiteral("PrimaryBtn"));
    connect(saveShopBtn, &QPushButton::clicked, this, &SettingsWidget::onSaveShopClicked);
    shopActionLayout->addStretch();
    shopActionLayout->addWidget(saveShopBtn);
    sLayout->addLayout(shopActionLayout);

    tabWidget->addTab(shopTab, QStringLiteral("Shop Profile"));

    // ── TAB 3: Staff Management ──
    auto* staffTab = new QWidget(tabWidget);
    auto* stLayout = new QHBoxLayout(staffTab);
    stLayout->setContentsMargins(12, 12, 12, 12);
    stLayout->setSpacing(16);

    m_staffListWidget = new QListWidget(staffTab);
    stLayout->addWidget(m_staffListWidget, 2);

    auto* stActions = new QVBoxLayout();
    stActions->setSpacing(12);

    m_addStaffBtn = new QPushButton(QStringLiteral("Add Staff User"), staffTab);
    m_addStaffBtn->setObjectName(QStringLiteral("PrimaryBtn"));
    connect(m_addStaffBtn, &QPushButton::clicked, this, &SettingsWidget::onAddStaffClicked);
    stActions->addWidget(m_addStaffBtn);

    m_deleteStaffBtn = new QPushButton(QStringLiteral("Remove Staff User"), staffTab);
    m_deleteStaffBtn->setObjectName(QStringLiteral("DangerBtn"));
    m_deleteStaffBtn->setEnabled(false);
    connect(m_deleteStaffBtn, &QPushButton::clicked, this, &SettingsWidget::onDeleteStaffClicked);
    stActions->addWidget(m_deleteStaffBtn);

    stActions->addStretch();
    stLayout->addLayout(stActions, 1);

    connect(m_staffListWidget, &QListWidget::currentRowChanged, this, [this](int row) {
        m_deleteStaffBtn->setEnabled(row >= 0);
    });

    tabWidget->addTab(staffTab, QStringLiteral("Staff Pins & Roles"));

    // ── TAB 4: Session & Security ──
    auto* sessionTab = new QWidget(tabWidget);
    auto* sessLayout = new QVBoxLayout(sessionTab);
    sessLayout->setContentsMargins(12, 12, 12, 12);
    sessLayout->setSpacing(16);

    auto* secFormFrame = new QFrame(sessionTab);
    secFormFrame->setObjectName(QStringLiteral("FormFrame"));
    auto* secGrid = new QGridLayout(secFormFrame);
    secGrid->setSpacing(16);

    secGrid->addWidget(makeFieldLabel(QStringLiteral("Auto-lock Timeout:")), 0, 0);
    m_sessionTimeoutCombo = new QComboBox(secFormFrame);
    m_sessionTimeoutCombo->addItem(QStringLiteral("1 minute"), 1);
    m_sessionTimeoutCombo->addItem(QStringLiteral("5 minutes"), 5);
    m_sessionTimeoutCombo->addItem(QStringLiteral("15 minutes"), 15);
    m_sessionTimeoutCombo->addItem(QStringLiteral("30 minutes"), 30);
    m_sessionTimeoutCombo->addItem(QStringLiteral("Never"), 0);
    secGrid->addWidget(m_sessionTimeoutCombo, 0, 1);

    sessLayout->addWidget(secFormFrame);

    m_sessionTimeLabel = new QLabel(QStringLiteral("Session Active: 00:00:00"), sessionTab);
    m_sessionTimeLabel->setStyleSheet(QStringLiteral("font-size: 14px; font-weight: bold; color: #1f6feb;"));
    sessLayout->addWidget(m_sessionTimeLabel);

    m_logoutBtn = new QPushButton(QStringLiteral("Logout"), sessionTab);
    m_logoutBtn->setObjectName(QStringLiteral("DangerBtn"));
    m_logoutBtn->setFixedWidth(200);
    connect(m_logoutBtn, &QPushButton::clicked, this, &SettingsWidget::onLogoutClicked);
    sessLayout->addWidget(m_logoutBtn);

    sessLayout->addStretch();
    tabWidget->addTab(sessionTab, QStringLiteral("Session & Security"));

    m_sessionTimer = new QTimer(this);
    connect(m_sessionTimer, &QTimer::timeout, this, &SettingsWidget::updateSessionTime);
    m_sessionTimer->start(1000);
    updateSessionTime();

    // ── TAB 5: Appearance ──
    auto* appTab = new QWidget(tabWidget);
    auto* appLayout = new QVBoxLayout(appTab);
    appLayout->setContentsMargins(12, 12, 12, 12);
    appLayout->setSpacing(16);

    auto* appFormFrame = new QFrame(appTab);
    appFormFrame->setObjectName(QStringLiteral("FormFrame"));
    auto* appGrid = new QGridLayout(appFormFrame);
    appGrid->setSpacing(16);

    appGrid->addWidget(makeFieldLabel(QStringLiteral("Application Theme:")), 0, 0);
    m_themeCombo = new QComboBox(appFormFrame);
    m_themeCombo->addItem(QStringLiteral("Dark (Pro-Inventory)"), Kirana::ThemeManager::Dark);
    m_themeCombo->addItem(QStringLiteral("Light (Nexus Core)"), Kirana::ThemeManager::Light);
    
    // Set current theme
    int currentThemeIdx = m_themeCombo->findData(Kirana::ThemeManager::instance().theme());
    if (currentThemeIdx >= 0) m_themeCombo->setCurrentIndex(currentThemeIdx);
    
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::onThemeChanged);
    
    appGrid->addWidget(m_themeCombo, 0, 1);
    appLayout->addWidget(appFormFrame);
    appLayout->addStretch();
    tabWidget->addTab(appTab, QStringLiteral("Appearance"));

    mainLayout->addWidget(tabWidget);
}

void SettingsWidget::updateFormFromSettings() {
    const auto& s = m_controller->settings();
    m_orderingCostSpin->setValue(s.orderingCost);
    m_holdingCostSpin->setValue(s.holdingCostRate * 100.0);
    m_leadTimeSpin->setValue(s.leadTimeDays);
    m_threshHighSpin->setValue(s.reorderThreshHigh * 100.0);
    m_threshMediumSpin->setValue(s.reorderThreshMedium * 100.0);

    int timeout = s.sessionTimeoutMinutes;
    int idx = m_sessionTimeoutCombo->findData(timeout);
    if (idx >= 0) m_sessionTimeoutCombo->setCurrentIndex(idx);

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
    s.sessionTimeoutMinutes = m_sessionTimeoutCombo->currentData().toInt();

    emit settingsChanged(s);
    updateFormFromSettings();
}

void SettingsWidget::onResetClicked() {
    AppSettings s;
    emit settingsChanged(s);
    updateFormFromSettings();
}

void SettingsWidget::onRunNowClicked() {
    emit reRunRequested();
}

void SettingsWidget::onSelectLogoClicked() {
    QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Select Shop Logo"), {}, QStringLiteral("Images (*.png *.jpg *.jpeg *.svg)"));
    if (!path.isEmpty()) {
        m_logoPath = path;
        m_logoPathLabel->setText(QFileInfo(path).fileName());
    }
}

void SettingsWidget::onSaveShopClicked() {
    ShopProfile shop = m_auth->shopProfile();
    shop.name = m_shopNameEdit->text().trimmed();
    shop.ownerName = m_ownerNameEdit->text().trimmed();
    shop.phone = m_phoneEdit->text().trimmed();
    shop.logoPath = m_logoPath;

    if (m_auth->updateShopProfile(shop)) {
        QMessageBox::information(this, QStringLiteral("Shop Profile"), QStringLiteral("Profile updated successfully."));
    } else {
        QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("Could not update shop profile."));
    }
}

void SettingsWidget::refreshStaffList() {
    m_staffListWidget->clear();
    QVector<StaffUser> list = m_auth->getStaff();

    for (const auto& u : list) {
        QString itemText = QStringLiteral("%1 (%2)")
            .arg(u.name)
            .arg(u.role);
        auto* item = new QListWidgetItem(itemText, m_staffListWidget);
        item->setData(Qt::UserRole, u.id);
        item->setData(Qt::UserRole + 1, u.role); // Store role to avoid deleting owner
    }
    m_deleteStaffBtn->setEnabled(false);
}

void SettingsWidget::onAddStaffClicked() {
    bool ok;
    QString name = QInputDialog::getText(this, QStringLiteral("Add Staff"), QStringLiteral("Staff Member Name:"), QLineEdit::Normal, {}, &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString pin = QInputDialog::getText(this, QStringLiteral("Set Staff PIN"), QStringLiteral("4-Digit PIN:"), QLineEdit::Password, {}, &ok);
    if (!ok || pin.length() != 4 || !pin.toInt()) {
        QMessageBox::warning(this, QStringLiteral("Invalid PIN"), QStringLiteral("PIN must be exactly 4 digits."));
        return;
    }

    if (m_auth->addStaff(name.trimmed(), pin)) {
        refreshStaffList();
    } else {
        QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("Could not add staff user."));
    }
}

void SettingsWidget::onDeleteStaffClicked() {
    auto* item = m_staffListWidget->currentItem();
    if (!item) return;

    int userId = item->data(Qt::UserRole).toInt();
    QString role = item->data(Qt::UserRole + 1).toString();

    if (role == QLatin1String("Owner")) {
        QMessageBox::warning(this, QStringLiteral("Action Denied"), QStringLiteral("The primary owner user cannot be removed."));
        return;
    }

    auto reply = QMessageBox::question(this, QStringLiteral("Remove User"),
        QStringLiteral("Are you sure you want to remove this staff user?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        if (m_auth->removeStaff(userId)) {
            refreshStaffList();
        } else {
            QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("Could not remove staff user."));
        }
    }
}

void SettingsWidget::updateSessionTime() {
    if (m_auth->isLoggedIn() && m_auth->loginTime().isValid()) {
        qint64 secs = m_auth->loginTime().secsTo(QDateTime::currentDateTime());
        if (secs < 0) secs = 0;
        qint64 h = secs / 3600;
        qint64 m = (secs % 3600) / 60;
        qint64 s = secs % 60;
        m_sessionTimeLabel->setText(QStringLiteral("Session Active: %1:%2:%3")
            .arg(h, 2, 10, QLatin1Char('0'))
            .arg(m, 2, 10, QLatin1Char('0'))
            .arg(s, 2, 10, QLatin1Char('0')));
    } else {
        m_sessionTimeLabel->setText(QStringLiteral("Not logged in"));
    }
}

void SettingsWidget::onLogoutClicked() {
    auto reply = QMessageBox::question(this, QStringLiteral("Logout"),
        QStringLiteral("Are you sure you want to log out?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_auth->logout();
    }
}

void SettingsWidget::onThemeChanged(int index) {
    ThemeManager::Theme t = static_cast<ThemeManager::Theme>(m_themeCombo->itemData(index).toInt());
    ThemeManager::instance().setTheme(t);
}

} // namespace Kirana
