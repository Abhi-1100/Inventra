/****************************************************************************
** Meta object code from reading C++ file 'AppController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/core/AppController.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AppController.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN6Kirana13AppControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto Kirana::AppController::qt_create_metaobjectdata<qt_meta_tag_ZN6Kirana13AppControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Kirana::AppController",
        "productsChanged",
        "",
        "QList<Product>",
        "products",
        "pipelineStateChanged",
        "live",
        "lastRun",
        "settingsChanged",
        "AppSettings",
        "s"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'productsChanged'
        QtMocHelpers::SignalData<void(const QVector<Product> &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'pipelineStateChanged'
        QtMocHelpers::SignalData<void(bool, QDateTime)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 6 }, { QMetaType::QDateTime, 7 },
        }}),
        // Signal 'settingsChanged'
        QtMocHelpers::SignalData<void(const AppSettings &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 10 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AppController, qt_meta_tag_ZN6Kirana13AppControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Kirana::AppController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Kirana13AppControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Kirana13AppControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6Kirana13AppControllerE_t>.metaTypes,
    nullptr
} };

void Kirana::AppController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AppController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->productsChanged((*reinterpret_cast<std::add_pointer_t<QList<Product>>>(_a[1]))); break;
        case 1: _t->pipelineStateChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QDateTime>>(_a[2]))); break;
        case 2: _t->settingsChanged((*reinterpret_cast<std::add_pointer_t<AppSettings>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AppController::*)(const QVector<Product> & )>(_a, &AppController::productsChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppController::*)(bool , QDateTime )>(_a, &AppController::pipelineStateChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppController::*)(const AppSettings & )>(_a, &AppController::settingsChanged, 2))
            return;
    }
}

const QMetaObject *Kirana::AppController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Kirana::AppController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Kirana13AppControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Kirana::AppController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void Kirana::AppController::productsChanged(const QVector<Product> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void Kirana::AppController::pipelineStateChanged(bool _t1, QDateTime _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void Kirana::AppController::settingsChanged(const AppSettings & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}
QT_WARNING_POP
