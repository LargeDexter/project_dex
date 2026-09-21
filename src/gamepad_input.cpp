#include "gamepad_input.h"

#ifdef PROJECT_DEX_HAS_SDL2

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QQuickWindow>
#include <QStandardPaths>

#include <SDL2/SDL.h>

namespace {
// How long a direction has to be held before it starts auto-repeating,
// and how fast it repeats after that -- tuned to feel like the same
// left/right keyboard-repeat browsing within a row already has.
constexpr qint64 kInitialRepeatDelayMs = 350;
constexpr qint64 kRepeatIntervalMs = 120;

// Left stick has to move this far off center (out of a max of 32767)
// before it counts as a direction -- avoids drift/noise on cheap sticks
// registering as constant input. Tune up if a pad feels twitchy, down if
// it feels unresponsive.
constexpr int kStickDeadzone = 12000;

// Trigger axes run 0 (released) to 32767 (fully pulled), unlike the
// sticks which are centered at 0 -- same idea, different range.
constexpr int kTriggerThreshold = 16000;

constexpr int kAxisCount = 6;
constexpr SDL_GameControllerAxis kCalibrationAxes[kAxisCount] = {
    SDL_CONTROLLER_AXIS_LEFTX,      SDL_CONTROLLER_AXIS_LEFTY,      SDL_CONTROLLER_AXIS_RIGHTX,
    SDL_CONTROLLER_AXIS_RIGHTY,     SDL_CONTROLLER_AXIS_TRIGGERLEFT, SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
};

int thresholdForAxis(int axisId)
{
    return (axisId == SDL_CONTROLLER_AXIS_TRIGGERLEFT || axisId == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
        ? kTriggerThreshold
        : kStickDeadzone;
}

SDL_GameController *asController(void *ptr)
{
    return static_cast<SDL_GameController *>(ptr);
}

QString kindToString(GamepadInput::Binding::Kind kind)
{
    switch (kind) {
    case GamepadInput::Binding::Button: return QStringLiteral("button");
    case GamepadInput::Binding::AxisPositive: return QStringLiteral("axisPositive");
    case GamepadInput::Binding::AxisNegative: return QStringLiteral("axisNegative");
    case GamepadInput::Binding::RawButton: return QStringLiteral("rawButton");
    default: return QString(); // Default kinds are never persisted.
    }
}
}

GamepadInput::GamepadInput(QObject *parent)
    : QObject(parent)
{
    // HIDAPI backend generally does a much better job than the plain
    // Linux joystick API at correctly identifying third-party pads
    // (proper button counts, correct rumble support, etc.) -- worth
    // enabling unconditionally, it's a no-op for controllers that don't
    // use it. Has to be set before SDL_InitSubSystem.
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "1");

    // SDL_INIT_GAMECONTROLLER pulls in SDL_INIT_JOYSTICK automatically. If
    // this fails (no SDL2 runtime available, or something about the
    // environment doesn't support it), controller support just quietly
    // stays off rather than taking the app down -- keyboard/mouse still
    // work regardless.
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
        qWarning() << "GamepadInput: SDL_Init failed, controller support disabled:" << SDL_GetError();
        return;
    }

    loadCommunityMappings();

    m_clock.start();
    loadMapping();
    refreshConnection();

    connect(&m_pollTimer, &QTimer::timeout, this, &GamepadInput::poll);
    m_pollTimer.start(16); // ~60Hz, matches typical display refresh.
}

GamepadInput::~GamepadInput()
{
    if (m_controller)
        SDL_GameControllerClose(asController(m_controller));
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

// The system's own SDL2 package (Fedora/Bazzite included) ships whatever
// controller-mapping database existed when it was last packaged, which is
// frequently stale or missing entries entirely for less mainstream pads --
// this is what caused genuinely broken button/axis mappings for a couple of
// controllers tested against this app. SDL_GameControllerAddMappingsFromRW()
// loads *additional* mapping entries on top of whatever's already
// registered, and a later entry for the same GUID takes priority over an
// earlier one -- so loading this bundled, actively-maintained database
// right after SDL_Init effectively overrides the system's stale entries
// with current ones, while still falling back to the system's own mapping
// for anything this database doesn't happen to cover. Refresh
// data/gamecontrollerdb.txt periodically from
// https://github.com/mdqinc/SDL_GameControllerDB.
void GamepadInput::loadCommunityMappings()
{
    QFile file(QStringLiteral(":/data/gamecontrollerdb.txt"));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "GamepadInput: couldn't open bundled gamecontrollerdb.txt";
        return;
    }

    const QByteArray data = file.readAll();
    SDL_RWops *rw = SDL_RWFromConstMem(data.constData(), data.size());
    if (!rw) {
        qWarning() << "GamepadInput: SDL_RWFromConstMem failed for bundled controller database";
        return;
    }

    // freesrc = 1: SDL closes/frees the RWops itself once it's done reading.
    const int added = SDL_GameControllerAddMappingsFromRW(rw, 1);
    if (added < 0)
        qWarning() << "GamepadInput: failed to load bundled controller mappings:" << SDL_GetError();
    else
        qInfo() << "GamepadInput: loaded" << added << "controller mappings from bundled database";
}

