#ifndef STUBS_H
#define STUBS_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Attrappen fuer die drei Objekte, mit denen die Seiten reden.
//
// Der Pruefer liest das QML nur ein und loest seine Bindungen auf; die
// Attrappen brauchen also die richtigen Namen und Signaturen und sonst
// nichts. Sie stehen hier ausgeschrieben statt aus den echten Koepfen
// erzeugt, weil die echten den Rust-Holer mitbringen, den es auf dem
// Baurechner nicht gibt -- und eine Attrappe, die vom Original abdriftet,
// faellt im ARM-Bau auf, der die echten benutzt.
//
// **Die NOTIFY-Namen muessen die echten sein**, sonst findet der Pruefer
// einen Connections-Block auf ein nicht vorhandenes Signal nicht.

class StubNetz : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool angemeldet READ flagge NOTIFY anmeldungGeaendert)
    Q_PROPERTY(QString benutzerId READ text NOTIFY anmeldungGeaendert)
public:
    bool flagge() const { return false; }
    QString text() const { return QString(); }
    Q_INVOKABLE bool einspielen(const QString &) { return false; }
    Q_INVOKABLE void abmelden() {}
signals:
    void anmeldungGeaendert();
    void markeAbgelaufen();
    void antwort(const QString &, const QVariantMap &);
    void fehler(const QString &, int, const QString &);
};

class StubKonto : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool angemeldet READ flagge NOTIFY geaendert)
    Q_PROPERTY(bool laeuft READ flagge NOTIFY laeuftGeaendert)
    Q_PROPERTY(QString name READ text NOTIFY geaendert)
    Q_PROPERTY(int straehne READ zahl NOTIFY geaendert)
    Q_PROPERTY(int xp READ zahl NOTIFY geaendert)
    Q_PROPERTY(int herzen READ zahl NOTIFY geaendert)
    Q_PROPERTY(bool herzenZaehlen READ flagge NOTIFY geaendert)
    Q_PROPERTY(QString kursName READ text NOTIFY geaendert)
    Q_PROPERTY(QString lernsprache READ text NOTIFY geaendert)
    Q_PROPERTY(QString muttersprache READ text NOTIFY geaendert)
    Q_PROPERTY(QVariantList pfad READ liste NOTIFY geaendert)
    Q_PROPERTY(QVariantList kurse READ liste NOTIFY geaendert)
    Q_PROPERTY(QVariantList angebote READ liste NOTIFY angeboteGeaendert)
public:
    bool flagge() const { return false; }
    int zahl() const { return 0; }
    QString text() const { return QString(); }
    QVariantList liste() const { return QVariantList(); }
    Q_INVOKABLE void anmelden(const QString &, const QString &) {}
    Q_INVOKABLE void passwortAnfordern(const QString &) {}
    Q_INVOKABLE bool markeEinspielen(const QString &) { return false; }
    Q_INVOKABLE void abmelden() {}
    Q_INVOKABLE void auffrischen() {}
    Q_INVOKABLE QVariantMap stufe(int) const { return QVariantMap(); }
    Q_INVOKABLE int naechsteStufe() const { return -1; }
    Q_INVOKABLE void kursWechseln(const QString &) {}
    Q_INVOKABLE void angeboteHolen() {}
    Q_INVOKABLE QString spracheName(const QString &) const { return QString(); }
signals:
    void geaendert();
    void angeboteGeaendert();
    void kursGewechselt();
    void laeuftGeaendert();
    void angemeldetWorden();
    void postUnterwegs();
    void anmeldungAbgelehnt(const QString &grund);
    void fehlgeschlagen(const QString &grund);
};

class StubLektion : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool laeuft READ flagge NOTIFY laeuftGeaendert)
    Q_PROPERTY(bool offen READ flagge NOTIFY geaendert)
    Q_PROPERTY(int nummer READ zahl NOTIFY geaendert)
    Q_PROPERTY(int anzahl READ zahl NOTIFY geaendert)
    Q_PROPERTY(int richtige READ zahl NOTIFY geaendert)
    Q_PROPERTY(int herzen READ zahl NOTIFY geaendert)
    Q_PROPERTY(QVariantMap aufgabe READ karte NOTIFY geaendert)
    Q_PROPERTY(bool fertig READ flagge NOTIFY geaendert)
public:
    bool flagge() const { return false; }
    int zahl() const { return 0; }
    QVariantMap karte() const { return QVariantMap(); }
    Q_INVOKABLE void uebenStarten() {}
    Q_INVOKABLE void stufeStarten(int) {}
    Q_INVOKABLE void antworten(const QVariantList &, const QString &) {}
    Q_INVOKABLE void weiter() {}
    Q_INVOKABLE void auslassen() {}
    Q_INVOKABLE void abgeben() {}
    Q_INVOKABLE void abbrechen() {}
signals:
    void laeuftGeaendert();
    void geaendert();
    void benotet(bool richtig, const QString &loesung);
    void abgegeben(int xp);
    void fehlgeschlagen(const QString &grund);
};

#endif // STUBS_H
