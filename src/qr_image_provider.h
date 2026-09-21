#pragma once

#include <QQuickImageProvider>

// Renders a QR code as a QImage on demand, so QML can just do
// Image { source: "image://qr/" + encodeURIComponent(someUrl) }. Registered
// in main.cpp via engine.addImageProvider("qr", ...).
//
// Backed by libqrencode -- same graceful-degrade pattern as SDL2/Multimedia
// in CMakeLists.txt: if the library isn't available at build time, this
// still exists and gets registered, but requestImage() just returns a null
// QImage (see SettingsManager::qrCodeAvailable(), which the setup wizard
// checks to fall back to showing the URL as plain text instead).
class QrImageProvider : public QQuickImageProvider
{
public:
    QrImageProvider();

    // `id` is the text to encode, percent-encoded by the QML caller (plain
    // text would otherwise get parsed as part of the image:// URL itself --
    // see the class comment above for the expected call shape).
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};