void GamepadInput::refreshConnection()
{
    if (m_controller && SDL_GameControllerGetAttached(asController(m_controller)))
        return; // Already have one, and it's still plugged in.

    if (m_controller) {
        SDL_GameControllerClose(asController(m_controller));
        m_controller = nullptr;
    }

    // Grabs the first recognized controller. Good enough for a single
    // living-room PC with one pad at a time -- multi-controller support
    // (local co-op / player switching) is a natural future addition, not
    // needed for basic navigation.
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            SDL_GameController *controller = SDL_GameControllerOpen(i);
            if (controller) {
                m_controller = controller;
                break;
            }
        }
    }

    const bool nowConnected = m_controller != nullptr;
    if (nowConnected != m_connected) {
        m_connected = nowConnected;
        emit connectedChanged();
        if (m_connected) {
            qInfo() << "GamepadInput: controller connected -" << SDL_GameControllerName(asController(m_controller));
            // Logged once per connect, not per frame -- cheap, and it's
            // genuinely useful for diagnosing a specific pad's quirks later
            // (some devices' community SDL mappings alias inputs together
            // in surprising ways, e.g. a "trigger" turning out to share a
            // raw axis with a stick).
            char *mapping = SDL_GameControllerMapping(asController(m_controller));
            qInfo() << "GamepadInput: SDL mapping string -" << (mapping ? mapping : "(none)");
            if (mapping)
                SDL_free(mapping);
        } else {
            qInfo() << "GamepadInput: controller disconnected";
        }
    }
}

void GamepadInput::poll()
{
    // Pumps SDL's event queue -- needed for it to notice hot-plug/unplug
    // so a controller plugged in after launch gets picked up (and one
    // unplugged mid-session gets noticed) without restarting the app.
    SDL_Event event;
    bool deviceListChanged = false;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_CONTROLLERDEVICEADDED || event.type == SDL_CONTROLLERDEVICEREMOVED)
            deviceListChanged = true;
    }
    if (deviceListChanged || !m_controller)
        refreshConnection();

    if (!m_controller)
        return;

    SDL_GameController *controller = asController(m_controller);
    SDL_GameControllerUpdate();

    if (m_calibrating) {
        handleCalibrationPoll(controller);
        return;
    }

    if (m_waitingForCaptureRelease) {
        if (isBindingPhysicallyActive(controller, m_pendingReleaseBinding))
            return; // Still holding whatever was just calibrated -- don't act on it yet.
        m_waitingForCaptureRelease = false;
    }

    if (!m_window) {
        // Would otherwise fail completely silently -- this is the one
        // condition that would make a *connected, working* controller
        // produce zero visible effect.
        if (!m_loggedMissingWindow) {
            qWarning() << "GamepadInput: controller present but no window has been set -- input has nowhere to go";
            m_loggedMissingWindow = true;
        }
        return;
    }

    // "up"/"down"/"left"/"right" (stick, by default/convention) and
    // "dpadUp"/"dpadDown"/"dpadLeft"/"dpadRight" (D-pad) are independent,
    // separately-calibratable bindings for the same four directions -- see
    // the comment on defaultBinding() -- so both get checked and either
    // one firing moves the highlight, exactly like the single combined
    // DirectionDefault binding used to.
    updateDirection(isActionActive(controller, QStringLiteral("up")) || isActionActive(controller, QStringLiteral("dpadUp")),
                     m_upHeld, m_upNextRepeat, Qt::Key_Up);
    updateDirection(isActionActive(controller, QStringLiteral("down")) || isActionActive(controller, QStringLiteral("dpadDown")),
                     m_downHeld, m_downNextRepeat, Qt::Key_Down);
    updateDirection(isActionActive(controller, QStringLiteral("left")) || isActionActive(controller, QStringLiteral("dpadLeft")),
                     m_leftHeld, m_leftNextRepeat, Qt::Key_Left);
    updateDirection(isActionActive(controller, QStringLiteral("right")) || isActionActive(controller, QStringLiteral("dpadRight")),
                     m_rightHeld, m_rightNextRepeat, Qt::Key_Right);

    // Confirm/back/paging are taps, not held-repeat -- edge detected so a
    // press sends exactly one tap, not one per poll tick for as long as
    // it's held.
    dispatchEdgeAction(QStringLiteral("confirm"), Qt::Key_Return);
    dispatchEdgeAction(QStringLiteral("back"), Qt::Key_Escape);
    dispatchEdgeAction(QStringLiteral("pageForward"), Qt::Key_PageDown);
    dispatchEdgeAction(QStringLiteral("pageBackward"), Qt::Key_PageUp);
}

