#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <iostream>
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    auto db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("C:/Inventra/build/inventra.db");
    if (!db.open()) { std::cout << "Open Error: " << db.lastError().text().toStdString() << "\n"; return 1; }
    QSqlQuery q("SELECT COUNT(*) FROM products", db);
    if (q.next()) { std::cout << "Count: " << q.value(0).toInt() << "\n"; }
    else { std::cout << "Query Error: " << q.lastError().text().toStdString() << "\n"; }
    return 0;
}
