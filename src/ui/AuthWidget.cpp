#include "core/ThemeManager.h"
#include "ui/AuthWidget.h"
#include "core/AuthController.h"
#include "core/ProductData.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QPainter>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>

namespace Kirana {

// ══════════════════════════════════════════════
// PinDotIndicator
// ══════════════════════════════════════════════

PinDotIndicator::PinDotIndicator(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(120, 24);
}

void PinDotIndicator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int dotR    = 8;
    const int spacing = 28;
    const int totalW  = 4 * spacing;
    int startX = (width() - totalW) / 2 + spacing / 2 - dotR;
    const int y = height() / 2 - dotR;

    for (int i = 0; i < 4; ++i) {
        bool filled = (i < m_filled);
        QColor fill   = filled ? QColor("#afc6ff") : QColor("#262a30");
        QColor border = filled ? QColor("#afc6ff") : QColor("#424754");

        // Glow effect for filled dots
        if (filled) {
            QColor glow(0xaf, 0xc6, 0xff, 60);
            p.setPen(Qt::NoPen);
            p.setBrush(glow);
            p.drawEllipse(startX + i * spacing - 3, y - 3, (dotR + 3) * 2, (dotR + 3) * 2);
        }

        p.setPen(QPen(border, 1.5));
        p.setBrush(fill);
        p.drawEllipse(startX + i * spacing, y, dotR * 2, dotR * 2);
    }
}

// ══════════════════════════════════════════════
// AuthWidget
// ══════════════════════════════════════════════

AuthWidget::AuthWidget(AuthController* auth, QWidget* parent)
    : QWidget(parent)
    , m_auth(auth)
{
    setObjectName(QStringLiteral("AuthPage"));
    setFocusPolicy(Qt::StrongFocus);

    m_stack = new QStackedWidget(this);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(m_stack);

    buildRegistrationPage();
    buildLoginPage();

    m_stack->addWidget(m_regPage);    // index 0
    m_stack->addWidget(m_loginPage);  // index 1
}

// ─────────────────────────────────────────────
// Registration page
// ─────────────────────────────────────────────

