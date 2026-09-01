/****************************************************************************
** Meta object code from reading C++ file 'AuthController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/core/AuthController.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AuthController.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN6Kirana14AuthControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto Kirana::AuthController::qt_create_metaobjectdata<qt_meta_tag_ZN6Kirana14AuthControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Kirana::AuthController",
        "registrationSucceeded",
        "",
        "Kirana::StaffUser",
        "user",
        "loginSucceeded",
        "loginFailed",
        "loggedOut",
        "sessionLocked",
        "sessionUnlocked",
        "staffChanged",
        "shopProfileChanged",
        "Kirana::ShopProfile",
        "shop"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'registrationSucceeded'
        QtMocHelpers::SignalData<void(const Kirana::StaffUser &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'loginSucceeded'
        QtMocHelpers::SignalData<void(const Kirana::StaffUser &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'loginFailed'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'loggedOut'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'sessionLocked'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'sessionUnlocked'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'staffChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'shopProfileChanged'
        QtMocHelpers::SignalData<void(const Kirana::ShopProfile &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AuthController, qt_meta_tag_ZN6Kirana14AuthControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Kirana::AuthController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Kirana14AuthControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Kirana14AuthControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6Kirana14AuthControllerE_t>.metaTypes,
    nullptr
} };

void Kirana::AuthController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AuthController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->registrationSucceeded((*reinterpret_cast<std::add_pointer_t<Kirana::StaffUser>>(_a[1]))); break;
        case 1: _t->loginSucceeded((*reinterpret_cast<std::add_pointer_t<Kirana::StaffUser>>(_a[1]))); break;
        case 2: _t->loginFailed(); break;
        case 3: _t->loggedOut(); break;
        case 4: _t->sessionLocked(); break;
        case 5: _t->sessionUnlocked(); break;
        case 6: _t->staffChanged(); break;
        case 7: _t->shopProfileChanged((*reinterpret_cast<std::add_pointer_t<Kirana::ShopProfile>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Kirana::StaffUser >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Kirana::StaffUser >(); break;
            }
            break;
        case 7:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Kirana::ShopProfile >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)(const Kirana::StaffUser & )>(_a, &AuthController::registrationSucceeded, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)(const Kirana::StaffUser & )>(_a, &AuthController::loginSucceeded, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::loginFailed, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::loggedOut, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::sessionLocked, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::sessionUnlocked, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::staffChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)(const Kirana::ShopProfile & )>(_a, &AuthController::shopProfileChanged, 7))
            return;
    }
}

const QMetaObject *Kirana::AuthController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Kirana::AuthController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Kirana14AuthControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Kirana::AuthController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void Kirana::AuthController::registrationSucceeded(const Kirana::StaffUser & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void Kirana::AuthController::loginSucceeded(const Kirana::StaffUser & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void Kirana::AuthController::loginFailed()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void Kirana::AuthController::loggedOut()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void Kirana::AuthController::sessionLocked()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void Kirana::AuthController::sessionUnlocked()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void Kirana::AuthController::staffChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void Kirana::AuthController::shopProfileChanged(const Kirana::ShopProfile & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}
QT_WARNING_POP
