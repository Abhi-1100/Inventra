#include <QCoreApplication>
#include <QDebug>
#include "python/PythonBridge.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    Kirana::AppSettings s;
    s.dbPath = "build/inventra.db";
    Kirana::PythonBridge::instance().initialize("build/python");
    qDebug() << "Running pipeline...";
    auto res = Kirana::PythonBridge::instance().runPipeline("/Users/ghoriommukeshbai/Desktop/research paper/model/kirana_inventory_dataset_v2_polished.csv", s);
    qDebug() << "Success:" << res.success << "Error:" << res.errorMsg << "Rows:" << res.totalRows << "isImport:" << res.isImport;
    return 0;
}
