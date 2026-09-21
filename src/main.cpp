#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>

#include "game_library_model.h"
#include "theme.h"
#include "input_state.h"
#include "ui_state.h"
#include "system_specs.h"
#include "gamepad_input.h"
#include "settings_manager.h"
#include "qr_image_provider.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    GameLibraryModel gameLibrary;

    // Registered from C++ rather than as a QML pragma-Singleton file -- see
    // theme.h for why.
    static Theme theme;
    qmlRegisterSingletonInstance("ProjectDex", 1, 0, "Theme", &theme);

    static InputState inputState;
    qmlRegisterSingletonInstance("ProjectDex", 1, 0, "InputState", &inputState);

    static UiState uiState;
    qmlRegisterSingletonInstance("ProjectDex", 1, 0, "UiState", &uiState);

    static SystemSpecs systemSpecs;
    qmlRegisterSingletonInstance("ProjectDex", 1, 0, "SystemSpecs", &systemSpecs);

    // Controller navigation (D-pad/stick/A/B) -- implemented by sending
    // the same synthetic key events the keyboard already drives, aimed at
    // whatever item currently has active focus. See gamepad_input.h for
    // why that means no QML had to change to support it. Gracefully
    // becomes a no-op if SDL2 wasn't available at build time.
    static GamepadInput gamepadInput;
    qmlRegisterSingletonInstance("ProjectDex", 1, 0, "GamepadInput", &gamepadInput);

    // Steam API key / SteamID / SteamGridDB key, entered via the setup
    // wizard (SetupWizardScreen.qml) and persisted to settings.json -- see
    // app_settings.h. LibrarySyncWorker reads the same file directly
    // instead of going through this singleton (it lives on a background
    // thread), so this object exists purely for the wizard's own UI.
    static SettingsManager settingsManager;
    qmlRegisterSingletonInstance("ProjectDex", 1, 0, "SettingsManager", &settingsManager);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("gameLibrary", &gameLibrary);

    // Lets QML request a QR code as an ordinary image:
    // Image { source: "image://qr/" + encodeURIComponent(someUrl) }.
    // Ownership passes to the engine. See qr_image_provider.h for the
    // graceful-degrade behavior when built without libqrencode.
    engine.addImageProvider(QStringLiteral("qr"), new QrImageProvider);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("ProjectDex", "Main");

    // Hand the gamepad poller the actual root window so it knows where to
    // send synthetic key events (i.e. wherever active focus is *within
    // this window*, not just anywhere in the process).
    if (!engine.rootObjects().isEmpty()) {
        if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first()))
            gamepadInput.setWindow(window);
    }

    gameLibrary.refresh(); // auto-sync on launch, per the plan

    return app.exec();
}
