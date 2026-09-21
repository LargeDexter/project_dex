#include <QCoreApplication>
#include <QTextStream>

#include "proton_db_client.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    qint64 appId = 22380; // Fallout: New Vegas -- known to have plenty of ProtonDB reports
    if (argc > 1)
        appId = QString(argv[1]).toLongLong();

    out << "Fetching ProtonDB rating for appid " << appId << "...\n";
    out.flush();

    ProtonDbClient client;
    QString error;
    const QVariantMap rating = client.fetchRating(appId, error);

    if (!error.isEmpty())
        out << "Error: " << error << "\n";

    if (!rating.value("available").toBool()) {
        out << "No ProtonDB reports available for this app.\n";
        return 0;
    }

    out << "Tier: " << rating.value("tier").toString() << "\n";
    out << "Confidence: " << rating.value("confidence").toString() << "\n";
    out << "Report count: " << rating.value("reportCount").toInt() << "\n";

    return 0;
}
