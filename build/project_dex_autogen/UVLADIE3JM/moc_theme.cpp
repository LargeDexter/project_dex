/****************************************************************************
** Meta object code from reading C++ file 'theme.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/theme.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'theme.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN5ThemeE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN5ThemeE = QtMocHelpers::stringData(
    "Theme",
    "background",
    "surface",
    "surfaceHighlight",
    "accent",
    "accentGlow",
    "textPrimary",
    "textSecondary",
    "radiusSmall",
    "radiusMedium",
    "radiusPill",
    "spacingSmall",
    "spacingMedium",
    "spacingLarge",
    "fontSizeSmall",
    "fontSizeMedium",
    "fontSizeLarge",
    "glowRingWidth"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN5ThemeE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
      17,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags, notifyId, revision
       1, QMetaType::QColor, 0x00015401, uint(-1), 0,
       2, QMetaType::QColor, 0x00015401, uint(-1), 0,
       3, QMetaType::QColor, 0x00015401, uint(-1), 0,
       4, QMetaType::QColor, 0x00015401, uint(-1), 0,
       5, QMetaType::QColor, 0x00015401, uint(-1), 0,
       6, QMetaType::QColor, 0x00015401, uint(-1), 0,
       7, QMetaType::QColor, 0x00015401, uint(-1), 0,
       8, QMetaType::Int, 0x00015401, uint(-1), 0,
       9, QMetaType::Int, 0x00015401, uint(-1), 0,
      10, QMetaType::Int, 0x00015401, uint(-1), 0,
      11, QMetaType::Int, 0x00015401, uint(-1), 0,
      12, QMetaType::Int, 0x00015401, uint(-1), 0,
      13, QMetaType::Int, 0x00015401, uint(-1), 0,
      14, QMetaType::Int, 0x00015401, uint(-1), 0,
      15, QMetaType::Int, 0x00015401, uint(-1), 0,
      16, QMetaType::Int, 0x00015401, uint(-1), 0,
      17, QMetaType::Int, 0x00015401, uint(-1), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject Theme::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN5ThemeE.offsetsAndSizes,
    qt_meta_data_ZN5ThemeE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN5ThemeE_t,
        // property 'background'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'surface'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'surfaceHighlight'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'accent'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'accentGlow'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'textPrimary'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'textSecondary'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'radiusSmall'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'radiusMedium'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'radiusPill'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'spacingSmall'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'spacingMedium'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'spacingLarge'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'fontSizeSmall'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'fontSizeMedium'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'fontSizeLarge'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'glowRingWidth'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<Theme, std::true_type>
    >,
    nullptr
} };

void Theme::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Theme *>(_o);
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QColor*>(_v) = _t->background(); break;
        case 1: *reinterpret_cast< QColor*>(_v) = _t->surface(); break;
        case 2: *reinterpret_cast< QColor*>(_v) = _t->surfaceHighlight(); break;
        case 3: *reinterpret_cast< QColor*>(_v) = _t->accent(); break;
        case 4: *reinterpret_cast< QColor*>(_v) = _t->accentGlow(); break;
        case 5: *reinterpret_cast< QColor*>(_v) = _t->textPrimary(); break;
        case 6: *reinterpret_cast< QColor*>(_v) = _t->textSecondary(); break;
        case 7: *reinterpret_cast< int*>(_v) = _t->radiusSmall(); break;
        case 8: *reinterpret_cast< int*>(_v) = _t->radiusMedium(); break;
        case 9: *reinterpret_cast< int*>(_v) = _t->radiusPill(); break;
        case 10: *reinterpret_cast< int*>(_v) = _t->spacingSmall(); break;
        case 11: *reinterpret_cast< int*>(_v) = _t->spacingMedium(); break;
        case 12: *reinterpret_cast< int*>(_v) = _t->spacingLarge(); break;
        case 13: *reinterpret_cast< int*>(_v) = _t->fontSizeSmall(); break;
        case 14: *reinterpret_cast< int*>(_v) = _t->fontSizeMedium(); break;
        case 15: *reinterpret_cast< int*>(_v) = _t->fontSizeLarge(); break;
        case 16: *reinterpret_cast< int*>(_v) = _t->glowRingWidth(); break;
        default: break;
        }
    }
}

const QMetaObject *Theme::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Theme::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN5ThemeE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Theme::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    return _id;
}
QT_WARNING_POP
