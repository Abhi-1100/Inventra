#pragma once

#include <QWidget>
#include <QDate>
#include "core/ProductData.h"

class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;
class QDateEdit;
class QTableView;
class QStandardItemModel;
class QSortFilterProxyModel;
class QPushButton;

namespace Kirana {

class AppController;
class AuthController;

// ─────────────────────────────────────────────
// StockWidget
//
// Two tabs: Stock In (Restocks) and Stock Out (Adjustments)
// Below tabs: Running ledger of movements, exportable to CSV
// ─────────────────────────────────────────────

class StockWidget : public QWidget {
    Q_OBJECT

public:
    explicit StockWidget(AppController* controller,
                         AuthController* auth,
                         QWidget* parent = nullptr);

private slots:
    void onStockInSaved();
    void onStockOutSaved();
    void onExportCsv();
    void onFilterChanged();
    void onProductsChanged();

private:
    void buildLayout();
    void populateProductCombos();
    void loadLedger();

    AppController*  m_controller = nullptr;
    AuthController* m_auth       = nullptr;

    // Tabs & Forms
    QComboBox*      m_inProductCombo = nullptr;
    QSpinBox*       m_inQtySpin      = nullptr;
    QLineEdit*      m_inSupplierEdit = nullptr;
    QDoubleSpinBox* m_inCostSpin     = nullptr;
    QDateEdit*      m_inDateEdit     = nullptr;

    QComboBox*      m_outProductCombo = nullptr;
    QSpinBox*       m_outQtySpin      = nullptr;
    QComboBox*      m_outReasonCombo  = nullptr;
    QDateEdit*      m_outDateEdit     = nullptr;

    // Ledger Filter
    QDateEdit*      m_filterFromDate = nullptr;
    QDateEdit*      m_filterToDate   = nullptr;
    QComboBox*      m_filterTypeCombo = nullptr;

    // Ledger View
    QTableView*            m_ledgerTable = nullptr;
    QStandardItemModel*    m_ledgerModel = nullptr;
    QSortFilterProxyModel* m_proxyModel  = nullptr;
};

} // namespace Kirana
