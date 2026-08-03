#include <QCoreApplication>
#include <QDebug>
#include "python/PythonBridge.h"
#include "core/AppController.h"
#include "core/Database.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    
    Kirana::AppSettings s;
    s.dbPath = "build/Inventra.app/Contents/MacOS/inventra.db";
    
    qDebug() << "Initializing bridge...";
    Kirana::PythonBridge::instance().initialize("build/Inventra.app/Contents/MacOS/python");
    
    qDebug() << "Running pipeline...";
    auto res = Kirana::PythonBridge::instance().runPipeline("/Users/ghoriommukeshbai/Desktop/research paper/model/kirana_inventory_dataset_v2_polished.csv", s);
    
    qDebug() << "Success:" << res.success << "Error:" << res.errorMsg << "Rows:" << res.totalRows << "isImport:" << res.isImport;
    return 0;
}
