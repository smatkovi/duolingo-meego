#include "json.h"

#include <QStringList>

namespace
{

class Reader
{
public:
    Reader(const QByteArray &text) : m_text(text), m_at(0) {}

    QVariant read()
    {
        skipSpace();
        const QVariant value = readValue();
        if (!m_error.isEmpty())
            return QVariant();
        skipSpace();
        if (m_at != m_text.size()) {
            fail("trailing characters");
            return QVariant();
        }
        return value;
    }

    QString error() const { return m_error; }

private:
    void fail(const char *what)
    {
        if (m_error.isEmpty())
            m_error = QString("%1 at byte %2").arg(QString::fromLatin1(what)).arg(m_at);
    }

    void skipSpace()
    {
        while (m_at < m_text.size()) {
            const char c = m_text.at(m_at);
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                ++m_at;
            else
                break;
        }
    }

    bool literal(const char *word)
    {
        const int length = int(qstrlen(word));
        if (m_text.mid(m_at, length) != QByteArray(word))
            return false;
        m_at += length;
        return true;
    }

    QVariant readValue()
    {
        if (m_at >= m_text.size()) {
            fail("unexpected end");
            return QVariant();
        }
        switch (m_text.at(m_at)) {
        case '{': return readObject();
        case '[': return readArray();
        case '"': return readString();
        case 't': if (literal("true")) return QVariant(true); break;
        case 'f': if (literal("false")) return QVariant(false); break;
        case 'n': if (literal("null")) return QVariant(); break;
        default: return readNumber();
        }
        fail("unknown value");
        return QVariant();
    }

    QVariant readObject()
    {
        QVariantMap map;
        ++m_at;                                  // '{'
        skipSpace();
        if (m_at < m_text.size() && m_text.at(m_at) == '}') {
            ++m_at;
            return map;
        }
        forever {
            skipSpace();
            if (m_at >= m_text.size() || m_text.at(m_at) != '"') {
                fail("expected a key");
                return QVariant();
            }
            const QVariant key = readString();
            if (!m_error.isEmpty())
                return QVariant();
            skipSpace();
            if (m_at >= m_text.size() || m_text.at(m_at) != ':') {
                fail("expected ':'");
                return QVariant();
            }
            ++m_at;
            skipSpace();
            const QVariant value = readValue();
            if (!m_error.isEmpty())
                return QVariant();
            map.insert(key.toString(), value);
            skipSpace();
            if (m_at >= m_text.size()) {
                fail("unterminated object");
                return QVariant();
            }
            if (m_text.at(m_at) == ',') {
                ++m_at;
                continue;
            }
            if (m_text.at(m_at) == '}') {
                ++m_at;
                return map;
            }
            fail("expected ',' or '}'");
            return QVariant();
        }
    }

    QVariant readArray()
    {
        QVariantList list;
        ++m_at;                                  // '['
        skipSpace();
        if (m_at < m_text.size() && m_text.at(m_at) == ']') {
            ++m_at;
            return list;
        }
        forever {
            skipSpace();
            const QVariant value = readValue();
            if (!m_error.isEmpty())
                return QVariant();
            list.append(value);
            skipSpace();
            if (m_at >= m_text.size()) {
                fail("unterminated array");
                return QVariant();
            }
            if (m_text.at(m_at) == ',') {
                ++m_at;
                continue;
            }
            if (m_text.at(m_at) == ']') {
                ++m_at;
                return list;
            }
            fail("expected ',' or ']'");
            return QVariant();
        }
    }

    QVariant readString()
    {
        ++m_at;                                  // '"'
        QString out;
        while (m_at < m_text.size()) {
            const char c = m_text.at(m_at);
            if (c == '"') {
                ++m_at;
                // The server sends UTF-8; the escapes above are already
                // decoded into out, so only the raw run needs converting --
                // which is why the run is collected as bytes, not chars.
                return out;
            }
            if (c == '\\') {
                ++m_at;
                if (m_at >= m_text.size())
                    break;
                const char escape = m_text.at(m_at++);
                switch (escape) {
                case '"':  out += QLatin1Char('"'); break;
                case '\\': out += QLatin1Char('\\'); break;
                case '/':  out += QLatin1Char('/'); break;
                case 'b':  out += QLatin1Char('\b'); break;
                case 'f':  out += QLatin1Char('\f'); break;
                case 'n':  out += QLatin1Char('\n'); break;
                case 'r':  out += QLatin1Char('\r'); break;
                case 't':  out += QLatin1Char('\t'); break;
                case 'u': {
                    bool ok = false;
                    const ushort code = m_text.mid(m_at, 4).toUShort(&ok, 16);
                    if (!ok) {
                        fail("bad \\u escape");
                        return QVariant();
                    }
                    m_at += 4;
                    out += QChar(code);
                    break;
                }
                default:
                    fail("bad escape");
                    return QVariant();
                }
                continue;
            }
            // Collect the plain run in one go so UTF-8 sequences survive.
            int start = m_at;
            while (m_at < m_text.size() && m_text.at(m_at) != '"'
                   && m_text.at(m_at) != '\\')
                ++m_at;
            out += QString::fromUtf8(m_text.constData() + start, m_at - start);
        }
        fail("unterminated string");
        return QVariant();
    }

