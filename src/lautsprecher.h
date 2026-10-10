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

#include <QObject>
#include <QProcess>
#include <QString>

class Lautsprecher : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool spielt READ spielt NOTIFY spieltGeaendert)

public:
    explicit Lautsprecher(QObject *eltern = 0);
    ~Lautsprecher();

    bool spielt() const;

    // Spielt die Adresse. Laeuft noch etwas, wird es abgebrochen -- zwei
    // Saetze uebereinander versteht niemand.
    Q_INVOKABLE void spielen(const QString &adresse);
    Q_INVOKABLE void aus();

signals:
    void spieltGeaendert();

private slots:
    void fertig(int status, QProcess::ExitStatus art);

private:
    QProcess *m_lauf;
};

#endif // LAUTSPRECHER_H
