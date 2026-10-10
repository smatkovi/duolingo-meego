#ifndef JSON_H
#define JSON_H

#include <QByteArray>
#include <QVariant>

// A JSON reader and writer that works the same on both editions.
//
// Qt 4.7 on Harmattan has no QJsonDocument -- it arrived with Qt 5 -- and the
// Sailfish edition should not parse the same server answers through a second,
// differently behaved implementation. So both use this one; it is small
// enough that reading it is cheaper than reasoning about two parsers.
//
// Values map to QVariant the obvious way: object -> QVariantMap,
// array -> QVariantList, string -> QString, number -> double (or qlonglong
// where the text has no fraction and fits), true/false -> bool,
// null -> an invalid QVariant.
namespace Json
{

// Returns an invalid QVariant on malformed input; pass an error pointer to
// find out where it gave up.
QVariant parse(const QByteArray &text, QString *error = 0);

QByteArray serialise(const QVariant &value);

// Convenience for the usual "walk into the answer" case:
// value(answer, "data/person/firstname").
QVariant value(const QVariant &root, const QString &path);

}

#endif
