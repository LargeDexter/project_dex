#pragma once

#include <QObject>

class UiState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 expandedAppId READ expandedAppId NOTIFY expandedAppIdChanged)

public:
    explicit UiState(QObject *parent = nullptr) : QObject(parent) {}

    qint64 expandedAppId() const { return m_expandedAppId; }

public slots:
    void expandGame(qint64 appId) { setExpandedAppId(appId); }
    void collapse() { setExpandedAppId(-1); }

signals:
    void expandedAppIdChanged();

private:
    void setExpandedAppId(qint64 id)
    {
        if (m_expandedAppId != id) {
            m_expandedAppId = id;
            emit expandedAppIdChanged();
        }
    }

    qint64 m_expandedAppId = -1;
};
