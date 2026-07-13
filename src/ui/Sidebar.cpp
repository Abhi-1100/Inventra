#include "core/ThemeManager.h"
#include "ui/Sidebar.h"
#include "core/ProductData.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QPixmap>
#include <QLabel>
#include <QFontMetrics>

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

    // Hover / active background
    if (m_active) {
        QColor bg(ThemeManager::instance().tokens().Accent);
        bg.setAlpha(26); // 10% opacity
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(r.adjusted(8, 3, -8, -3), 6, 6);

        // Left accent bar
        p.setBrush(ThemeManager::instance().tokens().Accent);
        p.drawRect(0, r.height() / 2 - 12, 4, 24);

    } else if (m_hovered) {
        QColor bg(ThemeManager::instance().tokens().Accent);
        bg.setAlpha(13); // 5% opacity
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(r.adjusted(8, 3, -8, -3), 6, 6);
    }

    // Icon — left side (after accent bar)
    const int iconSz  = 18;
    const int iconX   = 20;
    const int iconY   = (r.height() - iconSz) / 2;

    if (!m_icon.isNull()) {
        QPixmap tinted = m_icon.scaled(iconSz, iconSz,
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);

        // Paint icon with colour-overlay using painter compositing
        QColor tintColor = m_active
            ? ThemeManager::instance().tokens().Accent
            : (m_hovered ? ThemeManager::instance().tokens().TextPrimary : ThemeManager::instance().tokens().TextSecondary);

        p.setOpacity(1.0);
        // Draw icon (SVG already contains stroke="currentColor";
        // tinting approach: draw icon at full opacity and overlay)
        p.drawPixmap(iconX, iconY, tinted);

        // Colour-tint the icon by painting the tint color in SourceIn mode
        p.save();
        p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        p.fillRect(QRect(iconX, iconY, iconSz, iconSz), tintColor);
        p.restore();
    }

    // Label text
    const int textX = iconX + iconSz + 12;
    QColor textColor = m_active
        ? ThemeManager::instance().tokens().Accent
        : (m_hovered ? ThemeManager::instance().tokens().TextPrimary : ThemeManager::instance().tokens().TextSecondary);

    p.setPen(textColor);
    QFont f = p.font();
    f.setFamily(QStringLiteral("Hanken Grotesk"));
    f.setPixelSize(13);
    f.setWeight(m_active ? QFont::Medium : QFont::Normal);
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
    // Styles moved to QSS
    buildLayout();
}

void Sidebar::buildLayout() {
    auto* vlay = new QVBoxLayout(this);
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(2);

    // ── Logo area ──────────────────────────────
    auto* logoArea = new QWidget(this);
    logoArea->setFixedHeight(60);
    logoArea->setStyleSheet(QStringLiteral(
        "border-bottom:1px solid #21262d; background:transparent;"));

    auto* logoRow = new QHBoxLayout(logoArea);
    logoRow->setContentsMargins(16, 0, 16, 0);
    logoRow->setSpacing(8);

    auto* logoIcon = new QLabel(logoArea);
    logoIcon->setPixmap(
        QPixmap(QStringLiteral(":/icons/inventra_logo.svg")).scaled(
            22, 22, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto* wordmark = new QLabel(QStringLiteral("Inventra"), logoArea);
    wordmark->setStyleSheet(QStringLiteral(
        "font-family:'Segoe UI','Inter',sans-serif;"
        "font-size:15px; font-weight:600;"
        "color:#e6edf3; background:transparent;"
        "border:none;"));

    logoRow->addWidget(logoIcon);
    logoRow->addWidget(wordmark);
    logoRow->addStretch();

    vlay->addWidget(logoArea);
    vlay->addSpacing(6);

    // ── Nav buttons ────────────────────────────
    struct NavItem {
        const char* icon;
        const char* label;
        const char* tip;
        Page page;
    };

    static const NavItem items[] = {
        { ":/icons/dashboard.svg",   "Dashboard",    "Dashboard",       Page::Dashboard  },
        { ":/icons/daily_entry.svg", "Daily Entry",  "Daily Entry",     Page::DailyEntry },
        { ":/icons/stock.svg",       "Stock In/Out", "Stock In / Out",  Page::Stock      },
        { ":/icons/products.svg",    "Products",     "Products",        Page::Products   },
        { ":/icons/analytics.svg",   "Analytics",    "Analytics",       Page::Analytics  },
        { ":/icons/import.svg",      "Import",       "Import CSV",      Page::Import     },
        { ":/icons/settings.svg",    "Settings",     "Settings",        Page::Settings   },
    };

    for (const auto& item : items) {
        auto* btn = new SidebarButton(
            QString::fromLatin1(item.icon),
            QString::fromLatin1(item.label),
            QString::fromLatin1(item.tip),
            this);
        connectButton(btn, item.page);
        m_buttons.append(btn);
        vlay->addWidget(btn);
    }

    vlay->addStretch();

    // ── Version / role label ───────────────────
    auto* version = new QLabel(QStringLiteral("v1.0.0"), this);
    version->setFixedHeight(28);
    version->setAlignment(Qt::AlignCenter);
    version->setStyleSheet(QStringLiteral(
        "font-size:9px; color:#6e7681; background:transparent; border:none;"));
    vlay->addWidget(version);

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
    // Staff cannot see Analytics (idx 4), Import (5), Settings (6)
    const bool isOwner = (role == QLatin1String("Owner"));
    if (m_buttons.size() >= 7) {
        m_buttons[4]->setVisible(isOwner);   // Analytics
        m_buttons[5]->setVisible(isOwner);   // Import
        m_buttons[6]->setVisible(isOwner);   // Settings
    }
}

} // namespace Kirana
