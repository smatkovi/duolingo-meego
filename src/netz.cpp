#include "netz.h"
#include "json.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QSettings>
#include <QTextStream>
#include <QUrl>

#if QT_VERSION >= 0x050000
#include <QBuffer>
#include <QNetworkRequest>
#include <QStandardPaths>
#endif

namespace {

const char *BASIS = "https://www.duolingo.com";

#if QT_VERSION < 0x050000
// Der Rust-Holer macht auf Harmattan das Netz (Qt 4.7 kann kein TLS 1.2).
const char *HOLER = "/opt/duolingo/bin/duo-holer";
#endif

// base64url ohne Polsterung, wie in einer Marke.
QByteArray ausBase64Url(const QString &text)
{
    QString b = text;
    b.replace(QLatin1Char('-'), QLatin1Char('+'));
    b.replace(QLatin1Char('_'), QLatin1Char('/'));
    while (b.size() % 4)
        b.append(QLatin1Char('='));
    return QByteArray::fromBase64(b.toLatin1());
}

// Ein Merkzettel ueber jede Anfrage: Zeit, Weg, Pfad, Zustand. **Ohne
// Koerper, ohne Marke, ohne Passwort** -- er soll sagen, *was* schiefging,
// und nichts verraten. Ohne ihn bleibt eine misslungene Anmeldung am Telefon
// eine Meldung ohne Spur, und man raet beim naechsten Mal wieder.
void merken(const QString &zeile)
{
    const QString ordner = QDir::homePath() + QLatin1String("/.cache/duolingo");
    QDir().mkpath(ordner);
    QFile datei(ordner + QLatin1String("/netz.log"));
    // Bei 40 kB von vorn anfangen; das Telefon hat wenig Platz.
    if (datei.size() > 40000)
        datei.remove();
    if (!datei.open(QIODevice::Append | QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream aus(&datei);
    aus << QDateTime::currentDateTime().toString(Qt::ISODate)
        << QLatin1Char(' ') << zeile << QLatin1Char('\n');
}

}  // namespace

Netz::Netz(QObject *eltern)
    : QObject(eltern)
{
    // **Die Anmeldung haengt am Benutzeragenten, nachgemessen am
    // 10.10.2026 gegen ein Probekonto mit bekanntem Passwort:** mit einem
    // eigenen Namen antwortet `POST /login` auf das richtige Passwort mit
    // 401, mit dem der Android-App mit 200 und Marke. Der Grund steht im
    // Web-Buendel: die Weboberflaeche legt der Anmeldung ein
    // reCAPTCHA-Token bei (Feld `signal`, `c.e("LOGIN")`); wer kein
    // Browser ist, kommt nur ueber den Weg der App hinein.
    //
    // Der Server prueft die Zeile streng: `Duodroid/<zahl.zahl.zahl>`,
    // danach nichts oder `Dalvik/...`. "Duodroid/6.0.0 Qt/4.7.4 (...)" wird
    // abgelehnt, "Duodroid/0.1.0-meego5" auch. Deshalb diese Form -- und
    // darin steht wenigstens die Wahrheit ueber Geraet und System, statt
    // ein Android-Telefon zu erfinden.
    m_benutzeragent = QLatin1String(
        "Duodroid/6.0.0 Dalvik/2.1.0 (Linux; U; MeeGo 1.2 Harmattan; Nokia N9)");

#if QT_VERSION >= 0x050000
    m_netz = new QNetworkAccessManager(this);
    connect(m_netz, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(fertig(QNetworkReply*)));
#endif
    markeLaden();
}

QString Netz::kennungAusMarke(const QString &jwt)
{
    const QStringList teile = jwt.split(QLatin1Char('.'));
    if (teile.size() < 2)
        return QString();
    const QVariant inhalt = Json::parse(ausBase64Url(teile.at(1)));
    const QVariant kennung = inhalt.toMap().value(QLatin1String("sub"));
    if (!kennung.isValid())
        return QString();
    // Die Kennung ist eine grosse Zahl; als Gleitkomma gelesen verliert sie
    // Stellen. Darum ueber qlonglong, und nur im Notfall ueber den Text.
    bool ging = false;
    const qlonglong zahl = kennung.toLongLong(&ging);
    return ging ? QString::number(zahl) : kennung.toString();
}

void Netz::markeLaden()
{
    QSettings s;
    m_marke = s.value(QLatin1String("konto/marke")).toString();
    m_benutzerId = s.value(QLatin1String("konto/kennung")).toString();
    if (!m_marke.isEmpty() && m_benutzerId.isEmpty())
        m_benutzerId = kennungAusMarke(m_marke);
}

void Netz::markeSichern()
{
    QSettings s;
    s.setValue(QLatin1String("konto/marke"), m_marke);
    s.setValue(QLatin1String("konto/kennung"), m_benutzerId);
    s.sync();
}

void Netz::marke(const QString &jwt)
{
    const bool vorher = angemeldet();
    m_marke = jwt;
    m_benutzerId = kennungAusMarke(jwt);
    markeSichern();
    if (vorher != angemeldet() || !jwt.isEmpty())
        emit anmeldungGeaendert();
}

bool Netz::einspielen(const QString &jwt)
{
    QString sauber = jwt.trimmed();
    // Wer den Keks aus dem Browser kopiert, hat leicht `jwt_token=` davor.
    const int gleich = sauber.indexOf(QLatin1Char('='));
    if (gleich > 0 && gleich < 20)
        sauber = sauber.mid(gleich + 1).trimmed();
    if (sauber.endsWith(QLatin1Char(';')))
        sauber.chop(1);
    if (sauber.count(QLatin1Char('.')) != 2 || kennungAusMarke(sauber).isEmpty())
        return false;
    marke(sauber);
    return true;
}

void Netz::abmelden()
{
    if (m_marke.isEmpty())
        return;
    m_marke.clear();
    m_benutzerId.clear();
    markeSichern();
    emit anmeldungGeaendert();
}

void Netz::auswerten(const QString &kennung, int status,
                     const QByteArray &koerper, const QStringList &kopf)
{
    merken(QString::fromLatin1("%1 -> %2, %3 Byte")
               .arg(kennung).arg(status).arg(koerper.size()));
    // Die Anmeldung gibt die Marke im Antwortkopf zurueck, nicht im
    // Koerper. Wer nur den Koerper liest, haelt eine geglueckte Anmeldung
    // fuer ein leeres Ergebnis.
    for (int i = 0; i < kopf.size(); ++i) {
        const QString zeile = kopf.at(i);
        if (zeile.startsWith(QLatin1String("jwt:"), Qt::CaseInsensitive))
            marke(zeile.mid(4).trimmed());
    }

    if (status == 401) {
        emit markeAbgelaufen();
        emit fehler(kennung, status, tr("Die Anmeldung gilt nicht mehr."));
        return;
    }

    QString grund;
    const QVariant gelesen = Json::parse(koerper, &grund);
    if (status < 200 || status >= 300) {
        QString text = gelesen.toMap().value(QLatin1String("details")).toString();
        if (text.isEmpty())
            text = gelesen.toMap().value(QLatin1String("message")).toString();
        if (text.isEmpty())
            text = QString::fromUtf8(koerper.left(200));
        if (status == 0)
            text = kopf.join(QLatin1String("; "));
        emit fehler(kennung, status, text);
        return;
    }

    QVariantMap karte;
    if (gelesen.type() == QVariant::List)
        karte.insert(QLatin1String("liste"), gelesen);
    else
        karte = gelesen.toMap();
    // 202 heisst bei der Abgabe: angenommen, aber noch nicht verrechnet.
    karte.insert(QLatin1String("__status"), status);
    emit antwort(kennung, karte);
}

#if QT_VERSION >= 0x050000

void Netz::ruf(const QString &kennung, const QString &methode,
               const QString &pfad, const QByteArray &koerper,
               bool mitAnmeldung, const QString &idempotenz)
{
    QNetworkRequest a(QUrl(QLatin1String(BASIS) + pfad));
    a.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
    a.setRawHeader("User-Agent", m_benutzeragent.toUtf8());
    a.setRawHeader("Accept", "application/json");
    if (mitAnmeldung && !m_marke.isEmpty())
        a.setRawHeader("Authorization", "Bearer " + m_marke.toUtf8());
    if (!idempotenz.isEmpty())
        a.setRawHeader("Idempotency-Key", idempotenz.toUtf8());

    QNetworkReply *r;
    if (methode == QLatin1String("PUT"))
        r = m_netz->put(a, koerper);
    else if (methode == QLatin1String("POST"))
        r = m_netz->post(a, koerper);
    else if (methode == QLatin1String("PATCH"))
        r = m_netz->sendCustomRequest(a, "PATCH", new QBuffer(
                new QByteArray(koerper), m_netz));
    else
        r = m_netz->get(a);
    r->setProperty("kennung", kennung);
}

void Netz::fertig(QNetworkReply *r)
{
    r->deleteLater();
    const int status = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QStringList kopf;
    if (r->hasRawHeader("jwt"))
        kopf << QLatin1String("jwt: ") + QString::fromUtf8(r->rawHeader("jwt"));
    auswerten(r->property("kennung").toString(), status, r->readAll(), kopf);
}

#else  // Qt 4: das Netz macht der Rust-Holer.

void Netz::ruf(const QString &kennung, const QString &methode,
               const QString &pfad, const QByteArray &koerper,
               bool mitAnmeldung, const QString &idempotenz)
{
    merken(QString::fromLatin1("%1: %2 %3 (%4 Byte Koerper)")
               .arg(kennung, methode, pfad.left(60))
               .arg(koerper.size()));
    QProcess *p = new QProcess(this);
    p->setProperty("kennung", kennung);
    connect(p, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(holerFertig(int,QProcess::ExitStatus)));

    QStringList arg;
    arg << QLatin1String(BASIS) + pfad;
    arg << QLatin1String("--methode") << methode;
    arg << QLatin1String("--agent") << m_benutzeragent;
    arg << QLatin1String("--sprache")
        << QLocale().name().replace(QLatin1Char('_'), QLatin1Char('-'));
    if (mitAnmeldung && !m_marke.isEmpty())
        arg << QLatin1String("--marke") << m_marke;
    if (!idempotenz.isEmpty())
        arg << QLatin1String("--idempotenz") << idempotenz;

    p->start(QLatin1String(HOLER), arg);
    if (!koerper.isEmpty())
        p->write(koerper);
    p->closeWriteChannel();
}

void Netz::holerFertig(int status, QProcess::ExitStatus art)
{
    Q_UNUSED(art)
    QProcess *p = qobject_cast<QProcess *>(sender());
    if (!p)
        return;
    p->deleteLater();

    // Der Holer trennt sauber: Koerper auf die Standardausgabe, Zustand auf
    // die Fehlerausgabe (`status: 200`, dazu `header: jwt: …`).
    const QByteArray koerper = p->readAllStandardOutput();
    int httpstatus = status == 0 ? 0 : -1;
    QStringList kopf;
    // Kein foreach: der Cross-GCC fuer Harmattan (GCC 14) laeuft Q_FOREACH
    // nur einmal durch.
    const QList<QByteArray> zeilen = p->readAllStandardError().split('\n');
    for (int i = 0; i < zeilen.size(); ++i) {
        const QString zeile = QString::fromUtf8(zeilen.at(i)).trimmed();
        if (zeile.startsWith(QLatin1String("status: ")))
            httpstatus = zeile.mid(8).toInt();
        else if (zeile.startsWith(QLatin1String("header: ")))
            kopf << zeile.mid(8);
        else if (zeile.startsWith(QLatin1String("grund: ")))
            kopf << zeile.mid(7);
    }
    auswerten(p->property("kennung").toString(), httpstatus, koerper, kopf);
}

#endif
