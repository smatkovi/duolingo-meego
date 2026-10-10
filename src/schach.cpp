#include "schach.h"
#include "bot.h"
#include "json.h"
#include "konto.h"
#include "netz.h"

#include <QDateTime>
#include <QTimer>
#include <QUrl>

namespace {

const char *PARTIE_NEU = "schach-neu";
const char *PARTIE_ZUG = "schach-zug";
const char *PARTIE_STAND = "schach-stand";
const char *LEKTION = "schach-lektion";

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

Schach::Schach(Netz *netz, Konto *konto, QObject *eltern)
    : QObject(eltern), m_netz(netz), m_konto(konto), m_meineFarbeWeiss(true),
      m_laeuft(false), m_gewaehlt(-1), m_elo(0), m_siege(0), m_niederlagen(0),
      m_remis(0), m_abschnitt(1), m_beendetGemeldet(false), m_nummer(0),
      m_schritt(0)
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

// In der Lektion gibt es keine Partie beim Dienst -- trotzdem steht ein
// Brett da, also zaehlt auch das als "offen".
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
    if (m_modus == QLatin1String("lektion"))
        return !m_erwartet.isEmpty() && m_schritt < m_erwartet.size();
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
    if (m_modus == QLatin1String("lektion"))
        return tr("Aufgabe %1 von %2").arg(aufgabeNummer()).arg(aufgabenZahl());
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

void Schach::lektionStarten(int stufennummer)
{
    if (!m_netz->angemeldet() || !m_konto)
        return;
    const QVariantMap stufe = m_konto->stufe(stufennummer);
    if (stufe.isEmpty()) {
        setzeMeldung(tr("Diese Stufe gibt es nicht."));
        return;
    }
    const QVariantMap daten =
        stufe.value(QLatin1String("pathLevelClientData")).toMap();
    const QVariantList beiwerke =
        daten.value(QLatin1String("sessionMetadatas")).toList();
    QVariant beiwerk;
    if (!beiwerke.isEmpty()) {
        const int fertig = stufe.value(QLatin1String("finishedSessions")).toInt();
        beiwerk = beiwerke.at(fertig < beiwerke.size() ? fertig : 0);
    }

    QVariantList arten;
    arten.append(QLatin1String("chessMatch"));
    arten.append(QLatin1String("chessMiniMatch"));

    QVariantMap koerper;
    koerper.insert(QLatin1String("challengeTypes"), arten);
    koerper.insert(QLatin1String("fromLanguage"),
                   m_konto->muttersprache().isEmpty()
                       ? QLatin1String("en") : m_konto->muttersprache());
    koerper.insert(QLatin1String("isV2"), true);
    koerper.insert(QLatin1String("juicy"), true);
    koerper.insert(QLatin1String("type"), QLatin1String("CHESS_SESSION"));
    koerper.insert(QLatin1String("metadata"), beiwerk);
    koerper.insert(QLatin1String("levelId"), stufe.value(QLatin1String("id")));
    koerper.insert(QLatin1String("levelSessionIndex"),
                   stufe.value(QLatin1String("finishedSessions")).toInt());
    koerper.insert(QLatin1String("pathLevelSessionMetadata"),
                   stufe.value(QLatin1String("pathLevelSessionMetadata")));

    m_partie.clear();
    m_partieId.clear();
    m_aufgaben.clear();
    m_nummer = 0;
    m_modus = QLatin1String("lektion");
    setzeLaeuft(true);
    setzeMeldung(QString());
    emit stellungGeaendert();
    m_netz->ruf(QLatin1String(LEKTION), QLatin1String("POST"),
                QLatin1String("/2023-05-23/sessions"), Json::serialise(koerper));
}

QString Schach::erwarteterZug() const
{
    if (m_schritt < 0 || m_schritt >= m_erwartet.size())
        return QString();
    return m_erwartet.at(m_schritt);
}

void Schach::aufgabeZeigen()
{
    if (m_nummer >= m_aufgaben.size()) {
        m_modus.clear();
        m_erwartet.clear();
        emit stellungGeaendert();
        emit lektionFertig();
        return;
    }
    const QVariantMap aufgabe = m_aufgaben.at(m_nummer).toMap();
    const QVariantMap puzzle =
        aufgabe.value(QLatin1String("chessPuzzleInfo")).toMap();

    m_stellung.setFen(aufgabe.value(QLatin1String("fen")).toString().toStdString());
    m_meineFarbeWeiss = m_stellung.whiteToMove();

    // `correctMoves` ist die verlangte Zugfolge, durch Leerzeichen
    // getrennt: "e1e6 e6b6" heisst erst Turm nach e6, dann nach b6.
    m_erwartet.clear();
    const QVariantList folgen = puzzle.value(QLatin1String("correctMoves")).toList();
    if (!folgen.isEmpty())
        m_erwartet = folgen.first().toString().split(QLatin1Char(' '),
                                                     QString::SkipEmptyParts);
    m_gegnerzuege.clear();
    const QVariantList gegner = puzzle.value(QLatin1String("enemyMoves")).toList();
    for (int i = 0; i < gegner.size(); ++i)
        m_gegnerzuege << gegner.at(i).toString();
    m_schritt = 0;

    // Der Satz des Lehrers. Die Sprachausgabe liegt bei; sie laeuft ueber
    // http, wie alles andere auch.
    m_lehrersatz.clear();
    m_lehrerton.clear();
    const char *quellen[] = { "instructionsSpeechBubbleContent",
                              "hintUsedInstructionsSpeechBubbleContent", 0 };
    for (int q = 0; quellen[q] && m_lehrersatz.isEmpty(); ++q) {
        const QVariantList blasen =
            puzzle.value(QLatin1String(quellen[q])).toList();
        if (blasen.isEmpty())
            continue;
        const QVariantMap erste = blasen.first().toMap();
        m_lehrersatz = erste.value(QLatin1String("text")).toString();
        QString ton = erste.value(QLatin1String("ttsUrl")).toString();
        if (ton.startsWith(QLatin1String("https://")))
            ton.replace(0, 5, QLatin1String("http"));
        m_lehrerton = ton;
    }
    // Die Sterne stehen als fremde Bauern auf dem Brett; der Satz erklaert
    // es. Sternchen im Text sind Hervorhebungen und gehoeren nicht hin.
    m_lehrersatz.remove(QLatin1String("**"));

    m_gewaehlt = -1;
    m_ziele.clear();
    emit lehrersatzGeaendert();
    emit stellungGeaendert();
    emit auswahlGeaendert();
}

void Schach::weiterInDerLektion()
{
    aufgabeZeigen();
}

void Schach::gegnerInDerLektion()
{
    // Der Gegenzug, den die Aufgabe vorschreibt -- nicht gerechnet,
    // sondern abgeschrieben: in einer Lektion soll genau das passieren,
    // was der Lehrer angekuendigt hat.
    const int i = m_schritt - 1;
    if (i < 0 || i >= m_gegnerzuege.size())
        return;
    const QString zug = m_gegnerzuege.at(i);
    if (zug.isEmpty())
        return;
    if (m_stellung.play(zug.toStdString()))
        emit stellungGeaendert();
}

void Schach::partieVerlassen()
{
    m_partie.clear();
    m_partieId.clear();
    m_gewaehlt = -1;
    m_ziele.clear();
    m_beendetGemeldet = false;
    m_modus.clear();
    m_aufgaben.clear();
    m_erwartet.clear();
    m_gegnerzuege.clear();
    m_nummer = 0;
    m_schritt = 0;
    m_lehrersatz.clear();
    m_lehrerton.clear();
    m_stellung.reset();
    emit lehrersatzGeaendert();
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

    if (m_modus == QLatin1String("lektion")) {
        const QString verlangt = erwarteterZug();
        const QString gezogen = von + nach;
        // Die Umwandlung haengt im verlangten Zug schon dran ("e7e8q").
        if (!verlangt.startsWith(gezogen)) {
            m_gewaehlt = -1;
            m_ziele.clear();
            emit auswahlGeaendert();
            emit zugAbgelehnt(tr("Nicht dieser Zug."));
            return;
        }
        if (!m_stellung.play(verlangt.toStdString()))
            return;
        ++m_schritt;
        m_gewaehlt = -1;
        m_ziele.clear();
        emit auswahlGeaendert();
        emit stellungGeaendert();
        if (m_schritt <= m_gegnerzuege.size())
            QTimer::singleShot(400, this, SLOT(gegnerInDerLektion()));
        if (m_schritt >= m_erwartet.size()) {
            // Aufgabe geschafft -- weiter zur naechsten.
            ++m_nummer;
            QTimer::singleShot(700, this, SLOT(weiterInDerLektion()));
        }
        return;
    }
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
    if (kennung == QLatin1String(LEKTION)) {
        setzeLaeuft(false);
        m_aufgaben = daten.value(QLatin1String("challenges")).toList();
        m_nummer = 0;
        if (m_aufgaben.isEmpty()) {
            m_modus.clear();
            setzeMeldung(tr("Diese Lektion kam ohne Aufgaben."));
            emit stellungGeaendert();
            return;
        }
        aufgabeZeigen();
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
        && kennung != QLatin1String(PARTIE_STAND)
        && kennung != QLatin1String(LEKTION))
        return;
    setzeLaeuft(false);
    if (kennung == QLatin1String(PARTIE_STAND))
        return;
    setzeMeldung(status > 0 ? tr("%1 (%2)").arg(text).arg(status) : text);
}
