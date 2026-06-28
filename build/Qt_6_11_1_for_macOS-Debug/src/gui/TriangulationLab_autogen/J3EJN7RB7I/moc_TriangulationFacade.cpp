/****************************************************************************
** Meta object code from reading C++ file 'TriangulationFacade.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../../src/gui/facade/TriangulationFacade.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TriangulationFacade.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN6facade19TriangulationFacadeE_t {};
} // unnamed namespace

template <> constexpr inline auto facade::TriangulationFacade::qt_create_metaobjectdata<qt_meta_tag_ZN6facade19TriangulationFacadeE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "facade::TriangulationFacade",
        "polygonChanged",
        "",
        "std::vector<QPointF>",
        "points",
        "triangulationCompleted",
        "std::vector<std::array<QPointF,3>>",
        "triangles",
        "debugStepChanged",
        "currentStep",
        "totalSteps",
        "errorOccurred",
        "message",
        "statusChanged"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'polygonChanged'
        QtMocHelpers::SignalData<void(const std::vector<QPointF> &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'triangulationCompleted'
        QtMocHelpers::SignalData<void(const std::vector<std::array<QPointF,3>> &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 7 },
        }}),
        // Signal 'debugStepChanged'
        QtMocHelpers::SignalData<void(int, int)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::Int, 10 },
        }}),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 12 },
        }}),
        // Signal 'statusChanged'
        QtMocHelpers::SignalData<void(const QString &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 12 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TriangulationFacade, qt_meta_tag_ZN6facade19TriangulationFacadeE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject facade::TriangulationFacade::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6facade19TriangulationFacadeE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6facade19TriangulationFacadeE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6facade19TriangulationFacadeE_t>.metaTypes,
    nullptr
} };

void facade::TriangulationFacade::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TriangulationFacade *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->polygonChanged((*reinterpret_cast<std::add_pointer_t<std::vector<QPointF>>>(_a[1]))); break;
        case 1: _t->triangulationCompleted((*reinterpret_cast<std::add_pointer_t<std::vector<std::array<QPointF,3>>>>(_a[1]))); break;
        case 2: _t->debugStepChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 3: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->statusChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (TriangulationFacade::*)(const std::vector<QPointF> & )>(_a, &TriangulationFacade::polygonChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (TriangulationFacade::*)(const std::vector<std::array<QPointF,3>> & )>(_a, &TriangulationFacade::triangulationCompleted, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (TriangulationFacade::*)(int , int )>(_a, &TriangulationFacade::debugStepChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (TriangulationFacade::*)(const QString & )>(_a, &TriangulationFacade::errorOccurred, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (TriangulationFacade::*)(const QString & )>(_a, &TriangulationFacade::statusChanged, 4))
            return;
    }
}

const QMetaObject *facade::TriangulationFacade::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *facade::TriangulationFacade::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6facade19TriangulationFacadeE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int facade::TriangulationFacade::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void facade::TriangulationFacade::polygonChanged(const std::vector<QPointF> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void facade::TriangulationFacade::triangulationCompleted(const std::vector<std::array<QPointF,3>> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void facade::TriangulationFacade::debugStepChanged(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void facade::TriangulationFacade::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void facade::TriangulationFacade::statusChanged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}
QT_WARNING_POP
