#ifndef LEKTION_H
#define LEKTION_H

// Eine Lektion: holen, Aufgabe fuer Aufgabe beantworten, abgeben.
//
// Die Oberflaeche bekommt jede Aufgabe als flache Karte (`aufgabe`), in der
// nur steht, was zu zeichnen ist -- die Rohdaten einer Aufgabe sind bis zu
// 20 kB gross und haben in QML nichts verloren. Die vier Darstellungen:
//
//   "wahl"    eine aus mehreren Antworten (assist, judge, select, form …)
//   "woerter" Woerter in die richtige Reihenfolge tippen (translate …)
//   "paare"   zwei Spalten zusammenbringen (match, listenMatch)
//   "tippen"  frei eintippen (name, listen, listenComplete …)
//
// Benotet wird hier, nicht beim Server -- siehe src/benoter.h.

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class Netz;
class Konto;

class Lektion : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool laeuft READ laeuft NOTIFY laeuftGeaendert)
    Q_PROPERTY(bool offen READ offen NOTIFY geaendert)
    Q_PROPERTY(int nummer READ nummer NOTIFY geaendert)
    Q_PROPERTY(int anzahl READ anzahl NOTIFY geaendert)
    Q_PROPERTY(int richtige READ richtige NOTIFY geaendert)
    Q_PROPERTY(int herzen READ herzen NOTIFY geaendert)
    Q_PROPERTY(QVariantMap aufgabe READ aufgabe NOTIFY geaendert)
    Q_PROPERTY(bool fertig READ fertig NOTIFY geaendert)

public:
    explicit Lektion(Netz *netz, Konto *konto, QObject *eltern = 0);

    bool laeuft() const { return m_laeuft; }
    bool offen() const { return !m_sitzung.isEmpty(); }
    int nummer() const { return m_nummer; }
    int anzahl() const { return m_aufgaben.size(); }
    int richtige() const { return m_richtige; }
    int herzen() const { return m_herzen; }
    bool fertig() const { return offen() && m_nummer >= m_aufgaben.size(); }
    QVariantMap aufgabe() const { return m_aufgabe; }

    // Eine Uebung ueber den ganzen Kurs -- geht immer, auch wenn der Pfad
    // nichts Offenes hat.
    Q_INVOKABLE void uebenStarten();
    // Eine Stufe des Pfades (Nummer aus Konto::pfad).
    Q_INVOKABLE void stufeStarten(int stufennummer);

    // Die Antwort auf die laufende Aufgabe. `reihe` sind die angetippten
    // Felder, `text` das Eingetippte -- je nach Darstellung ist eines leer.
    Q_INVOKABLE void antworten(const QVariantList &reihe, const QString &text);
    // Weiter zur naechsten Aufgabe (nach der Rueckmeldung).
    Q_INVOKABLE void weiter();
    // Diese Aufgabe auslassen; zaehlt als falsch, so wie in der App.
    Q_INVOKABLE void auslassen();
    // Die fertige Lektion abschicken.
    Q_INVOKABLE void abgeben();
    // Abbrechen, ohne abzugeben -- es geht nichts verloren ausser der Zeit.
    Q_INVOKABLE void abbrechen();

signals:
    void laeuftGeaendert();
    void geaendert();
    void benotet(bool richtig, const QString &loesung);
    void abgegeben(int xp);
    void fehlgeschlagen(const QString &grund);
    // Alle Toene dieser Lektion -- der Lautsprecher holt sie im Voraus.
    void toeneVorladen(const QStringList &adressen);

private slots:
    void antwort(const QString &kennung, const QVariantMap &daten);
    void fehler(const QString &kennung, int status, const QString &text);

private:
    void setzeLaeuft(bool wert);
    void starten(const QString &art, int stufennummer);
    void aufgabeZeigen();
    QVariantMap darstellen(const QVariantMap &roh) const;

    Netz *m_netz;
    Konto *m_konto;
    bool m_laeuft;
    QVariantMap m_sitzung;      // die ganze Sitzung, so wie sie kam
    QVariantList m_aufgaben;    // ihre challenges, zum Beschreiben
    QVariantMap m_aufgabe;      // die laufende Aufgabe, flach fuer QML
    int m_nummer;
    int m_richtige;
    int m_strecke;
    int m_besteStrecke;
    int m_herzen;
    qint64 m_begonnen;
    qint64 m_aufgabeBegonnen;
};

#endif // LEKTION_H
