#include <QCoreApplication>
#include <QTextStream>

#include "library_scanner.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    LibraryScanner scanner;
    if (!scanner.discoverLibraries()) {
        out << "Could not find a Steam installation. Checked ~/.local/share/Steam, "
               "~/.steam/steam, ~/.steam/root, and the Flatpak Steam data paths.\n"
               "Is Steam installed on this machine?\n";
        return 1;
    }

    out << "Found " << scanner.libraryPaths().size() << " Steam library folder(s):\n";
    for (const QString &path : scanner.libraryPaths())
        out << "  " << path << "\n";
    out << "\n";

    auto printApps = [&out](const QList<InstalledApp> &apps) {
        out << "Installed apps (" << apps.size() << "):\n";
        for (const InstalledApp &a : apps) {
            const double gb = a.sizeOnDiskBytes / (1024.0 * 1024.0 * 1024.0);
            out << QString("  [%1]  %2  --  %3 GB  (%4)\n")
                       .arg(a.appId, 8)
                       .arg(a.name, -40)
                       .arg(gb, 0, 'f', 2)
                       .arg(a.libraryPath);
        }
        out.flush();
    };

    printApps(scanner.scanInstalledApps());

    out << "\nWatching for install changes -- try installing or uninstalling something "
           "from Steam now. Ctrl+C to quit.\n";
    out.flush();

    QObject::connect(&scanner, &LibraryScanner::installChanged, [&]() {
        out << "\n-- install state changed --\n";
        printApps(scanner.scanInstalledApps());
    });
    scanner.startWatching();

    return app.exec();
}
