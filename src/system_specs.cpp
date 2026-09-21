#include "system_specs.h"

#include <algorithm>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>

SystemSpecs::SystemSpecs(QObject *parent)
    : QObject(parent)
{
    scanHardware();
    loadBenchmarkData();

    m_localCpuScore = lookupScore(m_cpuModel, m_cpuBenchmarks);
    m_localGpuScore = lookupScore(m_gpuModel, m_gpuBenchmarks);
}

void SystemSpecs::scanHardware()
{
    // --- CPU: /proc/cpuinfo has one "model name" line per logical core ---
    //
    // /proc entries are generated on read, not real files, so they report a
    // size of 0 via stat(). QFile's normal *buffered* read path uses that
    // reported size as a hint and can come back completely empty for these
    // pseudo-files. Opening Unbuffered forces plain read() syscalls instead
    // and sidesteps that -- this is a well-known Qt/procfs gotcha, not
    // specific to this file.
    QFile cpuinfo("/proc/cpuinfo");
    double fallbackMaxMHz = 0.0;
    if (cpuinfo.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
        const QString contents = QString::fromLocal8Bit(cpuinfo.readAll());
        int logicalCores = 0;
        for (const QString &line : contents.split('\n')) {
            if (line.startsWith("model name")) {
                logicalCores++;
                if (m_cpuModel.isEmpty()) {
                    const int colon = line.indexOf(':');
                    if (colon >= 0)
                        m_cpuModel = line.mid(colon + 1).trimmed();
                }
            } else if (line.startsWith("cpu MHz")) {
                const int colon = line.indexOf(':');
                if (colon >= 0)
                    fallbackMaxMHz = std::max(fallbackMaxMHz, line.mid(colon + 1).trimmed().toDouble());
            }
        }
        m_cpuCores = logicalCores;
    }

    // Prefer the kernel's reported max rated frequency over /proc/cpuinfo's
    // "cpu MHz", which reflects the CURRENT scaled frequency and fluctuates
    // with power-saving states -- not what a requirement like "2.5 GHz or
    // better" means. cpuinfo_max_freq is in kHz.
    QFile maxFreqFile("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    if (maxFreqFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
        bool ok = false;
        const double kHz = QString::fromLocal8Bit(maxFreqFile.readAll()).trimmed().toDouble(&ok);
        if (ok && kHz > 0.0)
            m_cpuGhz = kHz / 1000000.0;
    }
    if (m_cpuGhz <= 0.0 && fallbackMaxMHz > 0.0)
        m_cpuGhz = fallbackMaxMHz / 1000.0;

    // --- RAM: /proc/meminfo's MemTotal is in kB -- same Unbuffered fix. ---
    QFile meminfo("/proc/meminfo");
    if (meminfo.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
        const QString contents = QString::fromLocal8Bit(meminfo.readAll());
        for (const QString &line : contents.split('\n')) {
            if (line.startsWith("MemTotal:")) {
                static const QRegularExpression re("(\\d+)");
                const auto match = re.match(line);
                if (match.hasMatch())
                    m_ramGiB = match.captured(1).toDouble() / (1024.0 * 1024.0);
                break;
            }
        }
    }

    // --- GPU VRAM: AMD's amdgpu driver exposes this directly in sysfs, in
    // bytes. This runs regardless of whether lspci is installed, since it
    // doesn't depend on it. NVIDIA has no equivalent sysfs file, so fall
    // back to nvidia-smi when present. Intel iGPUs share system RAM and
    // don't report a meaningful dedicated figure -- left at 0, which just
    // means VRAM-based graphics matching falls back to the model-name
    // lookup for those. ---
    for (int i = 0; i < 4 && m_gpuVramGiB <= 0.0; ++i) {
        QFile vramFile(QStringLiteral("/sys/class/drm/card%1/device/mem_info_vram_total").arg(i));
        if (!vramFile.exists() || !vramFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
            continue;
        bool ok = false;
        const double bytes = QString::fromLocal8Bit(vramFile.readAll()).trimmed().toDouble(&ok);
        if (ok && bytes > 0.0)
            m_gpuVramGiB = bytes / (1024.0 * 1024.0 * 1024.0);
    }

    if (m_gpuVramGiB <= 0.0 && !QStandardPaths::findExecutable("nvidia-smi").isEmpty()) {
        QProcess nvidiaSmi;
        nvidiaSmi.start("nvidia-smi", {"--query-gpu=memory.total", "--format=csv,noheader,nounits"});
        if (nvidiaSmi.waitForFinished(3000)) {
            const QString firstLine = QString::fromLocal8Bit(nvidiaSmi.readAllStandardOutput())
                                           .trimmed().split('\n').value(0).trimmed();
            bool ok = false;
            const double mib = firstLine.toDouble(&ok);
            if (ok && mib > 0.0)
                m_gpuVramGiB = mib / 1024.0;
        }
    }

    // --- GPU model: parse `lspci` for the display controller line(s).
    // Prefer a discrete NVIDIA/AMD card over an Intel iGPU when both are
    // present. `pciutils` (which provides lspci) isn't always preinstalled
    // in a minimal dev container -- check first so we don't burn the
    // timeout waiting on a process that was never going to start. ---
    if (QStandardPaths::findExecutable("lspci").isEmpty())
        return;

    QProcess lspci;
    lspci.start("lspci", QStringList());
    if (lspci.waitForFinished(3000)) {
        const QString output = QString::fromLocal8Bit(lspci.readAllStandardOutput());
        QString fallback;
        for (const QString &line : output.split('\n')) {
            if (!line.contains("VGA compatible controller") && !line.contains("3D controller"))
                continue;

            const int colon = line.indexOf(':', line.indexOf("controller"));
            if (colon < 0)
                continue;
            const QString model = line.mid(colon + 1).trimmed();

            if (model.contains("Intel", Qt::CaseInsensitive)) {
                if (fallback.isEmpty())
                    fallback = model;
                continue;
            }
            // First non-Intel controller wins outright.
            m_gpuModel = model;
            break;
        }
        if (m_gpuModel.isEmpty())
            m_gpuModel = fallback;
    }
}

void SystemSpecs::loadBenchmarkData()
{
    QFile file(":/data/hardware_benchmarks.json");
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return;

    const QJsonObject root = doc.object();
    m_cpuBenchmarks = root.value("cpu").toArray();
    m_gpuBenchmarks = root.value("gpu").toArray();
}

QString SystemSpecs::matchLevel(const QString &label, const QString &value) const
{
    const QString l = label.toLower();
    if (l.contains("memory") || l.contains("ram"))
        return matchMemory(value);
    if (l.contains("graphics") || l.contains("video"))
        return matchGraphics(value);
    if (l.contains("processor") || l.contains("cpu"))
        return matchProcessor(value);

    // OS, DirectX, Sound, Storage, Additional Notes, etc. -- not compared.
    return QString();
}

QString SystemSpecs::matchMemory(const QString &value) const
{
    const double requiredGiB = extractGiB(value);
    if (requiredGiB <= 0.0 || m_ramGiB <= 0.0)
        return QString();

    if (m_ramGiB >= requiredGiB)
        return QStringLiteral("check");
    if (m_ramGiB >= requiredGiB * 0.75)
        return QStringLiteral("warn");
    return QStringLiteral("x");
}

namespace {
// Severity ranking used to combine two independent verdicts into one:
// "x" (3) beats "warn" (2) beats "check" (1) beats "" / not-applicable (0).
int verdictRank(const QString &v)
{
    if (v == QLatin1String("x"))
        return 3;
    if (v == QLatin1String("warn"))
        return 2;
    if (v == QLatin1String("check"))
        return 1;
    return 0;
}

QString worseVerdict(const QString &a, const QString &b)
{
    return verdictRank(a) >= verdictRank(b) ? a : b;
}
} // namespace

QString SystemSpecs::matchGraphics(const QString &value) const
{
    // VRAM and a named model are independent signals -- a lot of listings
    // give only one or the other (a bare "1GB VRAM" with no card name, or a
    // named card too old/obscure for the bundled table). Combine whichever
    // is available; worst wins if both are.
    const QString vramVerdict = matchGraphicsVram(value);
    const QString modelVerdict = matchGraphicsModel(value);
    return worseVerdict(vramVerdict, modelVerdict);
}

QString SystemSpecs::matchGraphicsVram(const QString &value) const
{
    if (m_gpuVramGiB <= 0.0)
        return QString();

    const double requiredGiB = extractGiB(value);
    if (requiredGiB <= 0.0)
        return QString();

    if (m_gpuVramGiB >= requiredGiB)
        return QStringLiteral("check");
    if (m_gpuVramGiB >= requiredGiB * 0.75)
        return QStringLiteral("warn");
    return QStringLiteral("x");
}

QString SystemSpecs::matchGraphicsModel(const QString &value) const
{
    if (m_localGpuScore < 0)
        return QString();

    const int requiredScore = lookupScore(value, m_gpuBenchmarks);
    if (requiredScore < 0)
        return QString();

    if (m_localGpuScore >= requiredScore)
        return QStringLiteral("check");
    if (m_localGpuScore >= requiredScore * 0.7)
        return QStringLiteral("warn");
    return QStringLiteral("x");
}

QString SystemSpecs::matchProcessor(const QString &value) const
{
    // A CPU model name alone doesn't tell you whether a *different* CPU
    // beats it -- "2 GHz Processor or better" and "4 threads" are numbers
    // we can check directly against this machine, which is a more reliable
    // signal than fuzzy-matching a model string. Combine whichever of the
    // two axes the requirement actually states; the worse of the two wins,
    // so a fast-but-2-core CPU doesn't get a false "check" against a
    // 4-core requirement.
    const QString ghzVerdict = matchProcessorGhz(value);
    const QString coreVerdict = matchProcessorCores(value);
    const QString combined = worseVerdict(ghzVerdict, coreVerdict);
    if (!combined.isEmpty())
        return combined;

    // Neither a GHz figure nor an explicit core/thread count was stated --
    // fall back to a fuzzy match against a named CPU model (e.g. "Intel
    // Core i5 750 or higher").
    const int requiredScore = lookupScore(value, m_cpuBenchmarks);
    if (requiredScore >= 0 && m_localCpuScore >= 0) {
        if (m_localCpuScore >= requiredScore)
            return QStringLiteral("check");
        if (m_localCpuScore >= requiredScore * 0.7)
            return QStringLiteral("warn");
        return QStringLiteral("x");
    }

    return QString();
}

QString SystemSpecs::matchProcessorGhz(const QString &value) const
{
    if (m_cpuGhz <= 0.0)
        return QString();

    const double requiredGhz = extractGhz(value);
    if (requiredGhz <= 0.0)
        return QString();

    if (m_cpuGhz >= requiredGhz)
        return QStringLiteral("check");
    if (m_cpuGhz >= requiredGhz * 0.85)
        return QStringLiteral("warn");
    return QStringLiteral("x");
}

QString SystemSpecs::matchProcessorCores(const QString &value) const
{
    const int requiredThreads = extractCoreOrThreadCount(value);
    if (requiredThreads <= 0 || m_cpuCores <= 0)
        return QString();

    if (m_cpuCores >= requiredThreads)
        return QStringLiteral("check");
    if (m_cpuCores >= requiredThreads - 1)
        return QStringLiteral("warn");
    return QStringLiteral("x");
}

double SystemSpecs::extractGiB(const QString &value)
{
    static const QRegularExpression re("(\\d+(?:\\.\\d+)?)\\s*(GB|MB)",
                                        QRegularExpression::CaseInsensitiveOption);
    const auto match = re.match(value);
    if (!match.hasMatch())
        return -1.0;

    const double amount = match.captured(1).toDouble();
    const QString unit = match.captured(2).toUpper();
    return unit == "MB" ? amount / 1024.0 : amount;
}

double SystemSpecs::extractGhz(const QString &value)
{
    static const QRegularExpression re("(\\d+(?:\\.\\d+)?)\\s*GHz",
                                        QRegularExpression::CaseInsensitiveOption);
    const auto match = re.match(value);
    return match.hasMatch() ? match.captured(1).toDouble() : -1.0;
}

int SystemSpecs::extractCoreOrThreadCount(const QString &value)
{
    static const QRegularExpression re(
        "(\\d+)\\s*(?:hardware\\s*)?(?:cpu\\s*)?(?:threads?|cores?)",
        QRegularExpression::CaseInsensitiveOption);
    const auto match = re.match(value);
    return match.hasMatch() ? match.captured(1).toInt() : -1;
}

QStringList SystemSpecs::significantTokens(const QString &text)
{
    static const QSet<QString> stopWords = {
        "the", "or", "and", "with", "series", "graphics", "processor",
        "corporation", "controller", "compatible", "vga", "3d", "family",
        "generation", "higher", "above", "equivalent"
    };

    QString cleaned = text.toLower();
    cleaned.replace(QRegularExpression("[^a-z0-9]+"), " ");

    QStringList tokens;
    const auto parts = cleaned.split(' ', Qt::SkipEmptyParts);
    for (const QString &tok : parts) {
        if (tok.size() >= 2 && !stopWords.contains(tok))
            tokens << tok;
    }
    return tokens;
}

int SystemSpecs::lookupScore(const QString &text, const QJsonArray &table) const
{
    if (text.isEmpty())
        return -1;

    const QString haystack = text.toLower();
    int bestScore = -1;
    int bestMatchedTokens = 0;

    for (const QJsonValue &v : table) {
        const QJsonObject entry = v.toObject();
        const QString name = entry.value("name").toString();
        const int score = entry.value("score").toInt();
        const QStringList tokens = significantTokens(name);
        if (tokens.isEmpty())
            continue;

        int matched = 0;
        bool matchedModelToken = false;
        static const QRegularExpression hasDigit("\\d");
        for (const QString &token : tokens) {
            if (haystack.contains(token)) {
                matched++;
                if (hasDigit.match(token).hasMatch())
                    matchedModelToken = true;
            }
        }

        // Require the distinguishing model-number token to hit (so "Radeon"
        // alone can't match every AMD card), plus at least half of the
        // entry's other tokens.
        if (matchedModelToken && matched * 2 >= tokens.size() && matched > bestMatchedTokens) {
            bestMatchedTokens = matched;
            bestScore = score;
        }
    }

    return bestScore;
}
