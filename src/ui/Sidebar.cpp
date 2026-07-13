#include "core/ThemeManager.h"
#include "ui/Sidebar.h"
#include "core/ProductData.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QLabel>
#include <QFontMetrics>
#include <QLinearGradient>

namespace Kirana {

// ══════════════════════════════════════════════
// SidebarButton
// ══════════════════════════════════════════════

SidebarButton::SidebarButton(const QString& iconPath,
                               const QString& label,
                               const QString& tooltip,
                               QWidget* parent)
    : QWidget(parent)
    , m_iconPath(iconPath)
    , m_label(label)
{
    setFixedSize(260, 44);
    setToolTip(tooltip);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);

    m_icon = QPixmap(iconPath);
}

void SidebarButton::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    const QRect r = rect();

    // ── Background ─────────────────────────────
    if (m_active) {
        // Active: primary tint background
        QColor bg(0xaf, 0xc6, 0xff, 26); // rgba(175,198,255, 0.10)
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(r.adjusted(6, 2, -6, -2), 6, 6);

        // Left accent bar — 4px, full height of row, primary color
        p.setBrush(QColor("#afc6ff"));
        p.setPen(Qt::NoPen);
        p.drawRect(0, 0, 4, r.height());

    } else if (m_hovered) {
        // Hover: subtle surface-container-high tint
        QColor bg(0x26, 0x2a, 0x30, 180);
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(r.adjusted(6, 2, -6, -2), 6, 6);
    }

    // ── Icon ───────────────────────────────────
    const int iconSz = 20;
    const int iconX  = 20;
    const int iconY  = (r.height() - iconSz) / 2;

    QColor iconColor = m_active  ? QColor("#afc6ff")
                     : m_hovered ? QColor("#e0e2ea")
                                 : QColor("#8c90a0");

    if (!m_icon.isNull()) {
        QPixmap scaled = m_icon.scaled(iconSz, iconSz,
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
        p.setOpacity(1.0);
        p.drawPixmap(iconX, iconY, scaled);

        // Tint overlay
        p.save();
        p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        p.fillRect(QRect(iconX, iconY, iconSz, iconSz), iconColor);
        p.restore();
    } else {
        // Fallback: draw a small colored dot as placeholder
        p.setPen(Qt::NoPen);
        p.setBrush(iconColor);
        p.drawEllipse(iconX + 4, iconY + 4, iconSz - 8, iconSz - 8);
    }

    // ── Label ──────────────────────────────────
    const int textX = iconX + iconSz + 12;
    QColor textColor = m_active  ? QColor("#afc6ff")
                      : m_hovered ? QColor("#e0e2ea")
                                  : QColor("#c2c6d6");

    p.setPen(textColor);
    QFont f;
    f.setFamily(QStringLiteral("Hanken Grotesk"));
    f.setPixelSize(13);
    f.setWeight(m_active ? QFont::DemiBold : QFont::Normal);
    p.setFont(f);
    p.drawText(QRect(textX, 0, r.width() - textX - 8, r.height()),
               Qt::AlignVCenter | Qt::AlignLeft, m_label);
}

