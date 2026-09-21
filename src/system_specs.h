#pragma once

#include <QJsonArray>
#include <QObject>
#include <QString>
#include <QStringList>

// Scans this machine's CPU/RAM/GPU once at startup and compares Steam's
// freeform system-requirement text against a small bundled benchmark
// table (data/hardware_benchmarks.json) to produce a rough green/yellow/red
// verdict per requirement line.
//
// This is intentionally NOT pulling live data from a benchmark site at
// runtime -- PassMark (the usual source for this kind of comparison) blocks
// automated scraping and its API is a paid product, and TechPowerUp's GPU
// database has similar restrictions. Instead the scores below are a small,
// hand-curated, offline reference table of relative performance tiers,
// meant to be periodically refreshed as new hardware ships rather than
// fetched per-request. Treat the verdicts as an estimate, not a guarantee --
// see matchLevel()'s doc comment for exactly what is and isn't compared.
class SystemSpecs : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString cpuModel READ cpuModel CONSTANT)
    Q_PROPERTY(int cpuCores READ cpuCores CONSTANT)
    Q_PROPERTY(double cpuGhz READ cpuGhz CONSTANT)
    Q_PROPERTY(double ramGiB READ ramGiB CONSTANT)
    Q_PROPERTY(QString gpuModel READ gpuModel CONSTANT)
    Q_PROPERTY(double gpuVramGiB READ gpuVramGiB CONSTANT)

public:
    explicit SystemSpecs(QObject *parent = nullptr);

    QString cpuModel() const { return m_cpuModel; }
    int cpuCores() const { return m_cpuCores; }
    double cpuGhz() const { return m_cpuGhz; }
    double ramGiB() const { return m_ramGiB; }
    QString gpuModel() const { return m_gpuModel; }
    double gpuVramGiB() const { return m_gpuVramGiB; }

    // Returns "check", "warn", "x", or "" (no verdict -- unable to compare)
    // for a single parsed requirement line, e.g. label "Memory", value
    // "8 GB RAM". Only Memory, Processor, and Graphics lines are ever
    // scored; OS/DirectX/Sound/Storage/Additional Notes always return ""
    // since they're not meaningful compatibility signals for a Linux/Proton
    // setup (or, for Storage, not really a "compatibility" risk at all).
    Q_INVOKABLE QString matchLevel(const QString &label, const QString &value) const;

private:
    void scanHardware();
    void loadBenchmarkData();

    QString matchMemory(const QString &value) const;
    QString matchProcessor(const QString &value) const;
    QString matchGraphics(const QString &value) const;

    // The two independent axes matchProcessor() combines: a required clock
    // speed ("2.5 GHz Processor or better") and a required core/thread
    // count ("4 hardware CPU threads"), each checked against this machine
    // directly rather than via the fuzzy model-name table. Either can
    // return "" when the requirement text doesn't state that axis.
    QString matchProcessorGhz(const QString &value) const;
    QString matchProcessorCores(const QString &value) const;

    // Likewise for Graphics: a lot of Steam listings only give a VRAM
    // figure ("1GB VRAM") with no card name at all, or name a card too old
    // or obscure for the bundled table -- so VRAM is checked directly
    // first, with the fuzzy model-name lookup as a second, independent
    // signal when a name is actually given.
    QString matchGraphicsVram(const QString &value) const;
    QString matchGraphicsModel(const QString &value) const;

    int lookupScore(const QString &text, const QJsonArray &table) const;

    static double extractGiB(const QString &value);
    static double extractGhz(const QString &value);
    static int extractCoreOrThreadCount(const QString &value);
    static QStringList significantTokens(const QString &text);

    QString m_cpuModel;
    int m_cpuCores = 0;
    double m_cpuGhz = 0.0;
    double m_ramGiB = 0.0;
    QString m_gpuModel;
    double m_gpuVramGiB = 0.0;

    QJsonArray m_cpuBenchmarks;
    QJsonArray m_gpuBenchmarks;

    // This machine's own score in each table, looked up once at startup
    // since the hardware doesn't change for the life of the process.
    int m_localCpuScore = -1;
    int m_localGpuScore = -1;
};
