#pragma once

#include <QString>
#include <QMap>

// A node in a parsed VDF (Valve KeyValues) tree.
// Leaf nodes carry a string value; block nodes carry named children.
// Both libraryfolders.vdf and appmanifest_<id>.acf use this same format.
struct VdfNode {
    QString value;
    QMap<QString, VdfNode> children;
    bool isBlock = false;

    bool hasChild(const QString &key) const { return children.contains(key); }

    const VdfNode &child(const QString &key) const
    {
        static const VdfNode empty;
        auto it = children.find(key);
        return it != children.end() ? it.value() : empty;
    }
};

// Parses VDF text into a tree. On malformed input, returns as much as could
// be parsed rather than failing outright -- Steam's own files are not always
// perfectly well-formed.
VdfNode parseVdf(const QString &text);
