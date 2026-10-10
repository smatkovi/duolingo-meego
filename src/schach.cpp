#include "schach.h"
#include "bot.h"
#include "json.h"
#include "netz.h"

#include <QDateTime>
#include <QTimer>
#include <QUrl>

namespace {

const char *PARTIE_NEU = "schach-neu";
const char *PARTIE_ZUG = "schach-zug";
const char *PARTIE_STAND = "schach-stand";

QString feldName(int feld)
{
    if (feld < 0 || feld > 63)
        return QString();
    return QString(QLatin1Char('a' + (feld % 8)))
           + QString::number(feld / 8 + 1);
}

int feldNummer(const QString &name)
{
    if (name.size() < 2)
        return -1;
    const int linie = name.at(0).toLatin1() - 'a';
    const int reihe = name.at(1).toLatin1() - '1';
    if (linie < 0 || linie > 7 || reihe < 0 || reihe > 7)
        return -1;
    return reihe * 8 + linie;
}

}  // namespace

Schach::Schach(Netz *netz, QObject *eltern)
    : QObject(eltern), m_netz(netz), m_meineFarbeWeiss(true), m_laeuft(false),
      m_gewaehlt(-1), m_elo(0), m_siege(0), m_niederlagen(0), m_remis(0),
      m_abschnitt(1), m_beendetGemeldet(false)
{
    connect(m_netz, SIGNAL(antwort(QString,QVariantMap)),
            this, SLOT(antwort(QString,QVariantMap)));
    connect(m_netz, SIGNAL(fehler(QString,int,QString)),
            this, SLOT(fehler(QString,int,QString)));
}

void Schach::setzeLaeuft(bool wert)
{
    if (m_laeuft == wert)
        return;
    m_laeuft = wert;
    emit laeuftGeaendert();
}

void Schach::setzeMeldung(const QString &text)
{
    if (m_meldung == text)
        return;
    m_meldung = text;
    emit meldungGeaendert();
}

QString Schach::fen() const
{
    return QString::fromStdString(m_stellung.fen());
}

QVariantList Schach::felder() const
{
    // Aus der FEN 64 Eintraege machen: "wK", "bq" … oder "". Dieselbe
    // Schreibweise, die das Brett in harbour-schachlehrer erwartet.
    QVariantList raus;
    for (int i = 0; i < 64; ++i)
        raus.append(QString());
    const QString f = fen();
    int feld = 56;
    for (int i = 0; i < f.size(); ++i) {
        const QChar c = f.at(i);
        if (c == QLatin1Char(' '))
            break;
        if (c == QLatin1Char('/')) {
            feld -= 16;
            continue;
        }
        if (c.isDigit()) {
            feld += c.toLatin1() - '0';
            continue;
        }
        if (feld >= 0 && feld < 64) {
            const bool weiss = c.isUpper();
            raus[feld] = QString(weiss ? QLatin1Char('w') : QLatin1Char('b'))
                         + QString(c.toLower());
        }
        ++feld;
    }
    return raus;
}

bool Schach::ichAmZug() const
{
    return offen() && !vorbei() && m_stellung.whiteToMove() == m_meineFarbeWeiss;
}

bool Schach::vorbei() const
{
    return m_stellung.gameOver();
}

QString Schach::letzterZug() const
{
    return QString::fromStdString(m_stellung.lastMove());
}

QStringList Schach::zugliste() const
{
    QStringList raus;
    const std::vector<std::string> &liste = m_stellung.sanHistory();
    for (std::vector<std::string>::const_iterator z = liste.begin();
         z != liste.end(); ++z)
        raus << QString::fromStdString(*z);
    return raus;
}

QString Schach::stand() const
{
    if (!offen())
        return QString();
    switch (m_stellung.outcome()) {
    case schach::core::Outcome::WhiteWins:
        return m_meineFarbeWeiss ? tr("Gewonnen") : tr("Verloren");
    case schach::core::Outcome::BlackWins:
        return m_meineFarbeWeiss ? tr("Verloren") : tr("Gewonnen");
    case schach::core::Outcome::Draw:
        return tr("Remis");
    default:
        break;
    }
    if (m_stellung.inCheck())
        return ichAmZug() ? tr("Schach!") : tr("Schach geboten");
    return ichAmZug() ? tr("Du bist am Zug") : tr("Der Rechner ueberlegt");
}

void Schach::standHolen()
{
    if (!m_netz->angemeldet())
        return;
    m_netz->ruf(QLatin1String(PARTIE_STAND), QLatin1String("GET"),
                QString::fromLatin1("/chess/1/%1/matches?sectionNumber=%2")
                    .arg(m_netz->benutzerId()).arg(m_abschnitt));
}

