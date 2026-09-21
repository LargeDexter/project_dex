/****************************************************************************
** Meta object code from reading C++ file 'game_library_model.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/game_library_model.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'game_library_model.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16GameLibraryModelE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN16GameLibraryModelE = QtMocHelpers::stringData(
    "GameLibraryModel",
    "syncingChanged",
    "",
    "syncFailed",
    "error",
    "countChanged",
    "heroGamesChanged",
    "categoryRowsChanged",
    "storeDetailsReady",
    "appId",
    "protonDbRatingReady",
    "trailerCached",
    "refresh",
    "gameAt",
    "QVariantMap",
    "index",
    "gameByAppId",
    "storeDetailsFor",
    "requestStoreDetails",
    "protonDbRatingFor",
    "requestProtonDbRating",
    "cachedTrailerUrlFor",
    "requestTrailerCache",
    "remoteUrl",
    "launchGameViaSteamCli",
    "onGameReady",
    "GameEntry",
    "entry",
    "onInstallStateChanged",
    "onSyncFinished",
    "syncing",
    "count",
    "heroGames",
    "QVariantList",
    "categoryRows"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN16GameLibraryModelE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      21,   14, // methods
       4,  191, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  140,    2, 0x06,    5 /* Public */,
       3,    1,  141,    2, 0x06,    6 /* Public */,
       5,    0,  144,    2, 0x06,    8 /* Public */,
       6,    0,  145,    2, 0x06,    9 /* Public */,
       7,    0,  146,    2, 0x06,   10 /* Public */,
       8,    1,  147,    2, 0x06,   11 /* Public */,
      10,    1,  150,    2, 0x06,   13 /* Public */,
      11,    1,  153,    2, 0x06,   15 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      12,    0,  156,    2, 0x0a,   17 /* Public */,
      13,    1,  157,    2, 0x10a,   18 /* Public | MethodIsConst  */,
      16,    1,  160,    2, 0x10a,   20 /* Public | MethodIsConst  */,
      17,    1,  163,    2, 0x10a,   22 /* Public | MethodIsConst  */,
      18,    1,  166,    2, 0x0a,   24 /* Public */,
      19,    1,  169,    2, 0x10a,   26 /* Public | MethodIsConst  */,
      20,    1,  172,    2, 0x0a,   28 /* Public */,
      21,    1,  175,    2, 0x10a,   30 /* Public | MethodIsConst  */,
      22,    2,  178,    2, 0x0a,   32 /* Public */,
      24,    1,  183,    2, 0x10a,   35 /* Public | MethodIsConst  */,
      25,    1,  186,    2, 0x08,   37 /* Private */,
      28,    0,  189,    2, 0x08,   39 /* Private */,
      29,    0,  190,    2, 0x08,   40 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong,    9,
    QMetaType::Void, QMetaType::LongLong,    9,
    QMetaType::Void, QMetaType::LongLong,    9,

 // slots: parameters
    QMetaType::Void,
    0x80000000 | 14, QMetaType::Int,   15,
    0x80000000 | 14, QMetaType::LongLong,    9,
    0x80000000 | 14, QMetaType::LongLong,    9,
    QMetaType::Void, QMetaType::LongLong,    9,
    0x80000000 | 14, QMetaType::LongLong,    9,
    QMetaType::Void, QMetaType::LongLong,    9,
    QMetaType::QString, QMetaType::LongLong,    9,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString,    9,   23,
    QMetaType::Bool, QMetaType::LongLong,    9,
    QMetaType::Void, 0x80000000 | 26,   27,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
      30, QMetaType::Bool, 0x00015001, uint(0), 0,
      31, QMetaType::Int, 0x00015001, uint(2), 0,
      32, 0x80000000 | 33, 0x00015009, uint(3), 0,
      34, 0x80000000 | 33, 0x00015009, uint(4), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject GameLibraryModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractListModel::staticMetaObject>(),
    qt_meta_stringdata_ZN16GameLibraryModelE.offsetsAndSizes,
    qt_meta_data_ZN16GameLibraryModelE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN16GameLibraryModelE_t,
        // property 'syncing'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'count'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'heroGames'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'categoryRows'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<GameLibraryModel, std::true_type>,
        // method 'syncingChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'syncFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'countChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'heroGamesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'categoryRowsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'storeDetailsReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'protonDbRatingReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'trailerCached'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'refresh'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'gameAt'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'gameByAppId'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'storeDetailsFor'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'requestStoreDetails'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'protonDbRatingFor'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'requestProtonDbRating'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'cachedTrailerUrlFor'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'requestTrailerCache'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'launchGameViaSteamCli'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'onGameReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const GameEntry &, std::false_type>,
        // method 'onInstallStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSyncFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void GameLibraryModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GameLibraryModel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->syncingChanged(); break;
        case 1: _t->syncFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->countChanged(); break;
        case 3: _t->heroGamesChanged(); break;
        case 4: _t->categoryRowsChanged(); break;
        case 5: _t->storeDetailsReady((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 6: _t->protonDbRatingReady((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 7: _t->trailerCached((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 8: _t->refresh(); break;
        case 9: { QVariantMap _r = _t->gameAt((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 10: { QVariantMap _r = _t->gameByAppId((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 11: { QVariantMap _r = _t->storeDetailsFor((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 12: _t->requestStoreDetails((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 13: { QVariantMap _r = _t->protonDbRatingFor((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 14: _t->requestProtonDbRating((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 15: { QString _r = _t->cachedTrailerUrlFor((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 16: _t->requestTrailerCache((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 17: { bool _r = _t->launchGameViaSteamCli((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 18: _t->onGameReady((*reinterpret_cast< std::add_pointer_t<GameEntry>>(_a[1]))); break;
        case 19: _t->onInstallStateChanged(); break;
        case 20: _t->onSyncFinished(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 18:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< GameEntry >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (GameLibraryModel::*)();
            if (_q_method_type _q_method = &GameLibraryModel::syncingChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (GameLibraryModel::*)(const QString & );
            if (_q_method_type _q_method = &GameLibraryModel::syncFailed; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (GameLibraryModel::*)();
            if (_q_method_type _q_method = &GameLibraryModel::countChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (GameLibraryModel::*)();
            if (_q_method_type _q_method = &GameLibraryModel::heroGamesChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (GameLibraryModel::*)();
            if (_q_method_type _q_method = &GameLibraryModel::categoryRowsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (GameLibraryModel::*)(qint64 );
            if (_q_method_type _q_method = &GameLibraryModel::storeDetailsReady; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (GameLibraryModel::*)(qint64 );
            if (_q_method_type _q_method = &GameLibraryModel::protonDbRatingReady; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (GameLibraryModel::*)(qint64 );
            if (_q_method_type _q_method = &GameLibraryModel::trailerCached; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isSyncing(); break;
        case 1: *reinterpret_cast< int*>(_v) = _t->count(); break;
        case 2: *reinterpret_cast< QVariantList*>(_v) = _t->heroGames(); break;
        case 3: *reinterpret_cast< QVariantList*>(_v) = _t->categoryRows(); break;
        default: break;
        }
    }
}

const QMetaObject *GameLibraryModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GameLibraryModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN16GameLibraryModelE.stringdata0))
        return static_cast<void*>(this);
    return QAbstractListModel::qt_metacast(_clname);
}

int GameLibraryModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractListModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void GameLibraryModel::syncingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void GameLibraryModel::syncFailed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void GameLibraryModel::countChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void GameLibraryModel::heroGamesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void GameLibraryModel::categoryRowsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void GameLibraryModel::storeDetailsReady(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void GameLibraryModel::protonDbRatingReady(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void GameLibraryModel::trailerCached(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}
QT_WARNING_POP