void GamepadInput::dispatchEdgeAction(const QString &action, int qtKey)
{
    SDL_GameController *controller = asController(m_controller);
    const bool active = isActionActive(controller, action);
    const bool wasActive = m_actionEdgeState.value(action, false);
    if (active && !wasActive)
        sendTap(qtKey);
    m_actionEdgeState[action] = active;
}

bool GamepadInput::isActionActive(void *controllerPtr, const QString &action) const
{
    SDL_GameController *controller = asController(controllerPtr);
    const Binding binding = m_mapping.value(action, defaultBinding(action));

    switch (binding.kind) {
    case Binding::DirectionDefault: {
        const bool dpadUp = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
        const bool dpadDown = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        const bool dpadLeft = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        const bool dpadRight = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        const Sint16 stickX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
        const Sint16 stickY = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
        if (action == QLatin1String("up")) return dpadUp || stickY < -kStickDeadzone;
        if (action == QLatin1String("down")) return dpadDown || stickY > kStickDeadzone;
        if (action == QLatin1String("left")) return dpadLeft || stickX < -kStickDeadzone;
        if (action == QLatin1String("right")) return dpadRight || stickX > kStickDeadzone;
        return false;
    }
    case Binding::PageDefault: {
        const bool leftTrigger = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > kTriggerThreshold;
        const bool rightTrigger = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > kTriggerThreshold;
        if (action == QLatin1String("pageBackward"))
            return SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) || leftTrigger;
        if (action == QLatin1String("pageForward"))
            return SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) || rightTrigger;
        return false;
    }
    case Binding::Button:
    case Binding::AxisPositive:
    case Binding::AxisNegative:
    case Binding::RawButton:
        return isBindingPhysicallyActive(controllerPtr, binding);
    }
    return false;
}

// Whether the specific physical control a binding refers to is currently
// pressed/deflected, straight from SDL, with no notion of which logical
// action (if any) it's assigned to. Only meaningful for the three
// "concrete" binding kinds (Button/AxisPositive/AxisNegative) -- the two
// Default kinds describe a *combination* of controls tied to a specific
// action's identity (e.g. "D-pad or stick, whichever direction word this
// is"), which doesn't make sense out of that context, so this treats them
// as never active.
bool GamepadInput::isBindingPhysicallyActive(void *controllerPtr, const Binding &binding) const
{
    SDL_GameController *controller = asController(controllerPtr);
    switch (binding.kind) {
    case Binding::Button:
        return SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(binding.id));
    case Binding::AxisPositive:
        return SDL_GameControllerGetAxis(controller, static_cast<SDL_GameControllerAxis>(binding.id)) > thresholdForAxis(binding.id);
    case Binding::AxisNegative:
        return SDL_GameControllerGetAxis(controller, static_cast<SDL_GameControllerAxis>(binding.id)) < -thresholdForAxis(binding.id);
    case Binding::RawButton: {
        SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
        return joystick && SDL_JoystickGetButton(joystick, binding.id);
    }
    default:
        return false;
    }
}

