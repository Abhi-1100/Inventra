/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/MainWindow.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN6Kirana10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto Kirana::MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN6Kirana10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Kirana::MainWindow",
        "onPageSelected",
        "",
        "Sidebar::Page",
        "page",
        "onLoginSucceeded",
        "Kirana::StaffUser",
        "user",
        "onLoggedOut",
        "onRunNowClicked",
        "onPipelineStarted",
        "onPipelineProgress",
        "pct",
        "stage",
        "onPipelineFinished",
        "PipelineRunResult",
        "result",
        "onPipelineError",
        "msg",
        "onSessionLocked",
        "onSessionUnlocked",
        "onIdleTimeout"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onPageSelected'
        QtMocHelpers::SlotData<void(Sidebar::Page)>(1, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onLoginSucceeded'
        QtMocHelpers::SlotData<void(const Kirana::StaffUser &)>(5, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 6, 7 },
        }}),
        // Slot 'onLoggedOut'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRunNowClicked'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPipelineStarted'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPipelineProgress'
        QtMocHelpers::SlotData<void(int, const QString &)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 12 }, { QMetaType::QString, 13 },
        }}),
        // Slot 'onPipelineFinished'
        QtMocHelpers::SlotData<void(const PipelineRunResult &)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 15, 16 },
        }}),
        // Slot 'onPipelineError'
        QtMocHelpers::SlotData<void(const QString &)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 18 },
        }}),
        // Slot 'onSessionLocked'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSessionUnlocked'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onIdleTimeout'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN6Kirana10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Kirana::MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Kirana10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Kirana10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6Kirana10MainWindowE_t>.metaTypes,
    nullptr
} };

void Kirana::MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onPageSelected((*reinterpret_cast<std::add_pointer_t<Sidebar::Page>>(_a[1]))); break;
        case 1: _t->onLoginSucceeded((*reinterpret_cast<std::add_pointer_t<Kirana::StaffUser>>(_a[1]))); break;
        case 2: _t->onLoggedOut(); break;
        case 3: _t->onRunNowClicked(); break;
        case 4: _t->onPipelineStarted(); break;
        case 5: _t->onPipelineProgress((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 6: _t->onPipelineFinished((*reinterpret_cast<std::add_pointer_t<PipelineRunResult>>(_a[1]))); break;
        case 7: _t->onPipelineError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->onSessionLocked(); break;
        case 9: _t->onSessionUnlocked(); break;
        case 10: _t->onIdleTimeout(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Kirana::StaffUser >(); break;
            }
            break;
        }
    }
}

const QMetaObject *Kirana::MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Kirana::MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Kirana10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int Kirana::MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}
QT_WARNING_POP