void AuthWidget::buildRegistrationPage() {
    m_regPage = new QWidget;
    m_regPage->setObjectName(QStringLiteral("AuthPage"));

    auto* outer = new QVBoxLayout(m_regPage);
    outer->setAlignment(Qt::AlignCenter);
    outer->setContentsMargins(40, 40, 40, 40);

    // Card
    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("AuthCard"));
    card->setFixedWidth(480);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(40, 40, 40, 40);
    cardLayout->setSpacing(20);

    // ── Logo + wordmark ──
    auto* logoRow = new QHBoxLayout;
    logoRow->setAlignment(Qt::AlignCenter);

    auto* logoIcon = new QLabel;
    logoIcon->setPixmap(
        QPixmap(QStringLiteral(":/icons/inventra_logo.svg")).scaled(32, 32,
            Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto* wordmark = new QLabel(QStringLiteral("Inventra"));
    wordmark->setStyleSheet(
        QStringLiteral("font-family: 'Hanken Grotesk', 'Segoe UI', sans-serif;"
                       "font-size: 24px; font-weight: 700;"
                       "color: #afc6ff; background: transparent; border: none;"));

    logoRow->addWidget(logoIcon);
    logoRow->addSpacing(8);
    logoRow->addWidget(wordmark);
    cardLayout->addLayout(logoRow);

    auto* subtitle = new QLabel(QStringLiteral("Set up your shop to get started"));
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet(QStringLiteral(
        "color: #8c90a0; font-size: 13px; font-family:'Hanken Grotesk',sans-serif;"
        "background: transparent; border: none;"));
    cardLayout->addWidget(subtitle);

    // Separator
    auto* sep = new QFrame; sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(QStringLiteral("background:#232a33; max-height:1px; border:none;"));
    cardLayout->addWidget(sep);

    // ── Form fields ──
    auto makeLabel = [&](const QString& text) -> QLabel* {
        auto* l = new QLabel(text);
        l->setStyleSheet(QStringLiteral(
            "color:#8c90a0; font-size:11px; font-weight:600;"
            "letter-spacing:0.05em;background:transparent;border:none;"));
        return l;
    };

    cardLayout->addWidget(makeLabel(QStringLiteral("SHOP NAME")));
    m_shopNameEdit = new QLineEdit;
    m_shopNameEdit->setPlaceholderText(QStringLiteral("e.g. Sharma General Store"));
    m_shopNameEdit->setMinimumHeight(40);
    cardLayout->addWidget(m_shopNameEdit);

    cardLayout->addWidget(makeLabel(QStringLiteral("OWNER NAME")));
    m_ownerNameEdit = new QLineEdit;
    m_ownerNameEdit->setPlaceholderText(QStringLiteral("Full name"));
    m_ownerNameEdit->setMinimumHeight(40);
    cardLayout->addWidget(m_ownerNameEdit);

    cardLayout->addWidget(makeLabel(QStringLiteral("PHONE NUMBER")));
    m_phoneEdit = new QLineEdit;
    m_phoneEdit->setPlaceholderText(QStringLiteral("+91 XXXXX XXXXX"));
    m_phoneEdit->setMinimumHeight(40);
    cardLayout->addWidget(m_phoneEdit);

    // PIN row
    auto* pinRow = new QHBoxLayout;
    auto* pinCol1 = new QVBoxLayout;
    auto* pinCol2 = new QVBoxLayout;

    pinCol1->addWidget(makeLabel(QStringLiteral("4-DIGIT PIN")));
    m_pinEdit = new QLineEdit;
    m_pinEdit->setEchoMode(QLineEdit::Password);
    m_pinEdit->setMaxLength(4);
    m_pinEdit->setPlaceholderText(QStringLiteral("••••"));
    m_pinEdit->setMinimumHeight(40);
    m_pinEdit->setStyleSheet(QStringLiteral(
        "font-family: 'JetBrains Mono','Consolas',monospace;"
        "letter-spacing:4px; font-size:16px;"));
    pinCol1->addWidget(m_pinEdit);

    pinCol2->addWidget(makeLabel(QStringLiteral("CONFIRM PIN")));
    m_pinConfEdit = new QLineEdit;
    m_pinConfEdit->setEchoMode(QLineEdit::Password);
    m_pinConfEdit->setMaxLength(4);
    m_pinConfEdit->setPlaceholderText(QStringLiteral("••••"));
    m_pinConfEdit->setMinimumHeight(40);
    m_pinConfEdit->setStyleSheet(QStringLiteral(
        "font-family: 'JetBrains Mono','Consolas',monospace;"
        "letter-spacing:4px; font-size:16px;"));
    pinCol2->addWidget(m_pinConfEdit);

    pinRow->addLayout(pinCol1);
    pinRow->addSpacing(16);
    pinRow->addLayout(pinCol2);
    cardLayout->addLayout(pinRow);

    // Logo upload
    auto* logoRow2 = new QHBoxLayout;
    auto* uploadBtn = new QPushButton(QStringLiteral("📁  Upload Shop Logo (optional)"));
    uploadBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:#0b0f14; border:1px dashed #30363d;"
        "  color:#6e7681; padding:8px; border-radius:5px; }"
        "QPushButton:hover { border-color:#6e7681; color:#8b949e; }"));
    m_logoPathLabel = new QLabel(QStringLiteral("No file selected"));
    m_logoPathLabel->setStyleSheet(QStringLiteral(
        "color:#6e7681; font-size:11px; background:transparent;"));
    logoRow2->addWidget(uploadBtn);
    logoRow2->addWidget(m_logoPathLabel, 1);
    cardLayout->addLayout(logoRow2);

    connect(uploadBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("Select Logo"), {},
            QStringLiteral("Images (*.png *.jpg *.jpeg *.svg)"));
        if (!path.isEmpty()) {
            m_logoPath = path;
            m_logoPathLabel->setText(QFileInfo(path).fileName());
        }
    });

    // Create shop button
    auto* createBtn = new QPushButton(QStringLiteral("Create Shop →"));
    createBtn->setObjectName(QStringLiteral("PrimaryBtn"));
    createBtn->setMinimumHeight(44);
    createBtn->setStyleSheet(QStringLiteral(
        "QPushButton#PrimaryBtn {"
        "  background:#1f6feb; border:none; color:#fff;"
        "  font-size:14px; font-weight:600; border-radius:7px;"
        "}"
        "QPushButton#PrimaryBtn:hover { background:#388bfd; }"
        "QPushButton#PrimaryBtn:pressed { background:#1158c7; }"));
    cardLayout->addSpacing(8);
    cardLayout->addWidget(createBtn);

    connect(createBtn, &QPushButton::clicked, this, &AuthWidget::onRegisterClicked);
    // Also allow Enter key
    connect(m_pinConfEdit, &QLineEdit::returnPressed, this, &AuthWidget::onRegisterClicked);

    outer->addWidget(card, 0, Qt::AlignCenter);
}