void GamepadInput::updateDirection(bool held, bool &wasHeld, qint64 &nextRepeatAt, int qtKey)
{
    const qint64 now = m_clock.elapsed();
    if (held && !wasHeld) {
        // Just pressed -- fire immediately, then wait the longer initial
        // delay before auto-repeat kicks in (matches how holding an arrow
        // key on a real keyboard behaves).
        sendTap(qtKey);
        nextRepeatAt = now + kInitialRepeatDelayMs;
    } else if (held && now >= nextRepeatAt) {
        sendTap(qtKey);
        nextRepeatAt = now + kRepeatIntervalMs;
    }
    wasHeld = held;
}

void GamepadInput::sendTap(int qtKey)
{
    sendKey(qtKey, true);
    sendKey(qtKey, false);
}

void GamepadInput::sendKey(int qtKey, bool pressed)
{
    if (!m_window)
        return;

    // IMPORTANT: send to the *window*, not directly to activeFocusItem().
    // A real keyboard press arrives at the window, and it's
    // QQuickWindow's own key-event delivery that sends it to the focused
    // item and then -- if unaccepted -- walks it up the parent chain so
    // each ancestor's Keys.onPressed gets a turn (that's what lets
    // GameRow.qml handle Left/Right/Up/Down even when a child GameTile
    // happens to hold literal focus). Sending straight to the focus item
    // with QCoreApplication::sendEvent bypasses all of that bubbling.
    QKeyEvent keyEvent(pressed ? QEvent::KeyPress : QEvent::KeyRelease, qtKey, Qt::NoModifier);
    QCoreApplication::sendEvent(m_window, &keyEvent);
}

// ---- Remapping ----

QStringList GamepadInput::calibrationActions() const
{
    return {
        QStringLiteral("up"),          QStringLiteral("down"),         QStringLiteral("left"),
        QStringLiteral("right"),       QStringLiteral("dpadUp"),       QStringLiteral("dpadDown"),
        QStringLiteral("dpadLeft"),    QStringLiteral("dpadRight"),    QStringLiteral("confirm"),
        QStringLiteral("back"),        QStringLiteral("pageForward"),  QStringLiteral("pageBackward"),
    };
}

QString GamepadInput::actionLabel(const QString &action) const
{
    static const QMap<QString, QString> labels = {
        {QStringLiteral("up"), QStringLiteral("Move Up (Stick)")},
        {QStringLiteral("down"), QStringLiteral("Move Down (Stick)")},
        {QStringLiteral("left"), QStringLiteral("Move Left (Stick)")},
        {QStringLiteral("right"), QStringLiteral("Move Right (Stick)")},
        {QStringLiteral("dpadUp"), QStringLiteral("Move Up (D-Pad)")},
        {QStringLiteral("dpadDown"), QStringLiteral("Move Down (D-Pad)")},
        {QStringLiteral("dpadLeft"), QStringLiteral("Move Left (D-Pad)")},
        {QStringLiteral("dpadRight"), QStringLiteral("Move Right (D-Pad)")},
        {QStringLiteral("confirm"), QStringLiteral("Confirm / Select")},
        {QStringLiteral("back"), QStringLiteral("Back / Cancel")},
        {QStringLiteral("pageForward"), QStringLiteral("Page Forward")},
        {QStringLiteral("pageBackward"), QStringLiteral("Page Backward")},
    };
    return labels.value(action, action);
}

GamepadInput::Binding GamepadInput::defaultBinding(const QString &action) const
{
    // "up"/"down"/"left"/"right" and "dpadUp"/"dpadDown"/"dpadLeft"/
    // "dpadRight" are two independent, separately-calibratable bindings for
    // the same four directions -- isActionActive() for "up" OR's both
    // together (see poll()), so out of the box this is just the old
    // DirectionDefault combo split into two halves that happen to add up
    // to the same thing. The point of splitting it is that each half can
    // now be recalibrated on its own: if a pad's D-pad turns out to be
    // mismapped (the same category of bug the raw-button fix exists for
    // on face buttons), fixing "dpadUp" doesn't touch "up"'s stick
    // binding, and vice versa -- neither one calibrating the other away.
    if (action == QLatin1String("up") || action == QLatin1String("down") || action == QLatin1String("left")
        || action == QLatin1String("right"))
        return Binding{Binding::DirectionDefault, 0};
    if (action == QLatin1String("dpadUp"))
        return Binding{Binding::Button, SDL_CONTROLLER_BUTTON_DPAD_UP};
    if (action == QLatin1String("dpadDown"))
        return Binding{Binding::Button, SDL_CONTROLLER_BUTTON_DPAD_DOWN};
    if (action == QLatin1String("dpadLeft"))
        return Binding{Binding::Button, SDL_CONTROLLER_BUTTON_DPAD_LEFT};
    if (action == QLatin1String("dpadRight"))
        return Binding{Binding::Button, SDL_CONTROLLER_BUTTON_DPAD_RIGHT};
    if (action == QLatin1String("pageForward") || action == QLatin1String("pageBackward"))
        return Binding{Binding::PageDefault, 0};
    if (action == QLatin1String("confirm"))
        return Binding{Binding::Button, SDL_CONTROLLER_BUTTON_A};
    if (action == QLatin1String("back"))
        return Binding{Binding::Button, SDL_CONTROLLER_BUTTON_B};
    return Binding{};
}

