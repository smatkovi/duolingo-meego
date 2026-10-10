#ifndef NETZ_H
#define NETZ_H

// Die Leitung zu Duolingo.
//
// Ein einziger Ort fuer alles, was an jeder Anfrage haengt: der Benutzer-
// agent, die Marke (JWT) und ihr Aufbewahren. Die Oberflaeche spricht nur
// mit dieser Klasse.
//
// Auf Harmattan kann Qt 4.7 kein TLS 1.2, deshalb macht das Netz dort der
// Rust-Holer daneben (meego/fetch) -- dieselbe Schnittstelle, andere
// Innereien. Der Qt-5-Zweig steht daneben, damit eine spaetere
// Sailfish-Fassung nichts kostet.
//
// Zwei Besonderheiten bei Duolingo, beide gemessen (doc/BEFUND.md):
//   * die Anmeldung gibt die Marke im **Antwortkopf** `jwt` zurueck, nicht
//     im Koerper;
//   * es gibt zwei Basen: `/2017-06-30` (Konto, Anmeldung) und
//     `/2023-05-23` (Sitzungen). Der Pfad sagt, welche gemeint ist.

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#if QT_VERSION >= 0x050000
#include <QNetworkAccessManager>
#include <QNetworkReply>
#else
#include <QProcess>
#endif

class Netz : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool angemeldet READ angemeldet NOTIFY anmeldungGeaendert)
    Q_PROPERTY(QString benutzerId READ benutzerId NOTIFY anmeldungGeaendert)

public:
    explicit Netz(QObject *eltern = 0);

    bool angemeldet() const { return !m_marke.isEmpty(); }
    QString benutzerId() const { return m_benutzerId; }

    // Ein Aufruf an die Schnittstelle. `kennung` kommt in den Antwort-
    // signalen wieder, damit die Oberflaeche weiss, wessen Antwort da ist.
    // `methode` ist "GET", "POST" oder "PUT".
    void ruf(const QString &kennung, const QString &methode,
             const QString &pfad, const QByteArray &koerper = QByteArray(),
             bool mitAnmeldung = true,
             const QString &idempotenz = QString());

    // Marke und Kennung aus der Anmeldung uebernehmen. Die Kennung steht
    // als `sub` in der Marke selbst -- ein Aufruf weniger.
    void marke(const QString &jwt);
    Q_INVOKABLE void abmelden();
    QString benutzeragent() const { return m_benutzeragent; }

    // Die Kennung aus einer Marke lesen (`sub` im mittleren Teil).
    static QString kennungAusMarke(const QString &jwt);

signals:
    // Eine Antwort ist da. `daten` ist das geparste JSON als Karte; bei
    // einer Liste an der Wurzel steckt sie unter dem Schluessel "liste".
    void antwort(const QString &kennung, const QVariantMap &daten);
    void fehler(const QString &kennung, int status, const QString &text);
    void anmeldungGeaendert();
    // Der Server hat die Marke abgelehnt (401). Die Oberflaeche soll die
    // Anmeldung zeigen.
    void markeAbgelaufen();

private slots:
#if QT_VERSION >= 0x050000
    void fertig(QNetworkReply *antwortObjekt);
#else
    void holerFertig(int status, QProcess::ExitStatus art);
#endif

private:
    void auswerten(const QString &kennung, int status,
                   const QByteArray &koerper, const QStringList &kopf);
    void markeLaden();
    void markeSichern();

    QString m_benutzeragent;
    QString m_marke;
    QString m_benutzerId;

#if QT_VERSION >= 0x050000
    QNetworkAccessManager *m_netz;
#endif
};

#endif // NETZ_H
