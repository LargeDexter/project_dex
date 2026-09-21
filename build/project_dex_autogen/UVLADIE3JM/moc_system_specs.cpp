/****************************************************************************
** Meta object code from reading C++ file 'system_specs.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/system_specs.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'system_specs.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.2. It"
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
struct qt_meta_tag_ZN11SystemSpecsE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN11SystemSpecsE = QtMocHelpers::stringData(
    "SystemSpecs",
    "matchLevel",
    "",
    "label",
    "value",
    "cpuModel",
    "cpuCores",
    "cpuGhz",
    "ramGiB",
    "gpuModel",
    "gpuVramGiB"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN11SystemSpecsE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       6,   25, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   20,    2, 0x102,    7 /* Public | MethodIsConst  */,

 // methods: parameters
    QMetaType::QString, QMetaType::QString, QMetaType::QString,    3,    4,

 // properties: name, type, flags, notifyId, revision
       5, QMetaType::QString, 0x00015401, uint(-1), 0,
       6, QMetaType::Int, 0x00015401, uint(-1), 0,
       7, QMetaType::Double, 0x00015401, uint(-1), 0,
       8, QMetaType::Double, 0x00015401, uint(-1), 0,
       9, QMetaType::QString, 0x00015401, uint(-1), 0,
      10, QMetaType::Double, 0x00015401, uint(-1), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject SystemSpecs::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN11SystemSpecsE.offsetsAndSizes,
    qt_meta_data_ZN11SystemSpecsE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN11SystemSpecsE_t,
        // property 'cpuModel'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'cpuCores'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'cpuGhz'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'ramGiB'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'gpuModel'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'gpuVramGiB'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SystemSpecs, std::true_type>,
        // method 'matchLevel'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void SystemSpecs::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SystemSpecs *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: { QString _r = _t->matchLevel((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->cpuModel(); break;
        case 1: *reinterpret_cast< int*>(_v) = _t->cpuCores(); break;
        case 2: *reinterpret_cast< double*>(_v) = _t->cpuGhz(); break;
        case 3: *reinterpret_cast< double*>(_v) = _t->ramGiB(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->gpuModel(); break;
        case 5: *reinterpret_cast< double*>(_v) = _t->gpuVramGiB(); break;
        default: break;
        }
    }
}

const QMetaObject *SystemSpecs::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SystemSpecs::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN11SystemSpecsE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SystemSpecs::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
