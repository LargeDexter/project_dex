#include <QCoreApplication>
#include <QTextStream>

#include "store_details_client.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    qint64 appId = 730; // Counter-Strike 2, default
    if (argc > 1)
        appId = QString(argv[1]).toLongLong();

    out << "Fetching store details for appid " << appId << "...\n";
    out.flush();

    StoreDetailsClient client;
    QString error;
    const QVariantMap details = client.fetchDetails(appId, error);

    if (!error.isEmpty())
        out << "Error: " << error << "\n";

    out << "Available: " << (details.value("available").toBool() ? "yes" : "no") << "\n";

    if (details.value("available").toBool()) {
        out << "\nDescription:\n" << details.value("shortDescription").toString() << "\n";
        out << "\nMinimum requirements (raw HTML):\n"
            << details.value("minimumRequirements").toString() << "\n";
        const QString trailerUrl = details.value("trailerUrl").toString();
        out << "\nTrailer URL: " << (trailerUrl.isEmpty() ? QStringLiteral("(none found)") : trailerUrl) << "\n";
    }

    return 0;
}