// ─────────────────────────────────────────────
// Login page
// ─────────────────────────────────────────────

void AuthWidget::buildLoginPage() {
    m_loginPage = new QWidget;
    m_loginPage->setObjectName(QStringLiteral("AuthPage"));

    // Full-screen centered layout — no card border, matches 6_Secure_Login.html
    auto* outer = new QVBoxLayout(m_loginPage);
    outer->setAlignment(Qt::AlignCenter);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Use a transparent frame (no card bg — the page itself is the bg)
    m_pinCard = new QFrame;
    m_pinCard->setObjectName(QStringLiteral("LoginFrame"));
    m_pinCard->setStyleSheet("QFrame#LoginFrame { background:transparent; border:none; }");

    auto* cardLayout = new QVBoxLayout(m_pinCard);
    cardLayout->setContentsMargins(40, 0, 40, 0);
    cardLayout->setSpacing(0);
    cardLayout->setAlignment(Qt::AlignCenter);

    // ── Brand header ─────────────────────────
    auto* brandWidget = new QWidget;
    brandWidget->setStyleSheet("background:transparent;");
    auto* brandLayout = new QVBoxLayout(brandWidget);
    brandLayout->setAlignment(Qt::AlignCenter);
    brandLayout->setSpacing(6);
    brandLayout->setContentsMargins(0, 0, 0, 40);

    // "Inventra" title in primary color
    auto* wordmark = new QLabel(QStringLiteral("Inventra"));
    wordmark->setAlignment(Qt::AlignCenter);
    wordmark->setStyleSheet(QStringLiteral(
        "font-family:'Hanken Grotesk','Segoe UI',sans-serif;"
        "font-size:36px;font-weight:700;"
        "color:#afc6ff;"
        "background:transparent;border:none;"));

    m_shopGreeting = new QLabel(QStringLiteral("Welcome back"));
    m_shopGreeting->setAlignment(Qt::AlignCenter);
    m_shopGreeting->setStyleSheet(QStringLiteral(
        "font-family:'Hanken Grotesk',sans-serif;"
        "font-size:15px;color:#8c90a0;"
        "background:transparent;border:none;"));

    brandLayout->addWidget(wordmark);
    brandLayout->addWidget(m_shopGreeting);
    cardLayout->addWidget(brandWidget);

    // ── PIN dot indicators ──────────────────
    m_pinDots = new PinDotIndicator;
    m_pinDots->setFixedSize(160, 32);
    cardLayout->addWidget(m_pinDots, 0, Qt::AlignCenter);
    cardLayout->addSpacing(32);

    // Error label (hidden)
    m_errorLabel = new QLabel(QStringLiteral("Incorrect PIN. Please try again."));
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet(QStringLiteral(
        "color:#ffb4ab;font-size:13px;font-family:'Hanken Grotesk',sans-serif;"
        "background:transparent;border:none;"));
    m_errorLabel->setVisible(false);
    cardLayout->addWidget(m_errorLabel);
    cardLayout->addSpacing(8);

    // ── PIN Keypad grid (3x4) ────────────────
    auto* pinGrid = new QGridLayout;
    pinGrid->setSpacing(20);  // gap-6 = 24px
    pinGrid->setAlignment(Qt::AlignCenter);

    auto makeDigitBtn = [this](const QString& label, int digit) {
        auto* btn = new QPushButton(label);
        btn->setObjectName(QStringLiteral("PinBtn"));
        btn->setFixedSize(76, 76);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton#PinBtn {"
            "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #262a30,stop:1 #1c2025);"
            "  border: 1px solid #424754;"
            "  color: #e0e2ea;"
            "  font-family:'JetBrains Mono',monospace;"
            "  font-size:20px;font-weight:500;"
            "  border-radius:38px;"
            "}"
            "QPushButton#PinBtn:hover {"
            "  background:#31353b;"
            "  border-color:#afc6ff;"
            "}"
            "QPushButton#PinBtn:pressed {"
            "  background:rgba(31,111,235,0.3);"
            "  border-color:#afc6ff;"
            "  color:#ffffff;"
            "}");
        connect(btn, &QPushButton::clicked, this, [this, digit]() {
            onPinDigitPressed(digit);
        });
        return btn;
    };

    for (int d = 1; d <= 9; ++d) {
        int row = (d - 1) / 3;
        int col = (d - 1) % 3;
        pinGrid->addWidget(makeDigitBtn(QString::number(d), d), row, col);
    }
    // Row 3: [empty] [0] [backspace]
    pinGrid->addItem(new QSpacerItem(76, 76), 3, 0);
    pinGrid->addWidget(makeDigitBtn(QStringLiteral("0"), 0), 3, 1);

    auto* bsBtn = new QPushButton(QStringLiteral("⌫"));
    bsBtn->setObjectName(QStringLiteral("PinBackspaceBtn"));
    bsBtn->setFixedSize(76, 76);
    bsBtn->setCursor(Qt::PointingHandCursor);
    bsBtn->setStyleSheet(
        "QPushButton#PinBackspaceBtn {"
        "  background:transparent;border:none;"
        "  color:#8c90a0;font-size:22px;"
        "  border-radius:38px;"
        "}"
        "QPushButton#PinBackspaceBtn:hover {"
        "  background:rgba(255,180,171,0.08);"
        "  color:#ffb4ab;"
        "}");
    connect(bsBtn, &QPushButton::clicked, this, &AuthWidget::onPinBackspace);
    pinGrid->addWidget(bsBtn, 3, 2);

    cardLayout->addLayout(pinGrid);
    cardLayout->addSpacing(32);

    // "Register your shop" link
    auto* forgotBtn = new QPushButton(QStringLiteral("Register your shop"));
    forgotBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background:transparent;border:none;"
        "  color:#afc6ff;"
        "  font-family:'Hanken Grotesk',sans-serif;"
        "  font-size:13px;font-weight:600;"
        "}"
        "QPushButton:hover { color:#d9e2ff; }"));
    forgotBtn->setCursor(Qt::PointingHandCursor);
    connect(forgotBtn, &QPushButton::clicked, this, &AuthWidget::onForgotPin);
    cardLayout->addWidget(forgotBtn, 0, Qt::AlignCenter);

    outer->addWidget(m_pinCard, 0, Qt::AlignCenter);

    // Shake animation
    m_shakeAnim = new QPropertyAnimation(m_pinCard, "pos", this);
    m_shakeAnim->setDuration(400);
}

