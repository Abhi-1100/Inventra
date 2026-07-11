#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QPropertyAnimation>
#include "core/AuthData.h"

namespace Kirana {

class AuthController;

// ─────────────────────────────────────────────
// PinDotIndicator
// 4 animated dots that fill as digits are entered
// ─────────────────────────────────────────────

class PinDotIndicator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int filled READ filled WRITE setFilled)
public:
    explicit PinDotIndicator(QWidget* parent = nullptr);
    int  filled() const    { return m_filled; }
    void setFilled(int n)  { m_filled = qBound(0,n,4); update(); }
    void reset()           { setFilled(0); }
protected:
    void paintEvent(QPaintEvent*) override;
private:
    int m_filled = 0;
};

// ─────────────────────────────────────────────
// AuthWidget
//
// Shows either the Registration page or the Login
// PIN pad depending on whether the shop is registered.
// ─────────────────────────────────────────────

class AuthWidget : public QWidget {
    Q_OBJECT

public:
    enum class Mode { Registration, Login, Unlock };

    explicit AuthWidget(AuthController* auth, QWidget* parent = nullptr);

    void setMode(Mode m);
    Mode mode() const { return m_mode; }

    // Called by MainWindow on loginFailed signal → shake animation
    void triggerShake();

public slots:
    void onLoginFailed();

private slots:
    void onPinDigitPressed(int digit);
    void onPinBackspace();
    void onRegisterClicked();
    void onForgotPin();

private:
    void buildRegistrationPage();
    void buildLoginPage();
    void submitPin();
    void resetPin();
    void updateShopGreeting();

    AuthController* m_auth;
    Mode            m_mode = Mode::Login;

    QStackedWidget* m_stack = nullptr;

    // ── Registration page ─────────────────────
    QWidget*   m_regPage       = nullptr;
    QLineEdit* m_shopNameEdit  = nullptr;
    QLineEdit* m_ownerNameEdit = nullptr;
    QLineEdit* m_phoneEdit     = nullptr;
    QLineEdit* m_pinEdit       = nullptr;
    QLineEdit* m_pinConfEdit   = nullptr;
    QLabel*    m_logoPathLabel = nullptr;
    QString    m_logoPath;

    // ── Login page ────────────────────────────
    QWidget*         m_loginPage    = nullptr;
    QLabel*          m_shopGreeting = nullptr;
    PinDotIndicator* m_pinDots      = nullptr;
    QLabel*          m_errorLabel   = nullptr;
    QString          m_pinBuffer;
    QWidget*         m_pinCard      = nullptr;

    // Shake animation
    QPropertyAnimation* m_shakeAnim = nullptr;
};

} // namespace Kirana
