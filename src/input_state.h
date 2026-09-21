#pragma once

#include <QObject>
#include <QString>

// Tracks whether the user has provided any input yet (gates the selection
// glow so it doesn't appear automatically on launch), and which input
// method was last used. Currently distinguishes keyboard vs mouse; a real
// controller/gamepad handler can call reportControllerInput() later without
// anything else here needing to change.
class InputState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasInteracted READ hasInteracted NOTIFY hasInteractedChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)
    // True right after keyboard/controller input -- while true, hover-driven
    // selection should be ignored, since tiles can shift underneath a
    // stationary cursor as keyboard navigation scrolls the view. Cleared
    // only by genuine mouse movement (see reportMouseMoved()), not by a
    // tile merely becoming hovered as a side effect of scrolling.
    Q_PROPERTY(bool mouseSuppressed READ mouseSuppressed NOTIFY mouseSuppressedChanged)

public:
    explicit InputState(QObject *parent = nullptr) : QObject(parent) {}

    bool hasInteracted() const { return m_hasInteracted; }
    QString mode() const { return m_mode; }
    bool mouseSuppressed() const { return m_mouseSuppressed; }

public slots:
    void reportKeyboardInput()
    {
        setMode("keyboard");
        setMouseSuppressed(true);
    }

    void reportMouseInput() { setMode("mouse"); }

    // Call only on an actual detected pointer movement (position delta),
    // never on a hover-enter alone -- that's what distinguishes "the user
    // moved the mouse" from "the view scrolled a different tile under it".
    void reportMouseMoved()
    {
        setMode("mouse");
        setMouseSuppressed(false);
    }

    void reportControllerInput() { setMode("controller"); } // not wired to real hardware yet

signals:
    void hasInteractedChanged();
    void modeChanged();
    void mouseSuppressedChanged();

private:
    void setMode(const QString &newMode)
    {
        if (!m_hasInteracted) {
            m_hasInteracted = true;
            emit hasInteractedChanged();
        }
        if (m_mode != newMode) {
            m_mode = newMode;
            emit modeChanged();
        }
    }

    void setMouseSuppressed(bool v)
    {
        if (m_mouseSuppressed != v) {
            m_mouseSuppressed = v;
            emit mouseSuppressedChanged();
        }
    }

    bool m_hasInteracted = false;
    bool m_mouseSuppressed = false;
    QString m_mode = "none";
};
