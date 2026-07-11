#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QTableWidget;
class QLabel;

namespace Kirana {

class AppController;

// ─────────────────────────────────────────────
// ImportWidget
//
// Allows user to pick a CSV data file, map headers,
// perform dry-run verification, and run the pipeline.
// ─────────────────────────────────────────────

class ImportWidget : public QWidget {
    Q_OBJECT

public:
    explicit ImportWidget(AppController* controller, QWidget* parent = nullptr);
    ~ImportWidget() override = default;

signals:
    void pipelineRunRequested(const QString& csvPath);

private slots:
    void onBrowseClicked();
    void onRunClicked();
    void onCsvSelected(const QString& path);

private:
    void buildLayout();
    void validateFile();

    AppController* m_controller = nullptr;

    QLineEdit*    m_filePathEdit = nullptr;
    QPushButton*  m_browseBtn    = nullptr;
    QPushButton*  m_runBtn       = nullptr;

    QTableWidget* m_mappingTable = nullptr;
    QLabel*       m_statusLabel  = nullptr;

    QString m_selectedPath;
};

} // namespace Kirana
