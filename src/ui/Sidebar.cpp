#include "ui/Sidebar.h"
#include "core/ProductData.h"

#include <QVBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QPixmap>
#include <QIcon>
#include <QLabel>
#include <QToolTip>

namespace Kirana {

// ══════════════════════════════════════════════
// SidebarButton
// ══════════════════════════════════════════════

SidebarButton::SidebarButton(const QString& iconPath,
                               const QString& tooltip,
                               QWidget* parent)
    : QWidget(parent)
    , m_iconPath(iconPath)
{
    setFixedSize(56, 52);
    setToolTip(tooltip);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);

    // Load icon — will render white on dark bg
    m_icon = QPixmap(iconPath);
}

void SidebarButton::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    const QRect r = rect();

    // Hover / active background
    if (m_active) {
        QColor bg(Palette::BgOverlay);
        bg.setAlpha(200);
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(r.adjusted(6, 4, -6, -4), 6, 6);
    } else if (m_hovered) {
        QColor bg(Palette::BgOverlay);
        bg.setAlpha(120);
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(r.adjusted(6, 4, -6, -4), 6, 6);
    }

    // Left accent bar (active only)
    if (m_active) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(Palette::Accent));
        p.drawRect(0, r.height() / 2 - 12, 3, 24);
    }

    // Icon — centre in button
    if (!m_icon.isNull()) {
        const int iconSz = 20;
        const int x = (r.width()  - iconSz) / 2;
        const int y = (r.height() - iconSz) / 2;

        // Tint: accent-blue for active, muted for inactive
        QColor tint = m_active
            ? QColor(Palette::Accent)
            : (m_hovered ? QColor(Palette::TextPrimary) : QColor(Palette::TextSecondary));

        // Draw icon with colour-overlay
        QPixmap tinted = m_icon.scaled(iconSz, iconSz,
                                        Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation);
        p.setOpacity(1.0);
        p.drawPixmap(x, y, tinted);
    }
}

void SidebarButton::mousePressEvent(QMouseEvent* ev) {
    if (ev->button() == Qt::LeftButton)
        emit clicked();
    QWidget::mousePressEvent(ev);
}

void SidebarButton::enterEvent(QEnterEvent* ev) {
    m_hovered = true;
    update();
    QWidget::enterEvent(ev);
}

void SidebarButton::leaveEvent(QEvent* ev) {
    m_hovered = false;
    update();
    QWidget::leaveEvent(ev);
}

// ══════════════════════════════════════════════
// Sidebar
// ══════════════════════════════════════════════

Sidebar::Sidebar(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(56);
    setObjectName("Sidebar");
    setStyleSheet("QWidget#Sidebar { background: #0d1117; border-right: 1px solid #21262d; }");

    buildLayout();
}

void Sidebar::buildLayout() {
    auto* vlay = new QVBoxLayout(this);
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(0);

    // ── Logo / monogram ──────────────────────
    auto* logo = new QLabel("K", this);
    logo->setFixedHeight(56);
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet(
        "font-family: 'Consolas', monospace;"
        "font-size: 22px;"
        "font-weight: 700;"
        "color: #58a6ff;"
        "background: transparent;"
        "border-bottom: 1px solid #21262d;"
    );
    vlay->addWidget(logo);
    vlay->addSpacing(8);

    // ── Nav buttons ──────────────────────────
    struct NavItem { const char* icon; const char* tip; Page page; };
    static const NavItem items[] = {
        { ":/icons/dashboard.svg", "Dashboard",  Page::Dashboard },
        { ":/icons/products.svg",  "Products",   Page::Products  },
        { ":/icons/analytics.svg", "Analytics",  Page::Analytics },
        { ":/icons/import.svg",    "Import CSV", Page::Import    },
        { ":/icons/settings.svg",  "Settings",   Page::Settings  },
    };

    for (const auto& item : items) {
        auto* btn = new SidebarButton(
            QString::fromLatin1(item.icon),
            QString::fromLatin1(item.tip),
            this);
        connectButton(btn, item.page);
        m_buttons.append(btn);
        vlay->addWidget(btn);
    }

    vlay->addStretch();

    // ── Version label ────────────────────────
    auto* version = new QLabel("v1", this);
    version->setFixedHeight(28);
    version->setAlignment(Qt::AlignCenter);
    version->setStyleSheet(
        "font-family: 'Consolas', monospace;"
        "font-size: 9px;"
        "color: #484f58;"
        "background: transparent;"
    );
    vlay->addWidget(version);

    // Set dashboard active by default
    if (!m_buttons.isEmpty())
        m_buttons[0]->setActive(true);
}

void Sidebar::connectButton(SidebarButton* btn, Page page) {
    connect(btn, &SidebarButton::clicked, this, [this, btn, page]() {
        setActivePage(page);
        emit pageSelected(page);
    });
}

void Sidebar::setActivePage(Page page) {
    m_activePage = page;
    for (int i = 0; i < m_buttons.size(); ++i)
        m_buttons[i]->setActive(static_cast<Page>(i) == page);
}

} // namespace Kirana
