#include "core/ThemeManager.h"
#include "ui/Sidebar.h"
#include "core/ProductData.h"
#include "widgets/QtAwesome.h"
#include <QPropertyAnimation>

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

static QColor interpolateColor(const QColor& c1, const QColor& c2, qreal progress) {
    int r = c1.red()   + progress * (c2.red()   - c1.red());
    int g = c1.green() + progress * (c2.green() - c1.green());
    int b = c1.blue()  + progress * (c2.blue()  - c1.blue());
    int a = c1.alpha() + progress * (c2.alpha() - c1.alpha());
    return QColor(r, g, b, a);
}

SidebarButton::SidebarButton(char32_t iconChar,
                               const QString& label,
                               const QString& tooltip,
                               QWidget* parent)
    : QWidget(parent)
    , m_iconChar(iconChar)
    , m_label(label)
{
    setFixedSize(260, 44);
    setToolTip(tooltip);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);
}

void SidebarButton::setActive(bool v) {
    if (m_active != v) {
        m_active = v;
        animateActive(v);
    }
}

void SidebarButton::animateHover(bool hover) {
    auto* anim = new QPropertyAnimation(this, "hoverProgress", this);
    anim->setDuration(150);
    anim->setStartValue(m_hoverProgress);
    anim->setEndValue(hover ? 1.0 : 0.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void SidebarButton::animateActive(bool active) {
    auto* anim = new QPropertyAnimation(this, "activeProgress", this);
    anim->setDuration(180);
    anim->setStartValue(m_activeProgress);
    anim->setEndValue(active ? 1.0 : 0.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void SidebarButton::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect r = rect();

    // ── Backgrounds ─────────────────────────────
    if (m_hoverProgress > 0.01) {
        QColor bg = QColor("#161616"); // Obsidian bg_hover
        bg.setAlpha(static_cast<int>(255 * m_hoverProgress));
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(r.adjusted(6, 2, -6, -2), 4, 4);
    }

    if (m_activeProgress > 0.01) {
        const auto& tokens = ThemeManager::instance().tokens();
        QColor bg = tokens.Accent;
        bg.setAlpha(static_cast<int>(26 * m_activeProgress));
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(r.adjusted(6, 2, -6, -2), 4, 4);

        // Left accent bar
        QColor accent = tokens.Accent;
        accent.setAlphaF(m_activeProgress);
        p.setBrush(accent);
        p.setPen(Qt::NoPen);
        p.drawRect(0, 0, 4, r.height());
    }

    // ── Icon & Text colors ──────────────────────
    const auto& tokens = ThemeManager::instance().tokens();
    QColor baseColor = interpolateColor(tokens.TextSecondary, tokens.TextPrimary, m_hoverProgress);
    QColor iconColor = interpolateColor(baseColor, tokens.Accent, m_activeProgress);
    QColor textColor = interpolateColor(baseColor, tokens.Accent, m_activeProgress);

    // ── Icon ───────────────────────────────────
    const int iconSz = 20;
    const int iconX  = 20;
    const int iconY  = (r.height() - iconSz) / 2;

    QPixmap pix = QtAwesome::instance().pixmap(m_iconChar, iconSz, iconColor);
    if (!pix.isNull()) {
        p.drawPixmap(iconX, iconY, pix);
    }

    // ── Label ──────────────────────────────────
    const int textX = iconX + iconSz + 12;
    p.setPen(textColor);
    QFont f;
    f.setFamily(QStringLiteral("SF Mono"));
    f.setPixelSize(13);
    f.setWeight(m_active ? QFont::Bold : QFont::Normal);
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
    m_hovered = true;
    animateHover(true);
    QWidget::enterEvent(ev);
}

void SidebarButton::leaveEvent(QEvent* ev) {
    m_hovered = false;
    animateHover(false);
    QWidget::leaveEvent(ev);
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

    // "Inventra" wordmark
    auto* wordmark = new QLabel(logoArea);
    wordmark->setText(QStringLiteral("Inventra"));
    wordmark->setStyleSheet(QStringLiteral(
        "font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;"
        "font-size:18px; font-weight:bold;"
        "color:#d97706;"             // Obsidian Accent
        "background:transparent;"
        "border:none;"));

    auto* subtitle = new QLabel(QStringLiteral("Inventory Management"), logoArea);
    subtitle->setStyleSheet(QStringLiteral(
        "font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;"
        "font-size:10px; font-weight:bold;"
        "color:#808080;"             // Obsidian TextSecondary
        "letter-spacing:0.02em;"
        "background:transparent;"
        "border:none;"));

    logoRow->addWidget(wordmark);
    logoRow->addWidget(subtitle);

    vlay->addWidget(logoArea);
    vlay->addSpacing(8);

    // ── Nav items ────────
    struct NavItem {
        char32_t icon;
        const char* label;
        const char* tip;
        Page page;
        bool bottom;
    };

    static const NavItem items[] = {
        { QtAwesome::Dashboard,   "Dashboard",    "Dashboard",       Page::Dashboard,  false },
        { QtAwesome::DailyEntry,  "Daily Entry",  "Daily Entry",     Page::DailyEntry, false },
        { QtAwesome::Stock,       "Stock In/Out", "Stock In / Out",  Page::Stock,      false },
        { QtAwesome::Products,    "Products",     "Products",        Page::Products,   false },
        { QtAwesome::Analytics,   "Analytics",    "Analytics",       Page::Analytics,  false },
        { QtAwesome::Import,      "Import",       "Import CSV",      Page::Import,     false },
    };

    auto* navArea = new QWidget(this);
    navArea->setStyleSheet("background:transparent;");
    auto* navLayout = new QVBoxLayout(navArea);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(2);

    for (const auto& item : items) {
        auto* btn = new SidebarButton(
            item.icon,
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
        QtAwesome::Settings,
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