QString GamepadInput::currentBindingLabel(const QString &action) const
{
    return bindingLabel(m_mapping.value(action, defaultBinding(action)));
}

QString GamepadInput::bindingLabel(const Binding &binding) const
{
    switch (binding.kind) {
    case Binding::DirectionDefault: return QStringLiteral("D-Pad / Left Stick (default)");
    case Binding::PageDefault: return QStringLiteral("Shoulder Button / Trigger (default)");
    case Binding::Button: return buttonName(binding.id);
    case Binding::AxisPositive: return axisName(binding.id) + QStringLiteral(" (+)");
    case Binding::AxisNegative: return axisName(binding.id) + QStringLiteral(" (-)");
    case Binding::RawButton: return QStringLiteral("Physical Button %1").arg(binding.id);
    }
    return QStringLiteral("Unbound");
}

QString GamepadInput::buttonName(int id) const
{
    switch (id) {
    case SDL_CONTROLLER_BUTTON_A: return QStringLiteral("A Button");
    case SDL_CONTROLLER_BUTTON_B: return QStringLiteral("B Button");
    case SDL_CONTROLLER_BUTTON_X: return QStringLiteral("X Button");
    case SDL_CONTROLLER_BUTTON_Y: return QStringLiteral("Y Button");
    case SDL_CONTROLLER_BUTTON_BACK: return QStringLiteral("Back / Select");
    case SDL_CONTROLLER_BUTTON_GUIDE: return QStringLiteral("Guide");
    case SDL_CONTROLLER_BUTTON_START: return QStringLiteral("Start");
    case SDL_CONTROLLER_BUTTON_LEFTSTICK: return QStringLiteral("Left Stick Click");
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return QStringLiteral("Right Stick Click");
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return QStringLiteral("Left Shoulder (LB)");
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return QStringLiteral("Right Shoulder (RB)");
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return QStringLiteral("D-Pad Up");
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return QStringLiteral("D-Pad Down");
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return QStringLiteral("D-Pad Left");
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return QStringLiteral("D-Pad Right");
    default: {
        const char *raw = SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(id));
        return raw ? QString::fromUtf8(raw) : QStringLiteral("Button %1").arg(id);
    }
    }
}

QString GamepadInput::axisName(int id) const
{
    switch (id) {
    case SDL_CONTROLLER_AXIS_LEFTX: return QStringLiteral("Left Stick (Horizontal)");
    case SDL_CONTROLLER_AXIS_LEFTY: return QStringLiteral("Left Stick (Vertical)");
    case SDL_CONTROLLER_AXIS_RIGHTX: return QStringLiteral("Right Stick (Horizontal)");
    case SDL_CONTROLLER_AXIS_RIGHTY: return QStringLiteral("Right Stick (Vertical)");
    case SDL_CONTROLLER_AXIS_TRIGGERLEFT: return QStringLiteral("Left Trigger (LT)");
    case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: return QStringLiteral("Right Trigger (RT)");
    default: return QStringLiteral("Axis %1").arg(id);
    }
}

