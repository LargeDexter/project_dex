#pragma once

#include <QElapsedTimer>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>

class QQuickWindow;

// Polls a connected game controller (Xbox/PlayStation/Switch Pro/Steam
// Controller/Deck, or anything SDL2's GameControllerDB recognizes) and
// turns input into the *exact same* synthetic key events the keyboard
// already drives: Qt.Key_Up/Down/Left/Right for navigation, Qt.Key_Return
// to confirm/select, Qt.Key_Escape to back out, Qt.Key_PageUp/PageDown to
// page a row.
//
// Every row (GameRow.qml), the hero banner (Hero.qml), and the expanded
// game view (ExpandedGameView.qml) already handle those keys -- that's
// deliberate, from when keyboard navigation was built out. So this class
// does NOT introduce a second, parallel navigation path in QML: a
// controller press just becomes the same keypress a keyboard would send,
// aimed at whatever item currently holds active focus.
//
// Which physical button/axis fires which of those key events is
// reassignable at runtime -- see the "remapping" section below -- because
// not every controller's SDL mapping lines up with its physical labels
// the same way (a real example: one pad's "shoulder button" SDL id turned
// out to correspond to its trigger). A calibration wizard in QML drives
// this by calling startCalibration() for one logical action at a time.
//
// Gracefully does nothing if SDL2 wasn't available at build time (see
// CMakeLists.txt) -- keyboard and mouse are unaffected either way.
class GamepadInput : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool calibrating READ calibrating NOTIFY calibratingChanged)
    Q_PROPERTY(QString calibratingAction READ calibratingAction NOTIFY calibratingActionChanged)

public:
    explicit GamepadInput(QObject *parent = nullptr);
    ~GamepadInput() override;

    bool connected() const { return m_connected; }
    bool calibrating() const { return m_calibrating; }
    QString calibratingAction() const { return m_calibratingAction; }

    // Set once, right after the QML engine has loaded Main.qml and the
    // root QQuickWindow exists. Synthetic key events are sent to whatever
    // item currently holds active focus within this window.
    void setWindow(QQuickWindow *window) { m_window = window; }

    // ---- Remapping (driven by a QML calibration wizard) ----

    // The fixed, ordered list of logical actions that can be remapped
    // (e.g. "up", "confirm", "pageForward"). Same order every time, so a
    // wizard can walk through them step by step.
    Q_INVOKABLE QStringList calibrationActions() const;

    // Human-readable name for an action id, e.g. "up" -> "Move Up".
    Q_INVOKABLE QString actionLabel(const QString &action) const;

    // Human-readable description of whatever is currently bound to an
    // action, e.g. "Left Shoulder (LB)", or "D-Pad / Left Stick (default)"
    // if it's never been remapped.
    Q_INVOKABLE QString currentBindingLabel(const QString &action) const;

    // Enters "listening" mode for one action: the very next new
    // button-press or stick/trigger movement the controller reports (not
    // counting whatever was already held down when this was called) gets
    // captured as that action's new binding. Emits calibrationCaptured()
    // once that happens. No-op if no controller is connected.
    Q_INVOKABLE void startCalibration(const QString &action);

    // Leaves listening mode without capturing anything.
    Q_INVOKABLE void cancelCalibration();

    Q_INVOKABLE void resetBindingToDefault(const QString &action);
    Q_INVOKABLE void resetAllBindingsToDefault();

    // A binding is either one of two built-in combo defaults (so out of
    // the box, both the D-pad and the stick move the highlight, and both
    // the shoulder button and the trigger page a row -- exactly like
    // before remapping existed), or a single specific physical control
    // once the player has calibrated that action themselves. Public so a
    // free helper function in gamepad_input.cpp can serialize it, even
    // though nothing outside this class otherwise needs to touch it.
    struct Binding
    {
        // RawButton reads a physical button by its raw SDL_Joystick index
        // instead of through SDL's GameController semantic mapping
        // (A/B/X/Y/shoulders/etc). GameController only has slots for 11
        // named buttons + the d-pad; a generic/no-name controller can have
        // more physical buttons than that, and the extras end up with no
        // semantic name at all -- invisible to this app no matter how
        // correct the mapping string is. Raw indices sidestep that ceiling
        // entirely: every physical button is reachable regardless of what,
        // if anything, SDL's mapping calls it. All calibration-captured
        // bindings use this kind; Button is kept only for the hardcoded
        // confirm/back defaults, which are fine on a normally-behaved pad.
        enum Kind { DirectionDefault, PageDefault, Button, AxisPositive, AxisNegative, RawButton };
        Kind kind = DirectionDefault;
        int id = 0; // SDL_GameControllerButton/Axis value, or a raw SDL_Joystick button index for RawButton; unused for the two Default kinds.
    };

signals:
    void connectedChanged();
    void calibratingChanged();
    void calibratingActionChanged();
    // Fired right after a calibration capture lands -- `label` is the same
    // string currentBindingLabel() would now return for `action`.
    void calibrationCaptured(const QString &action, const QString &label);

private:
    void poll();
    void loadCommunityMappings();
    void refreshConnection();
    void sendKey(int qtKey, bool pressed);
    void sendTap(int qtKey);
    void updateDirection(bool held, bool &wasHeld, qint64 &nextRepeatAt, int qtKey);
    void dispatchEdgeAction(const QString &action, int qtKey);
    bool isActionActive(void *controller, const QString &action) const;
    bool isBindingPhysicallyActive(void *controller, const Binding &binding) const;
    void handleCalibrationPoll(void *controller);
    void captureBinding(const Binding &binding);
    Binding defaultBinding(const QString &action) const;
    QString bindingLabel(const Binding &binding) const;
    QString buttonName(int id) const;
    QString axisName(int id) const;
    QString mappingFilePath() const;
    void loadMapping();
    void saveMapping();

    QQuickWindow *m_window = nullptr;
    void *m_controller = nullptr; // SDL_GameController*, opaque here so this header never needs SDL's.
    bool m_connected = false;
    bool m_loggedMissingWindow = false;
    QTimer m_pollTimer;
    QElapsedTimer m_clock;

    QMap<QString, Binding> m_mapping; // only holds entries the player has explicitly remapped.
    QMap<QString, bool> m_actionEdgeState; // last-frame held state, per edge-detected action.

    bool m_calibrating = false;
    QString m_calibratingAction;
    QVector<bool> m_calibBaselineButtons;
    QVector<int> m_calibBaselineAxes;
    QVector<bool> m_calibBaselineRawButtons; // baseline for the raw-index scan; see Binding::RawButton.

    // Right after a capture, the physical control that was just bound is
    // very likely still being held down (the player's finger hasn't left
    // the button yet) -- if normal dispatch resumed immediately, that same
    // still-held press would instantly fire the action it was just bound
    // to. Usually harmless, but genuinely broken for Confirm/Back: binding
    // Back to a button would make the wizard close itself the instant you
    // finished calibrating it. So normal dispatch stays paused until this
    // specific control is released at least once.
    bool m_waitingForCaptureRelease = false;
    Binding m_pendingReleaseBinding;

    // Repeat-while-held state per direction, so holding the stick/D-pad
    // browses a row the same way holding an arrow key on a real keyboard
    // does (one immediate step, a pause, then steady repeats).
    bool m_upHeld = false;
    bool m_downHeld = false;
    bool m_leftHeld = false;
    bool m_rightHeld = false;
    qint64 m_upNextRepeat = 0;
    qint64 m_downNextRepeat = 0;
    qint64 m_leftNextRepeat = 0;
    qint64 m_rightNextRepeat = 0;
};
