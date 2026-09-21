#include "qr_image_provider.h"

#include <QPainter>
#include <QUrl>

#ifdef PROJECT_DEX_HAS_QRENCODE
#include <qrencode.h>
#endif

QrImageProvider::QrImageProvider() : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage QrImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
#ifdef PROJECT_DEX_HAS_QRENCODE
    const QString text = QUrl::fromPercentEncoding(id.toUtf8());
    QRcode *qr = QRcode_encodeString(text.toUtf8().constData(), 0, QR_ECLEVEL_M, QR_MODE_8, 1);
    if (!qr) {
        if (size)
            *size = QSize();
        return QImage();
    }

    const int modules = qr->width;
    const int scale = 8;  // px per QR module, before any requestedSize rescale below
    const int quietZone = 4; // modules of white border -- part of the QR spec, scanners expect it
    const int imageSize = (modules + quietZone * 2) * scale;

    QImage image(imageSize, imageSize, QImage::Format_RGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    for (int y = 0; y < modules; ++y) {
        for (int x = 0; x < modules; ++x) {
            // Low bit of each byte is the actual module value (dark/light);
            // the rest encode libqrencode's internal reasoning for why --
            // see qrencode.h's QRcode_encodeString documentation.
            const unsigned char module = qr->data[y * modules + x];
            if (module & 1)
                painter.drawRect((x + quietZone) * scale, (y + quietZone) * scale, scale, scale);
        }
    }
    painter.end();
    QRcode_free(qr);

    if (size)
        *size = image.size();
    return requestedSize.isValid() ? image.scaled(requestedSize) : image;
#else
    Q_UNUSED(id);
    Q_UNUSED(requestedSize);
    if (size)
        *size = QSize();
    return QImage();
#endif
}
