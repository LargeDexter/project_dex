#include <QCoreApplication>
#include <QList>
#include <QPair>
#include <QProcessEnvironment>
#include <QTextStream>

#include "artwork_client.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString apiKey = env.value("STEAMGRIDDB_API_KEY");

    if (apiKey.isEmpty()) {
        out << "No STEAMGRIDDB_API_KEY set -- every game below will fall through "
               "to the Steam CDN fallback. That's a valid test too (proves the "
               "fallback path works), but set the key if you want to see "
               "SteamGridDB actually hit.\n\n";
    }

    // A handful of real appids -- swap for your own or pass appids as args,
    // e.g. ./artwork_test 2379780 553850
    QList<QPair<qint64, QString>> sample = {
        {2379780, "Balatro"},
        {553850, "HELLDIVERS 2"},
        {883710, "Resident Evil 2"},
    };

    if (argc > 1) {
        sample.clear();
        for (int i = 1; i < argc; ++i)
            sample.append({QString(argv[i]).toLongLong(), QString("appid %1").arg(argv[i])});
    }

    ArtworkClient client(apiKey);

    for (const auto &entry : sample) {
        const qint64 appId = entry.first;
        const QString name = entry.second;

        out << "Fetching box art for " << name << " (" << appId << ")...\n";
        out.flush();

        QString error;
        bool usedFallback = false;
        const QString localPath = client.fetchAndCacheBoxArt(appId, name, usedFallback, error);

        if (localPath.isEmpty()) {
            out << "  Failed: " << error << "\n";
        } else {
            out << "  Source: " << (usedFallback ? "Steam CDN fallback" : "SteamGridDB") << "\n";
            out << "  Saved to: " << localPath << "\n";
        }
        out << "\n";
    }

    return 0;
}
