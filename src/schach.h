#ifndef SCHACH_H
#define SCHACH_H

// Duolingos Schach.
//
// Es laeuft **nicht** ueber die Lektionsmaschine, sondern ueber einen
// eigenen kleinen Dienst (doc/SCHACH.md):
//
//   POST  /chess/1/{benutzer}/matches            eine Partie anfangen
//   PATCH /chess/1/{benutzer}/matches/{partie}   einen Zug eintragen
//   GET   /chess/1/{benutzer}/matches            Elo, Bilanz, laufende Partien
//
// Gesprochen wird in FEN und Zugnamen, also in gewoehnlichem Schach. Der
// Dienst fuehrt Buch; **den Gegenzug rechnet der Client** (nachgemessen:
// der Server nimmt den eigenen Zug an und schweigt dazu). Dafuer sitzt hier
// ein kleiner Gegner daneben -- siehe src/bot.h.
//
// Das Brett selbst ist nicht neu geschrieben: `src/core/Position` und
// `third_party/chess-library` kommen aus harbour-schachlehrer, wo sie gegen
// feste Stellungen geprueft sind.

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#ifndef Q_MOC_RUN
#include "core/Position.h"
#endif

class Netz;
class Konto;

class Schach : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool laeuft READ laeuft NOTIFY laeuftGeaendert)
    Q_PROPERTY(bool offen READ offen NOTIFY stellungGeaendert)
    Q_PROPERTY(QString fen READ fen NOTIFY stellungGeaendert)
    // 64 Felder, "wK", "bq" … oder "" -- so, wie das Brett sie erwartet.
    Q_PROPERTY(QVariantList felder READ felder NOTIFY stellungGeaendert)
    Q_PROPERTY(QVariantList ziele READ ziele NOTIFY auswahlGeaendert)
    Q_PROPERTY(int gewaehlt READ gewaehlt NOTIFY auswahlGeaendert)
    Q_PROPERTY(bool untenWeiss READ untenWeiss NOTIFY stellungGeaendert)
    Q_PROPERTY(bool ichAmZug READ ichAmZug NOTIFY stellungGeaendert)
    Q_PROPERTY(QString letzterZug READ letzterZug NOTIFY stellungGeaendert)
    Q_PROPERTY(QStringList zugliste READ zugliste NOTIFY stellungGeaendert)
    Q_PROPERTY(QString stand READ stand NOTIFY stellungGeaendert)
    Q_PROPERTY(QString meldung READ meldung NOTIFY meldungGeaendert)
    Q_PROPERTY(int elo READ elo NOTIFY bilanzGeaendert)
    Q_PROPERTY(int siege READ siege NOTIFY bilanzGeaendert)
    Q_PROPERTY(int niederlagen READ niederlagen NOTIFY bilanzGeaendert)
    Q_PROPERTY(int remis READ remis NOTIFY bilanzGeaendert)
    Q_PROPERTY(bool vorbei READ vorbei NOTIFY stellungGeaendert)
    // "partie" oder "lektion" -- oder leer, wenn nichts laeuft.
    Q_PROPERTY(QString modus READ modus NOTIFY stellungGeaendert)
    Q_PROPERTY(int aufgabeNummer READ aufgabeNummer NOTIFY stellungGeaendert)
    Q_PROPERTY(int aufgabenZahl READ aufgabenZahl NOTIFY stellungGeaendert)
    Q_PROPERTY(QString lehrersatz READ lehrersatz NOTIFY lehrersatzGeaendert)
    Q_PROPERTY(QString lehrerton READ lehrerton NOTIFY lehrersatzGeaendert)

public:
    explicit Schach(Netz *netz, Konto *konto, QObject *eltern = 0);

    bool laeuft() const { return m_laeuft; }
    // Offen ist das Brett bei einer Partie wie in einer Lektion.
    bool offen() const
    {
        return !m_partie.isEmpty() || m_modus == QLatin1String("lektion");
    }
    QString fen() const;
    QVariantList felder() const;
    QVariantList ziele() const { return m_ziele; }
    int gewaehlt() const { return m_gewaehlt; }
    bool untenWeiss() const { return m_meineFarbeWeiss; }
    bool ichAmZug() const;
    QString letzterZug() const;
    QStringList zugliste() const;
    QString stand() const;
    QString meldung() const { return m_meldung; }
    int elo() const { return m_elo; }
    int siege() const { return m_siege; }
    int niederlagen() const { return m_niederlagen; }
    int remis() const { return m_remis; }
    bool vorbei() const;
    QString modus() const { return m_modus; }
    int aufgabeNummer() const { return m_nummer + 1; }
    int aufgabenZahl() const { return m_aufgaben.size(); }
    QString lehrersatz() const { return m_lehrersatz; }
    QString lehrerton() const { return m_lehrerton; }

    // Bilanz und laufende Partien holen.
    Q_INVOKABLE void standHolen();
    // Eine neue Partie gegen den Rechner.
    Q_INVOKABLE void partieStarten(int abschnitt = 1);
    // Eine Lektion aus dem Schachkurs (Nummer aus Konto::pfad).
    Q_INVOKABLE void lektionStarten(int stufennummer);
    // Ein Feld antippen: erst die eigene Figur, dann das Ziel.
    Q_INVOKABLE void feldAntippen(int feld);
    // Umwandlung beantworten ("q", "r", "b", "n").
    Q_INVOKABLE void umwandeln(const QString &figur);
    Q_INVOKABLE void aufgeben();
    Q_INVOKABLE void partieVerlassen();

signals:
    void laeuftGeaendert();
    void stellungGeaendert();
    void auswahlGeaendert();
    void meldungGeaendert();
    void bilanzGeaendert();
    // Eine Umwandlung steht an; die Oberflaeche fragt nach der Figur.
    void umwandlungNoetig();
    void partieZuEnde(const QString &ausgang);
    void lehrersatzGeaendert();
    // In der Lektion: der Zug war nicht der verlangte.
    void zugAbgelehnt(const QString &hinweis);
    void lektionFertig();

private slots:
    void antwort(const QString &kennung, const QVariantMap &daten);
    void gegnerInDerLektion();
    void weiterInDerLektion();
    void fehler(const QString &kennung, int status, const QString &text);
    void gegnerZiehen();

private:
    void setzeLaeuft(bool wert);
    void setzeMeldung(const QString &text);
    void zugAbschicken(const QString &uci, const QString &san);
    void standPruefen();
    void aufgabeZeigen();
    QString erwarteterZug() const;

    Netz *m_netz;
    Konto *m_konto;
    schach::core::Position m_stellung;
    QVariantMap m_partie;        // was der Dienst ueber die Partie sagt
    QString m_partieId;
    bool m_meineFarbeWeiss;
    bool m_laeuft;
    int m_gewaehlt;
    QVariantList m_ziele;
    QString m_meldung;
    QString m_offeneUmwandlungVon;
    QString m_offeneUmwandlungNach;
    int m_elo;
    int m_siege;
    int m_niederlagen;
    int m_remis;
    int m_abschnitt;
    bool m_beendetGemeldet;

    // Lektion: die Aufgaben der Sitzung und die Zugfolge, die verlangt ist.
    QString m_modus;
    QVariantList m_aufgaben;
    int m_nummer;
    QStringList m_erwartet;      // die Zuege der laufenden Aufgabe (UCI)
    QStringList m_gegnerzuege;   // was der Gegner dazwischen zieht
    int m_schritt;               // wie weit die Folge gespielt ist
    QString m_lehrersatz;
    QString m_lehrerton;
};

#endif // SCHACH_H