void SidebarButton::mousePressEvent(QMouseEvent* ev) {
    if (ev->button() == Qt::LeftButton) emit clicked();
    QWidget::mousePressEvent(ev);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void SidebarButton::enterEvent(QEnterEvent* ev) {
#else
void SidebarButton::enterEvent(QEvent* ev) {
#endif
    m_hovered = true; update(); QWidget::enterEvent(ev);
}

void SidebarButton::leaveEvent(QEvent* ev) {
    m_hovered = false; update(); QWidget::leaveEvent(ev);
}

// ══════════════════════════════════════════════
// Sidebar
// ══════════════════════════════════════════════

Sidebar::Sidebar(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(260);
    setObjectName(QStringLiteral("Sidebar"));
    buildLayout();
}

void Sidebar::paintEvent(QPaintEvent* event) {
    // Custom background matching surface-container-low (#181c21)
    QPainter p(this);
    p.fillRect(rect(), QColor("#181c21"));
    // Right border
    p.setPen(QPen(QColor("#424754"), 1));
    p.drawLine(width() - 1, 0, width() - 1, height());
    QWidget::paintEvent(event);
}

void Sidebar::buildLayout() {
    auto* vlay = new QVBoxLayout(this);
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(0);

    // ── Logo / Brand area ───────────────────────
    auto* logoArea = new QWidget(this);
    logoArea->setFixedHeight(72);
    logoArea->setStyleSheet(QStringLiteral(
        "background:transparent; border-bottom: 1px solid #424754;"));

    auto* logoRow = new QVBoxLayout(logoArea);
    logoRow->setContentsMargins(24, 14, 16, 14);
    logoRow->setSpacing(2);

    // "Inventra" wordmark — gradient text effect using HTML
    auto* wordmark = new QLabel(logoArea);
    wordmark->setText(QStringLiteral("Inventra"));
    wordmark->setStyleSheet(QStringLiteral(
        "font-family:'Hanken Grotesk','Segoe UI',sans-serif;"
        "font-size:20px; font-weight:700;"
        "color:#afc6ff;"             // primary accent as approximation
        "background:transparent;"
        "border:none;"));

    auto* subtitle = new QLabel(QStringLiteral("Inventory Management"), logoArea);
    subtitle->setStyleSheet(QStringLiteral(
        "font-family:'Hanken Grotesk','Segoe UI',sans-serif;"
        "font-size:11px; font-weight:600;"
        "color:#8c90a0;"
        "letter-spacing:0.05em;"
        "background:transparent;"
        "border:none;"));

    logoRow->addWidget(wordmark);
    logoRow->addWidget(subtitle);

    vlay->addWidget(logoArea);
    vlay->addSpacing(8);

    // ── Nav items (all except Settings) ────────
    struct NavItem {
        const char* icon;
        const char* label;
        const char* tip;
        Page page;
        bool bottom; // true = pinned at bottom
    };

    static const NavItem items[] = {
        { ":/icons/dashboard.svg",   "Dashboard",    "Dashboard",       Page::Dashboard,  false },
        { ":/icons/daily_entry.svg", "Daily Entry",  "Daily Entry",     Page::DailyEntry, false },
        { ":/icons/stock.svg",       "Stock In/Out", "Stock In / Out",  Page::Stock,      false },
        { ":/icons/products.svg",    "Products",     "Products",        Page::Products,   false },
        { ":/icons/analytics.svg",   "Analytics",    "Analytics",       Page::Analytics,  false },
        { ":/icons/import.svg",      "Import",       "Import CSV",      Page::Import,     false },
    };

    auto* navArea = new QWidget(this);
    navArea->setStyleSheet("background:transparent;");
    auto* navLayout = new QVBoxLayout(navArea);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(2);

    for (const auto& item : items) {
        auto* btn = new SidebarButton(
            QString::fromLatin1(item.icon),
            QString::fromLatin1(item.label),
            QString::fromLatin1(item.tip),
            this);
        connectButton(btn, item.page);
        m_buttons.append(btn);
        navLayout->addWidget(btn);
    }
    navLayout->addStretch();

    vlay->addWidget(navArea, 1);

    // ── Settings — pinned at bottom ─────────────
    auto* separator = new QWidget(this);
    separator->setFixedHeight(1);
    separator->setStyleSheet(QStringLiteral(
        "background-color: #424754; border: none;"));
    vlay->addWidget(separator);

    auto* settingsBtn = new SidebarButton(
        QStringLiteral(":/icons/settings.svg"),
        QStringLiteral("Settings"),
        QStringLiteral("Settings"),
        this);
    connectButton(settingsBtn, Page::Settings);
    m_buttons.append(settingsBtn);
    vlay->addWidget(settingsBtn);
    vlay->addSpacing(8);

    // Set dashboard active by default
    if (!m_buttons.isEmpty())
        m_buttons[0]->setActive(true);
}

void Sidebar::connectButton(SidebarButton* btn, Page page) {
    connect(btn, &SidebarButton::clicked, this, [this, btn, page]() {
        Q_UNUSED(btn)
        setActivePage(page);
        emit pageSelected(page);
    });
}

void Sidebar::setActivePage(Page page) {
    m_activePage = page;
    for (int i = 0; i < m_buttons.size(); ++i)
        m_buttons[i]->setActive(static_cast<Page>(i) == page);
}

void Sidebar::setRole(const QString& role) {
    // Staff cannot see Analytics (idx 4), Import (5), Settings (idx 6 = last)
    const bool isOwner = (role == QLatin1String("Owner"));
    if (m_buttons.size() >= 7) {
        m_buttons[4]->setVisible(isOwner);   // Analytics
        m_buttons[5]->setVisible(isOwner);   // Import
        m_buttons[6]->setVisible(isOwner);   // Settings
    }
}

} // namespace Kirana
