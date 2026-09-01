/****************************************************************************
** Meta object code from reading C++ file 'DailyEntryWidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/DailyEntryWidget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DailyEntryWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN6Kirana16DailyEntryWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto Kirana::DailyEntryWidget::qt_create_metaobjectdata<qt_meta_tag_ZN6Kirana16DailyEntryWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Kirana::DailyEntryWidget",
        "onAddEntryClicked",
        "",
        "onDeleteEntryClicked",
        "onEditEntryClicked",
        "onBulkImportClicked",
        "onProductsChanged",
        "refreshList"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onAddEntryClicked'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDeleteEntryClicked'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onEditEntryClicked'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onBulkImportClicked'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onProductsChanged'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'refreshList'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DailyEntryWidget, qt_meta_tag_ZN6Kirana16DailyEntryWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Kirana::DailyEntryWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Kirana16DailyEntryWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Kirana16DailyEntryWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6Kirana16DailyEntryWidgetE_t>.metaTypes,
    nullptr
} };

void Kirana::DailyEntryWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DailyEntryWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onAddEntryClicked(); break;
        case 1: _t->onDeleteEntryClicked(); break;
        case 2: _t->onEditEntryClicked(); break;
        case 3: _t->onBulkImportClicked(); break;
        case 4: _t->onProductsChanged(); break;
        case 5: _t->refreshList(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *Kirana::DailyEntryWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Kirana::DailyEntryWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Kirana16DailyEntryWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int Kirana::DailyEntryWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