// ─────────────────────────────────────────────
// setMode
// ─────────────────────────────────────────────

void AuthWidget::setMode(Mode m) {
    m_mode = m;
    if (m == Mode::Registration) {
        m_stack->setCurrentWidget(m_regPage);
    } else {
        m_stack->setCurrentWidget(m_loginPage);
        if (m == Mode::Unlock) {
            m_shopGreeting->setText(QStringLiteral("Session Locked"));
        } else {
            updateShopGreeting();
        }
    }
}

void AuthWidget::updateShopGreeting() {
    const ShopProfile& shop = m_auth->shopProfile();
    if (!shop.name.isEmpty())
        m_shopGreeting->setText(
            QStringLiteral("Welcome back — %1").arg(shop.name));
}

// ─────────────────────────────────────────────
// PIN digit pressed
// ─────────────────────────────────────────────

void AuthWidget::onPinDigitPressed(int digit) {
    if (m_pinBuffer.length() >= 4) return;
    m_pinBuffer += QString::number(digit);
    m_pinDots->setFilled(m_pinBuffer.length());
    m_errorLabel->setVisible(false);

    if (m_pinBuffer.length() == 4)
        QTimer::singleShot(80, this, &AuthWidget::submitPin);
}

void AuthWidget::onPinBackspace() {
    if (m_pinBuffer.isEmpty()) return;
    m_pinBuffer.chop(1);
    m_pinDots->setFilled(m_pinBuffer.length());
    m_errorLabel->setVisible(false);
}

