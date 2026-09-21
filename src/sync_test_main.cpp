#include <QCoreApplication>
#include <QDebug>
#include <QProcessEnvironment>
#include <QTextStream>

#include "steam_client.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString apiKey = env.value("STEAM_API_KEY");
    const QString steamId = env.value("STEAM_ID");

    if (apiKey.isEmpty() || steamId.isEmpty()) {
        out << "Missing STEAM_API_KEY and/or STEAM_ID environment variables.\n";
        out << "Set them before running, e.g.:\n";
        out << "  export STEAM_API_KEY=your_api_key_here\n";
        out << "  export STEAM_ID=your_steamid64_here\n";
        return 1;
    }

    out << "Fetching owned games for SteamID64 " << steamId << "...\n";

    SteamClient client(apiKey, steamId);
    QList<OwnedGame> games;
    QString errorMessage;

    if (!client.fetchOwnedGames(games, errorMessage)) {
        out << "Failed: " << errorMessage << "\n";
        return 1;
    }

    out << "Found " << games.size() << " owned games:\n\n";
    for (const OwnedGame &g : games) {
        const double hours = g.playtimeForeverMinutes / 60.0;
        out << QString("  [%1]  %2  --  %3 hrs\n")
                   .arg(g.appId, 8)
                   .arg(g.name, -40)
                   .arg(hours, 0, 'f', 1);
    }

    return 0;
}