void GamepadInput::startCalibration(const QString &action)
{
    if (!m_controller)
        return; // Nothing to calibrate against.

    SDL_GameController *controller = asController(m_controller);
    SDL_GameControllerUpdate();

    // Snapshot whatever's already held/pushed right now, so calibration
    // only captures a *new* press -- otherwise a button the player is
    // still holding from clicking "Start" (unlikely with a mouse, but a
    // controller-driven UI could trigger this) would instantly capture.
    m_calibBaselineButtons.fill(false, SDL_CONTROLLER_BUTTON_MAX);
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
        m_calibBaselineButtons[i] = SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(i));

    m_calibBaselineAxes.fill(0, kAxisCount);
    for (int a = 0; a < kAxisCount; ++a)
        m_calibBaselineAxes[a] = SDL_GameControllerGetAxis(controller, kCalibrationAxes[a]);

    // Raw joystick-index baseline -- see Binding::RawButton. Covers every
    // physical button regardless of whether GameController's mapping gives
    // it a semantic name, which the scan above alone would miss.
    SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
    const int rawButtonCount = joystick ? SDL_JoystickNumButtons(joystick) : 0;
    m_calibBaselineRawButtons.fill(false, rawButtonCount);
    for (int i = 0; i < rawButtonCount; ++i)
        m_calibBaselineRawButtons[i] = SDL_JoystickGetButton(joystick, i);

    m_calibratingAction = action;
    m_calibrating = true;
    emit calibratingChanged();
    emit calibratingActionChanged();
}

void GamepadInput::cancelCalibration()
{
    if (!m_calibrating)
        return;
    m_calibrating = false;
    m_calibratingAction.clear();
    emit calibratingChanged();
    emit calibratingActionChanged();
}

void GamepadInput::handleCalibrationPoll(void *controllerPtr)
{
    SDL_GameController *controller = asController(controllerPtr);

    // Scans raw joystick button indices rather than GameController's
    // semantic A/B/X/Y/etc slots -- see Binding::RawButton. A generic
    // controller can have more physical buttons than GameController has
    // names for, and those extras would never show up in a semantic scan
    // no matter how correct the mapping string is; raw indices see all of
    // them, so this alone replaces what used to be a separate semantic
    // scan.
    SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
    const int rawButtonCount = joystick ? SDL_JoystickNumButtons(joystick) : 0;
    for (int i = 0; i < rawButtonCount; ++i) {
        const bool down = SDL_JoystickGetButton(joystick, i);
        const bool wasDown = i < m_calibBaselineRawButtons.size() && m_calibBaselineRawButtons[i];
        if (down && !wasDown) {
            captureBinding(Binding{Binding::RawButton, i});
            return;
        }
    }

    // The D-pad is reported through SDL's separate "hat" state, not through
    // the raw joystick buttons array the scan above reads -- so a D-pad
    // press is invisible to that scan no matter what. It *does* still have
    // a normal semantic GameController slot (unlike the extra buttons
    // RawButton exists for), so a small dedicated scan of just those four
    // slots -- using the semantic baseline captured in startCalibration()
    // -- catches it. Captured as a regular Binding::Button, which already
    // plays back fine via SDL_GameControllerGetButton().
    static constexpr SDL_GameControllerButton kDpadButtons[] = {
        SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    };
    for (SDL_GameControllerButton dpadButton : kDpadButtons) {
        const bool down = SDL_GameControllerGetButton(controller, dpadButton);
        const bool wasDown = dpadButton < m_calibBaselineButtons.size() && m_calibBaselineButtons[dpadButton];
        if (down && !wasDown) {
            captureBinding(Binding{Binding::Button, static_cast<int>(dpadButton)});
            return;
        }
    }

    for (int a = 0; a < kAxisCount; ++a) {
        const int axisId = kCalibrationAxes[a];
        const int value = SDL_GameControllerGetAxis(controller, kCalibrationAxes[a]);
        const int threshold = thresholdForAxis(axisId);
        const int baseline = a < m_calibBaselineAxes.size() ? m_calibBaselineAxes[a] : 0;
        if (value > threshold && baseline <= threshold) {
            captureBinding(Binding{Binding::AxisPositive, axisId});
            return;
        }
        if (value < -threshold && baseline >= -threshold) {
            captureBinding(Binding{Binding::AxisNegative, axisId});
            return;
        }
    }
}

void GamepadInput::captureBinding(const Binding &binding)
{
    const QString action = m_calibratingAction;
    m_mapping[action] = binding;
    saveMapping();

    // The physical press that was just captured is almost certainly still
    // being held (the finger hasn't lifted yet) -- pause normal dispatch
    // until it's released, so this same press can't immediately fire the
    // action it was just bound to. See poll()'s m_waitingForCaptureRelease
    // check and the comment on the member itself for why this matters.
    m_waitingForCaptureRelease = true;
    m_pendingReleaseBinding = binding;

    m_calibrating = false;
    m_calibratingAction.clear();
    emit calibratingChanged();
    emit calibratingActionChanged();
    emit calibrationCaptured(action, bindingLabel(binding));
}