void AuthWidget::submitPin() {
    if (m_mode == Mode::Unlock) {
        m_auth->unlockSession(m_pinBuffer);
    } else {
        m_auth->authenticate(m_pinBuffer);
    }
    // Result comes via loginSucceeded / loginFailed / sessionUnlocked signals
    resetPin();
}

void AuthWidget::resetPin() {
    m_pinBuffer.clear();
    m_pinDots->reset();
}

void AuthWidget::keyPressEvent(QKeyEvent* event) {
    if (m_mode == Mode::Registration) {
        QWidget::keyPressEvent(event);
        return;
    }

    if (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9) {
        onPinDigitPressed(event->key() - Qt::Key_0);
    } else if (event->key() == Qt::Key_Backspace) {
        onPinBackspace();
    } else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        if (m_pinBuffer.length() == 4) {
            submitPin();
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}

// ─────────────────────────────────────────────
// loginFailed → shake animation + red flash
// ─────────────────────────────────────────────

void AuthWidget::onLoginFailed() {
    m_errorLabel->setVisible(true);
    triggerShake();
}

void AuthWidget::triggerShake() {
    if (!m_pinCard) return;

    const QPoint origin = m_pinCard->pos();
    m_shakeAnim->stop();

    QSequentialAnimationGroup* group = new QSequentialAnimationGroup(this);

    auto addStep = [&](int dx, int ms) {
        auto* a = new QPropertyAnimation(m_pinCard, "pos");
        a->setDuration(ms);
        a->setStartValue(origin);
        a->setEndValue(origin + QPoint(dx, 0));
        group->addAnimation(a);
    };

    addStep(-10, 50);
    addStep( 10, 50);
    addStep(-8,  50);
    addStep( 8,  50);
    addStep(-4,  50);
    addStep( 0,  50);

    group->start(QAbstractAnimation::DeleteWhenStopped);

    // Briefly flash the dots red
    m_pinDots->setFilled(0);
}

// ─────────────────────────────────────────────
// Registration submit
// ─────────────────────────────────────────────

void AuthWidget::onRegisterClicked() {
    const QString shopName  = m_shopNameEdit->text().trimmed();
    const QString ownerName = m_ownerNameEdit->text().trimmed();
    const QString phone     = m_phoneEdit->text().trimmed();
    const QString pin       = m_pinEdit->text().trimmed();
    const QString pinConf   = m_pinConfEdit->text().trimmed();

    if (shopName.isEmpty() || ownerName.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Inventra"),
            QStringLiteral("Shop name and owner name are required."));
        return;
    }
    if (pin.length() != 4 || !pin.toInt()) {
        // allow 0000 too
        if (pin.length() != 4) {
            QMessageBox::warning(this, QStringLiteral("Inventra"),
                QStringLiteral("PIN must be exactly 4 digits."));
            return;
        }
    }
    if (pin != pinConf) {
        QMessageBox::warning(this, QStringLiteral("Inventra"),
            QStringLiteral("PINs do not match. Please re-enter."));
        m_pinConfEdit->clear();
        m_pinConfEdit->setFocus();
        return;
    }

    ShopProfile shop;
    shop.name      = shopName;
    shop.ownerName = ownerName;
    shop.phone     = phone;
    shop.logoPath  = m_logoPath;

    if (!m_auth->registerShop(shop, ownerName, pin)) {
        QMessageBox::critical(this, QStringLiteral("Inventra"),
            QStringLiteral("Registration failed. Please try again."));
    }
    // On success: MainWindow handles loginSucceeded signal
}

// ─────────────────────────────────────────────
// Forgot PIN
// ─────────────────────────────────────────────

void AuthWidget::onForgotPin() {
    QMessageBox dlg(this);
    dlg.setWindowTitle(QStringLiteral("Forgot PIN"));
    dlg.setIcon(QMessageBox::Information);
    dlg.setText(QStringLiteral(
        "A verification code has been sent to the registered phone number.\n\n"
        "(This is a stub — phone verification not yet integrated.)"));
    dlg.setStandardButtons(QMessageBox::Ok);
    dlg.exec();
}

} // namespace Kirana
