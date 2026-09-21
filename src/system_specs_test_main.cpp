// Headless test for SystemSpecs -- scans this machine's hardware and runs a
// handful of sample Steam-style requirement lines through the matcher, so
// the scan + matching logic can be verified independent of the QML/GUI
// layer. Run directly: ./system_specs_test
#include <QCoreApplication>
#include <QTextStream>

#include "system_specs.h"

static void printVerdict(const SystemSpecs &specs, const QString &label, const QString &value)
{
    const QString verdict = specs.matchLevel(label, value);
    QTextStream out(stdout);
    out << "  " << label << ": " << value
        << "  ->  " << (verdict.isEmpty() ? QStringLiteral("(no verdict)") : verdict) << "\n";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    SystemSpecs specs;

    QTextStream out(stdout);
    out << "Detected hardware:\n";
    out << "  CPU: " << (specs.cpuModel().isEmpty() ? QStringLiteral("(not detected)") : specs.cpuModel())
        << " (" << specs.cpuCores() << " logical cores, "
        << (specs.cpuGhz() > 0.0 ? QString::number(specs.cpuGhz(), 'f', 2) + " GHz max" : QStringLiteral("clock speed not detected"))
        << ")\n";
    out << "  RAM: " << QString::number(specs.ramGiB(), 'f', 1) << " GiB\n";
    out << "  GPU: " << (specs.gpuModel().isEmpty() ? QStringLiteral("(not detected)") : specs.gpuModel())
        << (specs.gpuVramGiB() > 0.0 ? QStringLiteral(" (%1 GiB VRAM)").arg(QString::number(specs.gpuVramGiB(), 'f', 1)) : QString())
        << "\n";
    out << "\n";

    out << "Sample requirement matches:\n";
    printVerdict(specs, "Memory", "8 GB RAM");
    printVerdict(specs, "Memory", "32 GB RAM");
    printVerdict(specs, "Processor", "Intel Core i5 750 or higher");
    printVerdict(specs, "Processor", "4 hardware CPU threads - Intel Core i5 750 or higher");
    printVerdict(specs, "Processor", "2 GHz Processor or better");
    printVerdict(specs, "Processor", "2.5 GHz Processor or better");
    printVerdict(specs, "Graphics", "NVIDIA GeForce RTX 3060 or better");
    printVerdict(specs, "Graphics", "1 GB or more, DirectX 11-compatible");
    printVerdict(specs, "Graphics", "1GB VRAM");
    printVerdict(specs, "Graphics", "128 MB Direct3D compatible video card");
    printVerdict(specs, "Graphics", "AMD HD 6850 ; NVIDIA GeForce GTS 450");
    printVerdict(specs, "OS", "Windows 10");

    return 0;
}
