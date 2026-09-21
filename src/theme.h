#pragma once

#include <QObject>
#include <QColor>

// Style tokens, exposed to QML as a singleton -- registered from C++ in
// main.cpp via qmlRegisterSingletonInstance rather than as a QML
// pragma-Singleton file. The QML-file singleton mechanism proved unreliable
// for objects referenced from recycled list delegates; C++-registered
// singletons go through Qt's core meta-object system instead and don't have
// that failure mode.
class Theme : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QColor background READ background CONSTANT)
    Q_PROPERTY(QColor surface READ surface CONSTANT)
    Q_PROPERTY(QColor surfaceHighlight READ surfaceHighlight CONSTANT)
    Q_PROPERTY(QColor accent READ accent CONSTANT)
    Q_PROPERTY(QColor accentGlow READ accentGlow CONSTANT)
    Q_PROPERTY(QColor textPrimary READ textPrimary CONSTANT)
    Q_PROPERTY(QColor textSecondary READ textSecondary CONSTANT)

    Q_PROPERTY(int radiusSmall READ radiusSmall CONSTANT)
    Q_PROPERTY(int radiusMedium READ radiusMedium CONSTANT)
    Q_PROPERTY(int radiusPill READ radiusPill CONSTANT)

    Q_PROPERTY(int spacingSmall READ spacingSmall CONSTANT)
    Q_PROPERTY(int spacingMedium READ spacingMedium CONSTANT)
    Q_PROPERTY(int spacingLarge READ spacingLarge CONSTANT)

    Q_PROPERTY(int fontSizeSmall READ fontSizeSmall CONSTANT)
    Q_PROPERTY(int fontSizeMedium READ fontSizeMedium CONSTANT)
    Q_PROPERTY(int fontSizeLarge READ fontSizeLarge CONSTANT)

    Q_PROPERTY(int glowRingWidth READ glowRingWidth CONSTANT)

public:
    explicit Theme(QObject *parent = nullptr) : QObject(parent) {}

    QColor background() const { return QColor("#0e0e12"); }
    QColor surface() const { return QColor("#1a1a20"); }
    QColor surfaceHighlight() const { return QColor("#242430"); }
    QColor accent() const { return QColor("#e8823c"); }
    QColor accentGlow() const { return QColor(232, 130, 60, 102); } // translucent orange, for the glow-ring
    QColor textPrimary() const { return QColor("#f2f2f0"); }
    QColor textSecondary() const { return QColor("#9a9a96"); }

    int radiusSmall() const { return 8; }
    int radiusMedium() const { return 14; }
    int radiusPill() const { return 999; }

    int spacingSmall() const { return 8; }
    int spacingMedium() const { return 16; }
    int spacingLarge() const { return 32; }

    int fontSizeSmall() const { return 13; }
    int fontSizeMedium() const { return 18; }
    int fontSizeLarge() const { return 28; }

    int glowRingWidth() const { return 3; }
};
