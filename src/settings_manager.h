#pragma once

#include <QObject>
#include <QString>

// QML-facing singleton wrapping AppSettings (see app_settings.h) -- the
// setup wizard (SetupWizardScreen.qml) reads/writes credentials through
// this, and it also owns the validation rules for each field so the wizard
// can flag an obviously-wrong key/id before the user ever hits Save (a
// couch-typed-on-a-virtual-keyboard field is exactly the kind of input
// that's easy to fat-finger and annoying to have to redo a whole sync
// cycle to discover).
//
// Deliberately NOT what LibrarySyncWorker reads from directly -- that
// worker lives on a background QThread, and touching this object's
// properties from another thread without going through Qt's
// signal/slot marshalling would be undefined behavior. It calls
// AppSettings::load() itself instead (plain file I/O, safe from any
// thread) -- see library_sync_worker.cpp.
class SettingsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString steamApiKey READ steamApiKey NOTIFY steamApiKeyChanged)
    Q_PROPERTY(QString steamId READ steamId NOTIFY steamIdChanged)
    Q_PROPERTY(QString steamGridDbKey READ steamGridDbKey NOTIFY steamGridDbKeyChanged)
    // True once both required fields (API key + SteamID) are non-empty.
    // NOT the same question as "will syncing actually work right now" --
    // this app also honors STEAM_API_KEY/STEAM_ID env vars as a fallback
    // (see library_sync_worker.cpp), so someone using those instead of the
    // wizard can have a fully working sync with hasCredentials still
    // false here. That's fine: this property only drives whether the
    // fields *this class knows about* look filled in, not the wizard's
    // auto-open behavior -- that's driven off GameLibraryModel's actual
    // syncFailed signal instead, which reflects the real fallback chain.
    Q_PROPERTY(bool hasCredentials READ hasCredentials NOTIFY hasCredentialsChanged)
    // Whether image://qr/... will actually render anything -- false if the
    // app was built without libqrencode (see CMakeLists.txt). The wizard
    // falls back to just showing the instructions URL as plain text when
    // this is false.
    Q_PROPERTY(bool qrCodeAvailable READ qrCodeAvailable CONSTANT)
    // Whether the main window should run fullscreen or windowed -- see
    // Main.qml, which binds its `visibility` to this. Read/write (unlike
    // the credential fields above, which only change via the wizard's
    // save()) since SettingsScreen.qml toggles this directly.
    Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged)

public:
    explicit SettingsManager(QObject *parent = nullptr);

    QString steamApiKey() const { return m_steamApiKey; }
    QString steamId() const { return m_steamId; }
    QString steamGridDbKey() const { return m_steamGridDbKey; }
    bool hasCredentials() const { return !m_steamApiKey.isEmpty() && !m_steamId.isEmpty(); }
    bool qrCodeAvailable() const;
    bool fullscreen() const { return m_fullscreen; }
    void setFullscreen(bool value);

public slots:
    // Persists all three fields at once (the wizard's Save button) and
    // updates this object's own properties to match. Returns an error
    // message on failure (e.g. couldn't write the file), or an empty
    // string on success.
    QString save(const QString &steamApiKey, const QString &steamId, const QString &steamGridDbKey);

    // Field-level validation, called live as the wizard's on-screen
    // keyboard edits each field -- returns a short human-readable problem
    // description, or an empty string if the value looks fine. Doesn't
    // call the actual Steam/SteamGridDB APIs (that only happens on a real
    // sync) -- this is just shape-checking to catch obvious typos (wrong
    // length, stray characters) before they cost a whole failed sync to
    // discover.
    QString validateSteamApiKey(const QString &key) const;
    QString validateSteamId(const QString &steamId) const;
    QString validateSteamGridDbKey(const QString &key) const; // empty is always valid -- this field is optional

signals:
    void steamApiKeyChanged();
    void steamIdChanged();
    void steamGridDbKeyChanged();
    void hasCredentialsChanged();
    void fullscreenChanged();

private:
    QString m_steamApiKey;
    QString m_steamId;
    QString m_steamGridDbKey;
    bool m_fullscreen = true;
};
