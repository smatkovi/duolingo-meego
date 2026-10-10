#ifndef KONTO_H
#define KONTO_H

// Das Konto: anmelden, und was oben auf dem Schirm steht -- Straehne, XP,
// Herzen, der laufende Kurs und sein Pfad.
//
// Die Anmeldung ist blankes JSON (`POST /2017-06-30/login?fields=jwt`) und
// verlangt keine Pruefungsaufgabe; die Marke kommt im Antwortkopf zurueck,
// darum steht das Einsammeln in Netz, nicht hier.

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class Netz;

class Konto : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool angemeldet READ angemeldet NOTIFY geaendert)
    Q_PROPERTY(bool laeuft READ laeuft NOTIFY laeuftGeaendert)
    Q_PROPERTY(QString name READ name NOTIFY geaendert)
    Q_PROPERTY(int straehne READ straehne NOTIFY geaendert)
    Q_PROPERTY(int xp READ xp NOTIFY geaendert)
    Q_PROPERTY(int herzen READ herzen NOTIFY geaendert)
    Q_PROPERTY(bool herzenZaehlen READ herzenZaehlen NOTIFY geaendert)
    Q_PROPERTY(QString kursName READ kursName NOTIFY geaendert)
    Q_PROPERTY(QString lernsprache READ lernsprache NOTIFY geaendert)
    Q_PROPERTY(QString muttersprache READ muttersprache NOTIFY geaendert)
    // Der Pfad, flach: je Eintrag eine Stufe mit Einheit, Zustand und
    // Fortschritt. Mehr braucht eine Liste auf 480 Pixel nicht.
    Q_PROPERTY(QVariantList pfad READ pfad NOTIFY geaendert)

public:
    explicit Konto(Netz *netz, QObject *eltern = 0);

    bool angemeldet() const;
    bool laeuft() const { return m_laeuft; }
    QString name() const { return m_name; }
    int straehne() const { return m_straehne; }
    int xp() const { return m_xp; }
    int herzen() const { return m_herzen; }
    bool herzenZaehlen() const { return m_herzenZaehlen; }
    QString kursName() const;
    QString lernsprache() const { return m_lernsprache; }
    QString muttersprache() const { return m_muttersprache; }
    QVariantList pfad() const { return m_pfad; }
    QVariantMap kurs() const { return m_kurs; }

    Q_INVOKABLE void anmelden(const QString &kennung, const QString &passwort);
    Q_INVOKABLE void abmelden();
    // Konto und Pfad neu holen -- nach dem Start, nach einer Lektion.
    Q_INVOKABLE void auffrischen();
    // Die Stufe zu einer Zeile des Pfades, so wie Lektion sie braucht.
    Q_INVOKABLE QVariantMap stufe(int nummer) const;
    // Die naechste Stufe, die dran ist (fuer den grossen Knopf).
    Q_INVOKABLE int naechsteStufe() const;

    // Der volle Name einer Sprache, soweit wir ihn kennen ("es" -> Spanisch).
    Q_INVOKABLE static QString spracheName(const QString &kuerzel);

signals:
    void geaendert();
    void laeuftGeaendert();
    void angemeldetWorden();
    void anmeldungAbgelehnt(const QString &grund);
    void fehlgeschlagen(const QString &grund);

private slots:
    void antwort(const QString &kennung, const QVariantMap &daten);
    void fehler(const QString &kennung, int status, const QString &text);

private:
    void setzeLaeuft(bool wert);
    void pfadLesen(const QVariantMap &kurs);

    Netz *m_netz;
    bool m_laeuft;
    QString m_name;
    int m_straehne;
    int m_xp;
    int m_herzen;
    bool m_herzenZaehlen;
    QString m_lernsprache;
    QString m_muttersprache;
    QVariantMap m_kurs;
    QVariantList m_pfad;
    QList<QVariantMap> m_stufen;
};

#endif // KONTO_H