void Schach::partieStarten(int abschnitt)
{
    if (!m_netz->angemeldet())
        return;
    m_abschnitt = abschnitt > 0 ? abschnitt : 1;
    QVariantMap koerper;
    koerper.insert(QLatin1String("sectionNumber"), m_abschnitt);
    koerper.insert(QLatin1String("userTimezone"), QLatin1String("Europe/Vienna"));
    setzeLaeuft(true);
    setzeMeldung(QString());
    m_netz->ruf(QLatin1String(PARTIE_NEU), QLatin1String("POST"),
                QString::fromLatin1("/chess/1/%1/matches").arg(m_netz->benutzerId()),
                Json::serialise(koerper));
}

void Schach::partieVerlassen()
{
    m_partie.clear();
    m_partieId.clear();
    m_gewaehlt = -1;
    m_ziele.clear();
    m_beendetGemeldet = false;
    m_stellung.reset();
    emit stellungGeaendert();
    emit auswahlGeaendert();
}

void Schach::feldAntippen(int feld)
{
    if (!offen() || !ichAmZug() || feld < 0 || feld > 63)
        return;

    const QVariantList stand = felder();
    const QString dort = stand.at(feld).toString();
    const bool eigene = !dort.isEmpty()
                        && (dort.at(0) == QLatin1Char('w')) == m_meineFarbeWeiss;

    if (m_gewaehlt < 0 || eigene) {
        if (!eigene) {
            m_gewaehlt = -1;
            m_ziele.clear();
        } else {
            m_gewaehlt = feld;
            m_ziele.clear();
            const std::vector<int> ziele = m_stellung.legalTargets(feld);
            for (std::vector<int>::const_iterator z = ziele.begin();
                 z != ziele.end(); ++z)
                m_ziele.append(*z);
        }
        emit auswahlGeaendert();
        return;
    }

    // Zweiter Tipper: das Ziel.
    const QString von = feldName(m_gewaehlt);
    const QString nach = feldName(feld);
    if (m_stellung.needsPromotion(m_gewaehlt, feld)) {
        m_offeneUmwandlungVon = von;
        m_offeneUmwandlungNach = nach;
        emit umwandlungNoetig();
        return;
    }

    const QString uci = von + nach;
    const QString san = QString::fromStdString(
        m_stellung.sanOf(uci.toStdString()));
    if (san.isEmpty()) {
        // Kein gueltiger Zug: Auswahl aufheben, nicht meckern.
        m_gewaehlt = -1;
        m_ziele.clear();
        emit auswahlGeaendert();
        return;
    }
    zugAbschicken(uci, san);
}

void Schach::umwandeln(const QString &figur)
{
    if (m_offeneUmwandlungVon.isEmpty())
        return;
    const QString uci = m_offeneUmwandlungVon + m_offeneUmwandlungNach
                        + figur.toLower();
    m_offeneUmwandlungVon.clear();
    m_offeneUmwandlungNach.clear();
    const QString san = QString::fromStdString(m_stellung.sanOf(uci.toStdString()));
    if (san.isEmpty())
        return;
    zugAbschicken(uci, san);
}

void Schach::zugAbschicken(const QString &uci, const QString &san)
{
    if (!m_stellung.play(uci.toStdString()))
        return;
    m_gewaehlt = -1;
    m_ziele.clear();
    emit auswahlGeaendert();
    emit stellungGeaendert();

    // Dem Dienst Bescheid sagen. Was er braucht, ist der Zugname, die neue
    // Stellung und die ganze Zugliste -- ohne sie merkt er sich den Zug
    // nicht (nachgemessen, doc/SCHACH.md).
    QVariantMap koerper;
    koerper.insert(QLatin1String("moveName"), san);
    koerper.insert(QLatin1String("fen"), fen());
    QVariantList zuege;
    const QStringList liste = zugliste();
    for (int i = 0; i < liste.size(); ++i)
        zuege.append(liste.at(i));
    koerper.insert(QLatin1String("moveHistory"), zuege);
    if (vorbei()) {
        koerper.insert(QLatin1String("status"), QLatin1String("completed"));
        koerper.insert(QLatin1String("outcome"), stand() == tr("Gewonnen")
                           ? QLatin1String("win")
                           : (stand() == tr("Remis") ? QLatin1String("draw")
                                                     : QLatin1String("loss")));
    }
    if (!m_partieId.isEmpty())
        m_netz->ruf(QLatin1String(PARTIE_ZUG), QLatin1String("PATCH"),
                    QString::fromLatin1("/chess/1/%1/matches/%2")
                        .arg(m_netz->benutzerId(), QString::fromUtf8(
                                 QUrl::toPercentEncoding(m_partieId))),
                    Json::serialise(koerper));

    standPruefen();
    if (!vorbei())
        // Der Rechner zieht gleich -- aber erst, nachdem das Brett den
        // eigenen Zug gezeigt hat.
        QTimer::singleShot(350, this, SLOT(gegnerZiehen()));
}

