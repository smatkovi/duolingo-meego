#include "lautsprecher.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QUrl>
#include <QtGlobal>

namespace {

// Mit vollem Pfad, nicht nur dem Namen: ein aus der Startansicht
// gestartetes Programm hat nicht unbedingt denselben PATH wie eine
// ssh-Sitzung, und ein stummer Knopf waere der zweite Fehler derselben
// Art.
#if QT_VERSION >= 0x050000
const char *SPIELER = "/usr/bin/gst-launch-1.0";
const char *BAUTEIL = "playbin";
#else
const char *SPIELER = "/usr/bin/gst-launch-0.10";
const char *BAUTEIL = "playbin2";
#endif

}  // namespace

Lautsprecher::Lautsprecher(QObject *eltern)
    : QObject(eltern), m_lauf(0)
{
    m_vorrat = QDir::homePath() + QLatin1String("/.cache/duolingo/ton");
    QDir().mkpath(m_vorrat);
    vorratAufraeumen();
    // Fuer den Vorrat reicht Qts eigenes Netz: die Tonadressen sind
    // **http**, und daran scheitert Qt 4.7 nicht -- gescheitert ist es nur
    // an TLS. Der Rust-Holer wird hier also nicht gebraucht.
    m_netz = new QNetworkAccessManager(this);
    connect(m_netz, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(geladen(QNetworkReply*)));
}

Lautsprecher::~Lautsprecher()
{
    aus();
}

QString Lautsprecher::vorratsdatei(const QString &adresse) const
{
    if (adresse.isEmpty())
        return QString();
    const QByteArray kennung =
        QCryptographicHash::hash(adresse.toUtf8(), QCryptographicHash::Md5)
            .toHex();
    return m_vorrat + QLatin1Char('/') + QString::fromLatin1(kennung)
           + QLatin1String(".mp3");
}

void Lautsprecher::vorratAufraeumen()
{
    // Der Vorrat darf nicht wachsen, bis die kleine Benutzerpartition
    // voll ist. Ueber 300 Stueck fliegt das Aelteste raus.
    QDir ordner(m_vorrat);
    QFileInfoList dateien = ordner.entryInfoList(QDir::Files, QDir::Time);
    for (int i = 300; i < dateien.size(); ++i)
        QFile::remove(dateien.at(i).absoluteFilePath());
}

void Lautsprecher::vorladen(const QString &adresse)
{
    if (adresse.isEmpty() || m_unterwegs.contains(adresse))
        return;
    const QString ziel = vorratsdatei(adresse);
    if (ziel.isEmpty() || QFileInfo(ziel).size() > 0)
        return;
    m_unterwegs.insert(adresse, true);
    QNetworkRequest anfrage((QUrl(adresse)));
    anfrage.setRawHeader("User-Agent", "Duolingo-MeeGo");
    m_netz->get(anfrage);
}

void Lautsprecher::vorladenListe(const QStringList &adressen)
{
    for (int i = 0; i < adressen.size(); ++i) {
        const QString a = adressen.at(i);
        if (a.isEmpty() || m_unterwegs.contains(a)
            || m_warteschlange.contains(a))
            continue;
        const QString ziel = vorratsdatei(a);
        if (!ziel.isEmpty() && QFileInfo(ziel).size() > 0)
            continue;
        m_warteschlange.append(a);
    }
    naechstesHolen();
}

void Lautsprecher::naechstesHolen()
{
    // Drei auf einmal: genug, dass es zuegig geht, wenig genug, dass die
    // laufende Aufgabe ihren Ton zuerst bekommt.
    while (m_unterwegs.size() < 3 && !m_warteschlange.isEmpty())
        vorladen(m_warteschlange.takeFirst());
}

void Lautsprecher::geladen(QNetworkReply *antwort)
{
    antwort->deleteLater();
    const QString adresse = antwort->request().url().toString();
    m_unterwegs.remove(adresse);
    naechstesHolen();
    if (antwort->error() != QNetworkReply::NoError)
        return;
    const QByteArray daten = antwort->readAll();
    if (daten.isEmpty())
        return;
    QFile datei(vorratsdatei(adresse));
    if (!datei.open(QIODevice::WriteOnly))
        return;
    datei.write(daten);
    datei.close();
}

bool Lautsprecher::spielt() const
{
    return m_lauf && m_lauf->state() != QProcess::NotRunning;
}

void Lautsprecher::spielen(const QString &adresse)
{
    if (adresse.isEmpty())
        return;
    aus();

    m_lauf = new QProcess(this);
    connect(m_lauf, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(fertig(int,QProcess::ExitStatus)));
    // Aus dem Vorrat, wenn er da ist -- das spart das Laden und damit den
    // groesseren Teil der Wartezeit.
    const QString datei = vorratsdatei(adresse);
    const bool imVorrat = !datei.isEmpty() && QFileInfo(datei).size() > 0;
    const QString uri = imVorrat
                            ? QUrl::fromLocalFile(datei).toString()
                            : adresse;
    if (!imVorrat)
        vorladen(adresse);   // fuers naechste Mal

    QStringList arg;
    arg << QLatin1String(BAUTEIL) << QLatin1String("uri=") + uri;
    m_lauf->start(QLatin1String(SPIELER), arg);
    emit spieltGeaendert();
}

void Lautsprecher::aus()
{
    if (!m_lauf)
        return;
    QProcess *alt = m_lauf;
    m_lauf = 0;
    alt->disconnect(this);
    if (alt->state() != QProcess::NotRunning) {
        alt->terminate();
        if (!alt->waitForFinished(300))
            alt->kill();
    }
    alt->deleteLater();
    emit spieltGeaendert();
}

void Lautsprecher::fertig(int status, QProcess::ExitStatus art)
{
    if (art != QProcess::NormalExit || status != 0)
        qWarning("Ton: %s endete mit %d", SPIELER, status);
    if (m_lauf) {
        m_lauf->deleteLater();
        m_lauf = 0;
    }
    emit spieltGeaendert();
}
