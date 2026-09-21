#include "library_scanner.h"
#include "vdf_parser.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {

// Resolves symlinks (e.g. Bazzite's /home -> /var/home) so the same physical
// library isn't counted twice under two different-looking paths. Falls back
// to the original path if it doesn't exist yet / can't be resolved.
QString canonicalLibraryPath(const QString &path)
{
    const QString canonical = QFileInfo(path).canonicalFilePath();
    return canonical.isEmpty() ? path : canonical;
}

// Common locations for Steam's primary install root on Linux, covering
// native installs, the classic ~/.steam symlink, and Flatpak Steam (which
// some distros, including some Bazzite images, use instead of a native
// package).
QStringList candidateSteamRoots()
{
    const QString home = QDir::homePath();
    return {
        home + "/.local/share/Steam",
        home + "/.steam/steam",
        home + "/.steam/root",
        home + "/.var/app/com.valvesoftware.Steam/data/Steam",
        home + "/.var/app/com.valvesoftware.Steam/.local/share/Steam",
    };
}

} // namespace

LibraryScanner::LibraryScanner(QObject *parent) : QObject(parent) {}

bool LibraryScanner::discoverLibraries()
{
    m_libraryPaths.clear();

    QString steamRoot;
    for (const QString &candidate : candidateSteamRoots()) {
        if (QFile::exists(candidate + "/steamapps/libraryfolders.vdf")) {
            steamRoot = candidate;
            break;
        }
    }

    if (steamRoot.isEmpty())
        return false;

    // The primary Steam root is always a library in its own right.
    m_libraryPaths.append(canonicalLibraryPath(steamRoot));

    QFile file(steamRoot + "/steamapps/libraryfolders.vdf");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString contents = QString::fromUtf8(file.readAll());
        file.close();

        const VdfNode root = parseVdf(contents);
        const VdfNode &folders = root.child("libraryfolders");
        for (const VdfNode &entry : folders.children) {
            if (!entry.isBlock)
                continue;
            const QString path = entry.child("path").value;
            if (path.isEmpty())
                continue;
            const QString canonical = canonicalLibraryPath(path);
            if (!m_libraryPaths.contains(canonical))
                m_libraryPaths.append(canonical);
        }
    }

    return true;
}

QList<InstalledApp> LibraryScanner::scanInstalledApps() const
{
    QList<InstalledApp> apps;

    for (const QString &libraryPath : m_libraryPaths) {
        QDir steamappsDir(libraryPath + "/steamapps");
        if (!steamappsDir.exists())
            continue;

        const QStringList manifests = steamappsDir.entryList({"appmanifest_*.acf"}, QDir::Files);
        for (const QString &manifestName : manifests) {
            QFile file(steamappsDir.filePath(manifestName));
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;

            const QString contents = QString::fromUtf8(file.readAll());
            file.close();

            const VdfNode root = parseVdf(contents);
            const VdfNode &state = root.child("AppState");
            if (!state.isBlock)
                continue;

            InstalledApp app;
            app.appId = state.child("appid").value.toLongLong();
            app.name = state.child("name").value;
            app.installDir = state.child("installdir").value;
            app.sizeOnDiskBytes = state.child("SizeOnDisk").value.toLongLong();
            app.libraryPath = libraryPath;
            apps.append(app);
        }
    }

    return apps;
}

void LibraryScanner::startWatching()
{
    for (const QString &libraryPath : m_libraryPaths) {
        const QString steamappsDir = libraryPath + "/steamapps";
        if (QDir(steamappsDir).exists())
            m_watcher.addPath(steamappsDir);
    }

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &LibraryScanner::installChanged);
}