void Schach::gegnerZiehen()
{
    if (!offen() || vorbei() || ichAmZug())
        return;
    // Staerke nach Abschnitt: der erste ist zum Lernen da.
    const int staerke = m_abschnitt <= 1 ? 1 : (m_abschnitt >= 4 ? 3 : 2);
    const std::string zug = Bot::zug(m_stellung, staerke);
    if (zug.empty())
        return;
    const QString san = QString::fromStdString(m_stellung.sanOf(zug));
    if (!m_stellung.play(zug))
        return;
    emit stellungGeaendert();

    QVariantMap koerper;
    koerper.insert(QLatin1String("moveName"), san);
    koerper.insert(QLatin1String("fen"), fen());
    QVariantList zuege;
    const QStringList liste = zugliste();
    for (int i = 0; i < liste.size(); ++i)
        zuege.append(liste.at(i));
    koerper.insert(QLatin1String("moveHistory"), zuege);
    if (!m_partieId.isEmpty())
        m_netz->ruf(QLatin1String(PARTIE_ZUG), QLatin1String("PATCH"),
                    QString::fromLatin1("/chess/1/%1/matches/%2")
                        .arg(m_netz->benutzerId(), QString::fromUtf8(
                                 QUrl::toPercentEncoding(m_partieId))),
                    Json::serialise(koerper));
    standPruefen();
}

void Schach::standPruefen()
{
    if (!vorbei() || m_beendetGemeldet)
        return;
    m_beendetGemeldet = true;
    emit partieZuEnde(stand());
    standHolen();
}

void Schach::aufgeben()
{
    if (!offen())
        return;
    setzeMeldung(tr("Aufgegeben."));
    QVariantMap koerper;
    koerper.insert(QLatin1String("status"), QLatin1String("completed"));
    koerper.insert(QLatin1String("outcome"), QLatin1String("loss"));
    koerper.insert(QLatin1String("endCondition"), QLatin1String("resignation"));
    koerper.insert(QLatin1String("fen"), fen());
    if (!m_partieId.isEmpty())
        m_netz->ruf(QLatin1String(PARTIE_ZUG), QLatin1String("PATCH"),
                    QString::fromLatin1("/chess/1/%1/matches/%2")
                        .arg(m_netz->benutzerId(), QString::fromUtf8(
                                 QUrl::toPercentEncoding(m_partieId))),
                    Json::serialise(koerper));
    partieVerlassen();
    standHolen();
}

void Schach::antwort(const QString &kennung, const QVariantMap &daten)
{
    if (kennung == QLatin1String(PARTIE_STAND)) {
        m_elo = daten.value(QLatin1String("eloRating")).toInt();
        const QVariantMap bilanz = daten.value(QLatin1String("record")).toMap();
        m_siege = bilanz.value(QLatin1String("wins")).toInt();
        m_niederlagen = bilanz.value(QLatin1String("losses")).toInt();
        m_remis = bilanz.value(QLatin1String("draws")).toInt();
        emit bilanzGeaendert();
        return;
    }
    if (kennung != QLatin1String(PARTIE_NEU) && kennung != QLatin1String(PARTIE_ZUG))
        return;
    setzeLaeuft(false);
    if (kennung == QLatin1String(PARTIE_ZUG))
        return;   // der Dienst fuehrt nur Buch; die Stellung kennen wir selbst

    m_partie = daten.value(QLatin1String("match")).toMap();
    m_partieId = m_partie.value(QLatin1String("id")).toString();
    m_meineFarbeWeiss = m_partie.value(QLatin1String("playerColor")).toString()
                        != QLatin1String("black");
    const QString fen = m_partie.value(QLatin1String("boardFen")).toString();
    m_stellung.setFen(fen.isEmpty() ? std::string(
                          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
                                    : fen.toStdString());
    // Was schon gespielt ist, nachspielen -- bei einer fortgesetzten Partie.
    const QVariantList zuege = m_partie.value(QLatin1String("moveHistory")).toList();
    for (int i = 0; i < zuege.size(); ++i)
        m_stellung.playSan(zuege.at(i).toString().toStdString());

    m_gewaehlt = -1;
    m_ziele.clear();
    m_beendetGemeldet = false;
    emit stellungGeaendert();
    emit auswahlGeaendert();
    standHolen();

    if (!ichAmZug() && !vorbei())
        QTimer::singleShot(400, this, SLOT(gegnerZiehen()));
}

void Schach::fehler(const QString &kennung, int status, const QString &text)
{
    if (kennung != QLatin1String(PARTIE_NEU)
        && kennung != QLatin1String(PARTIE_ZUG)
        && kennung != QLatin1String(PARTIE_STAND))
        return;
    setzeLaeuft(false);
    if (kennung == QLatin1String(PARTIE_STAND))
        return;
    setzeMeldung(status > 0 ? tr("%1 (%2)").arg(text).arg(status) : text);
}