    QVariant readNumber()
    {
        const int start = m_at;
        if (m_at < m_text.size() && (m_text.at(m_at) == '-' || m_text.at(m_at) == '+'))
            ++m_at;
        bool fraction = false;
        while (m_at < m_text.size()) {
            const char c = m_text.at(m_at);
            if (c >= '0' && c <= '9') {
                ++m_at;
            } else if (c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') {
                fraction = true;
                ++m_at;
            } else {
                break;
            }
        }
        const QByteArray text = m_text.mid(start, m_at - start);
        if (text.isEmpty()) {
            fail("expected a number");
            return QVariant();
        }
        bool ok = false;
        if (!fraction) {
            // Identifiers come back as numbers and must survive as integers:
            // a ride id printed through a double gains a ".0" and the server
            // rejects it.
            const qlonglong whole = text.toLongLong(&ok);
            if (ok)
                return whole;
        }
        const double number = text.toDouble(&ok);
        if (!ok) {
            fail("bad number");
            return QVariant();
        }
        return number;
    }

    const QByteArray m_text;
    int m_at;
    QString m_error;
};

void writeString(QByteArray &out, const QString &text)
{
    out += '"';
    const QByteArray utf8 = text.toUtf8();
    for (int i = 0; i < utf8.size(); ++i) {
        const char c = utf8.at(i);
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (uchar(c) < 0x20)
                out += QString("\\u%1").arg(int(uchar(c)), 4, 16, QLatin1Char('0')).toLatin1();
            else
                out += c;
        }
    }
    out += '"';
}

void writeValue(QByteArray &out, const QVariant &value)
{
    if (!value.isValid() || value.isNull()) {
        out += "null";
        return;
    }
    switch (value.type()) {
    case QVariant::Bool:
        out += value.toBool() ? "true" : "false";
        return;
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
        out += QByteArray::number(value.toLongLong());
        return;
    case QVariant::Double: {
        const double number = value.toDouble();
        // 'g' with 15 digits round-trips a double without printing the
        // 0.30000000000000004 that 17 digits would.
        out += QByteArray::number(number, 'g', 15);
        return;
    }
    case QVariant::List: {
        const QVariantList list = value.toList();
        out += '[';
        for (int i = 0; i < list.size(); ++i) {
            if (i)
                out += ',';
            writeValue(out, list.at(i));
        }
        out += ']';
        return;
    }
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        out += '{';
        bool first = true;
        for (QVariantMap::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
            if (!first)
                out += ',';
            first = false;
            writeString(out, it.key());
            out += ':';
            writeValue(out, it.value());
        }
        out += '}';
        return;
    }
    default:
        writeString(out, value.toString());
        return;
    }
}

}

QVariant Json::parse(const QByteArray &text, QString *error)
{
    Reader reader(text);
    const QVariant value = reader.read();
    if (error)
        *error = reader.error();
    return value;
}

QByteArray Json::serialise(const QVariant &value)
{
    QByteArray out;
    writeValue(out, value);
    return out;
}

QVariant Json::value(const QVariant &root, const QString &path)
{
    QVariant current = root;
    const QStringList steps = path.split(QLatin1Char('/'), QString::SkipEmptyParts);
    for (int i = 0; i < steps.size(); ++i) {
        const QString step = steps.at(i);
        bool isIndex = false;
        const int index = step.toInt(&isIndex);
        if (isIndex && current.type() == QVariant::List) {
            const QVariantList list = current.toList();
            if (index < 0 || index >= list.size())
                return QVariant();
            current = list.at(index);
            continue;
        }
        if (current.type() != QVariant::Map)
            return QVariant();
        const QVariantMap map = current.toMap();
        if (!map.contains(step))
            return QVariant();
        current = map.value(step);
    }
    return current;
}