void GamepadInput::resetBindingToDefault(const QString &action)
{
    if (m_mapping.remove(action) > 0)
        saveMapping();
}

void GamepadInput::resetAllBindingsToDefault()
{
    if (m_mapping.isEmpty())
        return;
    m_mapping.clear();
    saveMapping();
}

QString GamepadInput::mappingFilePath() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return dir + QStringLiteral("/gamepad_mapping.json");
}

void GamepadInput::loadMapping()
{
    QFile file(mappingFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return; // No saved mapping yet -- defaults apply, that's normal.

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return;

    const QJsonObject obj = doc.object();
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        const QJsonObject entry = it.value().toObject();
        const QString kindStr = entry.value(QStringLiteral("kind")).toString();
        Binding binding;
        binding.id = entry.value(QStringLiteral("id")).toInt();
        if (kindStr == QLatin1String("button"))
            binding.kind = Binding::Button;
        else if (kindStr == QLatin1String("axisPositive"))
            binding.kind = Binding::AxisPositive;
        else if (kindStr == QLatin1String("axisNegative"))
            binding.kind = Binding::AxisNegative;
        else if (kindStr == QLatin1String("rawButton"))
            binding.kind = Binding::RawButton;
        else
            continue; // Unrecognized entry -- skip rather than guess.
        m_mapping[it.key()] = binding;
    }
}

void GamepadInput::saveMapping()
{
    QJsonObject obj;
    for (auto it = m_mapping.constBegin(); it != m_mapping.constEnd(); ++it) {
        const QString kindStr = kindToString(it.value().kind);
        if (kindStr.isEmpty())
            continue; // Default kinds are never persisted -- nothing to save for them.
        QJsonObject entry;
        entry.insert(QStringLiteral("kind"), kindStr);
        entry.insert(QStringLiteral("id"), it.value().id);
        obj.insert(it.key(), entry);
    }

    const QString path = mappingFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "GamepadInput: couldn't save controller mapping to" << path << "-" << file.errorString();
        return;
    }
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

#else

// Built without SDL2 (see CMakeLists.txt) -- controller support quietly
// does nothing so the rest of the app (keyboard/mouse) is unaffected.
GamepadInput::GamepadInput(QObject *parent) : QObject(parent) {}
GamepadInput::~GamepadInput() = default;
void GamepadInput::poll() {}
void GamepadInput::loadCommunityMappings() {}
void GamepadInput::refreshConnection() {}
void GamepadInput::sendKey(int, bool) {}
void GamepadInput::sendTap(int) {}
void GamepadInput::updateDirection(bool, bool &wasHeld, qint64 &, int) { wasHeld = false; }
void GamepadInput::dispatchEdgeAction(const QString &, int) {}
bool GamepadInput::isActionActive(void *, const QString &) const { return false; }
bool GamepadInput::isBindingPhysicallyActive(void *, const Binding &) const { return false; }
void GamepadInput::handleCalibrationPoll(void *) {}
void GamepadInput::captureBinding(const Binding &) {}
GamepadInput::Binding GamepadInput::defaultBinding(const QString &) const { return Binding{}; }
QString GamepadInput::bindingLabel(const Binding &) const { return QStringLiteral("No controller support in this build"); }
QString GamepadInput::buttonName(int) const { return QString(); }
QString GamepadInput::axisName(int) const { return QString(); }
QString GamepadInput::mappingFilePath() const { return QString(); }
void GamepadInput::loadMapping() {}
void GamepadInput::saveMapping() {}

QStringList GamepadInput::calibrationActions() const
{
    return {
        QStringLiteral("up"),          QStringLiteral("down"),         QStringLiteral("left"),
        QStringLiteral("right"),       QStringLiteral("confirm"),      QStringLiteral("back"),
        QStringLiteral("pageForward"), QStringLiteral("pageBackward"),
    };
}
QString GamepadInput::actionLabel(const QString &action) const { return action; }
QString GamepadInput::currentBindingLabel(const QString &) const { return QStringLiteral("No controller support in this build"); }
void GamepadInput::startCalibration(const QString &) {}
void GamepadInput::cancelCalibration() {}
void GamepadInput::resetBindingToDefault(const QString &) {}
void GamepadInput::resetAllBindingsToDefault() {}

#endif
