#include "lautsprecher.h"

#include <QFile>
#include <QStringList>
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
}

Lautsprecher::~Lautsprecher()
{
    aus();
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
    QStringList arg;
    arg << QLatin1String(BAUTEIL) << QLatin1String("uri=") + adresse;
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
