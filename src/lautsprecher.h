#ifndef LAUTSPRECHER_H
#define LAUTSPRECHER_H

// Der Ton.
//
// Erst lief das ueber das QML-Bauteil `Audio` aus QtMultimediaKit -- der
// Knopf liess sich druecken, aber es kam nichts. **Was auf dem Geraet
// nachweislich spielt, ist gstreamer selbst**: `gst-launch-0.10 playbin2
// uri=http://…` gibt den Ton aus (nachgemessen am N950, auch direkt aus
// dem Netz). Also wird er genommen.
//
// Das kostet beim Antippen etwa eine Sekunde, bis der Ton anfaengt -- den
// Programmstart von gst-launch plus das Laden. Dafuer ist es ein Weg, der
// belegt ist, statt eines, der stumm bleibt.
//
// Die Adressen kommen schon als `http` an (src/lektion.cpp schreibt sie
// um), denn gstreamer auf Harmattan kann kein TLS.

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

class QNetworkReply;

class Lautsprecher : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool spielt READ spielt NOTIFY spieltGeaendert)

public:
    explicit Lautsprecher(QObject *eltern = 0);
    ~Lautsprecher();

    bool spielt() const;

    // Spielt die Adresse. Laeuft noch etwas, wird es abgebrochen -- zwei
    // Saetze uebereinander versteht niemand. Liegt der Ton schon im
    // Vorrat, faengt er sofort an; sonst wird er aus dem Netz gespielt.
    Q_INVOKABLE void spielen(const QString &adresse);
    Q_INVOKABLE void aus();

    // Den Ton holen, bevor jemand drauftippt. Das ist der Unterschied
    // zwischen "eine Sekunde warten" und "sofort": gst-launch startet
    // schnell, das Laden ueber die Leitung dauert.
    Q_INVOKABLE void vorladen(const QString &adresse);

public slots:
    // Eine ganze Lektion auf einmal vorbereiten. Geholt wird der Reihe
    // nach und immer nur ein paar gleichzeitig -- eine Lektion bringt
    // schnell vierzig Toene mit, und ueber GPRS soll das nicht die
    // Leitung verstopfen, bevor die erste Aufgabe dasteht.
    void vorladenListe(const QStringList &adressen);

public:

signals:
    void spieltGeaendert();

private slots:
    void fertig(int status, QProcess::ExitStatus art);
    void geladen(QNetworkReply *antwort);

private:
    // Wo ein Ton im Vorrat liegt -- oder leer, wenn er noch nicht da ist.
    QString vorratsdatei(const QString &adresse) const;
    void vorratAufraeumen();

    void naechstesHolen();

    QProcess *m_lauf;
    QNetworkAccessManager *m_netz;
    QString m_vorrat;
    QHash<QString, bool> m_unterwegs;
    QStringList m_warteschlange;
};

#endif // LAUTSPRECHER_H
