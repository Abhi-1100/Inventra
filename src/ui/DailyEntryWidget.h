#pragma once

#include <QWidget>
#include <QDate>
#include <QVector>
#include "core/ProductData.h"

class QComboBox;
class QSpinBox;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QPushButton;
class QTextEdit;
class QTabWidget;

namespace Kirana {

class AppController;
class AuthController;

// ─────────────────────────────────────────────
// DailyEntryWidget
//
// End-of-day (or real-time) manual sales + waste logging.
//   Tab 1: Single-entry form (product autocomplete,
//           units sold + wasted, quick-add)
//   Tab 2: Bulk paste mode (product,qty CSV)
//   Below tabs: running list of today's entries
// ─────────────────────────────────────────────

class DailyEntryWidget : public QWidget {
    Q_OBJECT

public:
    explicit DailyEntryWidget(AppController* controller,
                               AuthController* auth,
                               QWidget* parent = nullptr);

private slots:
    void onAddEntryClicked();
    void onDeleteEntryClicked();
    void onEditEntryClicked();
    void onBulkImportClicked();
    void onProductsChanged();
    void refreshList();

private:
    void buildLayout();
    void populateProductCombo();
    void saveEntry(int productId, const QString& productName, int sold, int wasted);

    AppController*  m_controller = nullptr;
    AuthController* m_auth       = nullptr;
    QDate           m_entryDate;

    // Single-entry form
    QTabWidget*  m_tabs        = nullptr;
    QComboBox*   m_productCombo= nullptr;
    QSpinBox*    m_soldSpin    = nullptr;
    QSpinBox*    m_wastedSpin  = nullptr;
    QPushButton* m_addBtn      = nullptr;

    // Bulk mode
    QTextEdit*   m_bulkEdit    = nullptr;
    QPushButton* m_bulkImport  = nullptr;
    QLabel*      m_bulkStatus  = nullptr;

    // Today's entries list
    QListWidget* m_entryList   = nullptr;
    QPushButton* m_deleteBtn   = nullptr;
    QPushButton* m_editBtn     = nullptr;
    QLabel*      m_dateLabel   = nullptr;
    QLabel*      m_summaryLabel= nullptr;

    QVector<DailyEntry> m_entries;
};

} // namespace Kirana
