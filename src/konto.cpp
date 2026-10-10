#include "konto.h"
#include "json.h"
#include "netz.h"

#include <QStringList>
#include <QUuid>

namespace {

// Was wir vom Konto brauchen. Alles abzufragen waere ein halbes Megabyte;
// der Pfad allein ist schon gross genug fuer eine Mobilfunkleitung.
const char *FELDER =
    "id,username,name,streak,totalXp,health,fromLanguage,learningLanguage,"
    "courses,currentCourse%7Bid,title,fromLanguage,learningLanguage,xp,"
    "healthEnabled,pathSectioned%7D";

const char *ANMELDEN = "anmelden";
const char *KONTO = "konto";
const char *PASSWORTPOST = "passwortpost";
const char *NACHSCHAU = "nachschau";
const char *WECHSELN = "kurswechsel";
const char *ANGEBOTE = "angebote";

}  // namespace

Konto::Konto(Netz *netz, QObject *eltern)
    : QObject(eltern), m_netz(netz), m_laeuft(false), m_straehne(0), m_xp(0),
      m_herzen(5), m_herzenZaehlen(false)
{
    connect(m_netz, SIGNAL(antwort(QString,QVariantMap)),
            this, SLOT(antwort(QString,QVariantMap)));
    connect(m_netz, SIGNAL(fehler(QString,int,QString)),
            this, SLOT(fehler(QString,int,QString)));
    connect(m_netz, SIGNAL(anmeldungGeaendert()), this, SIGNAL(geaendert()));
}

bool Konto::angemeldet() const
{
    return m_netz->angemeldet();
}

QString Konto::kursName() const
{
    const QString titel = m_kurs.value(QLatin1String("title")).toString();
    if (!titel.isEmpty())
        return titel;
    // Die Schnittstelle laesst den Titel oft leer -- dann selbst setzen.
    if (m_lernsprache.isEmpty())
        return QString();
    return spracheName(m_lernsprache);
}

QString Konto::spracheName(const QString &kuerzel)
{
    static const char *tabelle[][2] = {
        {"es", QT_TR_NOOP("Spanisch")},   {"fr", QT_TR_NOOP("Franzoesisch")},
        {"de", QT_TR_NOOP("Deutsch")},    {"en", QT_TR_NOOP("Englisch")},
        {"it", QT_TR_NOOP("Italienisch")},{"pt", QT_TR_NOOP("Portugiesisch")},
        {"nl", QT_TR_NOOP("Niederlaendisch")}, {"sv", QT_TR_NOOP("Schwedisch")},
        {"da", QT_TR_NOOP("Daenisch")},   {"nb", QT_TR_NOOP("Norwegisch")},
        {"pl", QT_TR_NOOP("Polnisch")},   {"ru", QT_TR_NOOP("Russisch")},
        {"uk", QT_TR_NOOP("Ukrainisch")}, {"cs", QT_TR_NOOP("Tschechisch")},
        {"tr", QT_TR_NOOP("Tuerkisch")},  {"el", QT_TR_NOOP("Griechisch")},
        {"hu", QT_TR_NOOP("Ungarisch")},  {"ro", QT_TR_NOOP("Rumaenisch")},
        {"ja", QT_TR_NOOP("Japanisch")},  {"ko", QT_TR_NOOP("Koreanisch")},
        {"zh", QT_TR_NOOP("Chinesisch")}, {"ar", QT_TR_NOOP("Arabisch")},
        {"he", QT_TR_NOOP("Hebraeisch")}, {"hi", QT_TR_NOOP("Hindi")},
        {"eo", QT_TR_NOOP("Esperanto")},  {"la", QT_TR_NOOP("Latein")},
        {"ga", QT_TR_NOOP("Irisch")},     {"cy", QT_TR_NOOP("Walisisch")},
        {"fi", QT_TR_NOOP("Finnisch")},   {"vi", QT_TR_NOOP("Vietnamesisch")},
        {0, 0}
    };
    for (int i = 0; tabelle[i][0]; ++i)
        if (kuerzel == QLatin1String(tabelle[i][0]))
            return tr(tabelle[i][1]);
    return kuerzel.toUpper();
}

void Konto::setzeLaeuft(bool wert)
{
    if (m_laeuft == wert)
        return;
    m_laeuft = wert;
    emit laeuftGeaendert();
}

void Konto::anmelden(const QString &kennung, const QString &passwort)
{
    QVariantMap koerper;
    koerper.insert(QLatin1String("identifier"), kennung);
    koerper.insert(QLatin1String("password"), passwort);
    koerper.insert(QLatin1String("distinctId"),
                   QUuid::createUuid().toString().remove(QLatin1Char('{'))
                       .remove(QLatin1Char('}')));
    setzeLaeuft(true);
    m_zuletztVersucht = kennung;
    // Der Weg, den die Weboberflaeche heute geht. Den alten
    // (/2017-06-30/login) gibt es noch, aber wenn sich etwas aendert,
    // aendert es sich dort zuerst.
    m_netz->ruf(QLatin1String(ANMELDEN), QLatin1String("POST"),
                QLatin1String("/2023-05-23/login?fields=jwt"),
                Json::serialise(koerper), false);
}

void Konto::passwortAnfordern(const QString &adresse)
{
    QVariantMap koerper;
    koerper.insert(QLatin1String("email"), adresse);
    setzeLaeuft(true);
    m_netz->ruf(QLatin1String(PASSWORTPOST), QLatin1String("POST"),
                QLatin1String("/2023-05-23/password-reset"),
                Json::serialise(koerper), false);
}

bool Konto::markeEinspielen(const QString &jwt)
{
    if (!m_netz->einspielen(jwt))
        return false;
    emit angemeldetWorden();
    auffrischen();
    return true;
}

void Konto::abmelden()
{
    m_netz->abmelden();
    m_name.clear();
    m_straehne = 0;
    m_xp = 0;
    m_kurs.clear();
    m_pfad.clear();
    m_stufen.clear();
    emit geaendert();
}

void Konto::auffrischen()
{
    if (!angemeldet())
        return;
    setzeLaeuft(true);
    // Die neue Basis, nicht die alte: bei den Faechern ohne Sprache
    // (Schach, Rechnen, Musik) gibt `/2017-06-30/users/{id}` fuer
    // `currentCourse` **null** zurueck, `/2023-05-23` den ganzen Pfad.
    // Bei den Sprachkursen antworten beide gleich (nachgemessen).
    m_netz->ruf(QLatin1String(KONTO), QLatin1String("GET"),
                QString::fromLatin1("/2023-05-23/users/%1?fields=%2")
                    .arg(m_netz->benutzerId(), QLatin1String(FELDER)));
}

void Konto::kursWechseln(const QString &kursId)
{
    if (kursId.isEmpty() || !angemeldet())
        return;
    // Genau das, was die Weboberflaeche schickt: die Kennung des Kurses
    // und die Muttersprache. Mit einer Kennung, die man noch nicht belegt
    // hat, faengt derselbe Aufruf den Kurs neu an.
    QVariantMap koerper;
    koerper.insert(QLatin1String("currentCourseId"), kursId);
    koerper.insert(QLatin1String("fromLanguage"),
                   m_muttersprache.isEmpty() ? QLatin1String("en")
                                             : m_muttersprache);
    setzeLaeuft(true);
    // Zusammengesetzt statt mit arg(): in einer Vorlage fuer arg() ist
    // **`%7B` schon eine Platzhalternummer** (`%7`), und ein verdoppeltes
    // `%%` entschaerft bei Qt gar nichts -- es blieb woertlich in der
    // Adresse stehen und der Server antwortete mit 400.
    m_netz->ruf(QLatin1String(WECHSELN), QLatin1String("PATCH"),
                QLatin1String("/2023-05-23/users/") + m_netz->benutzerId()
                    + QLatin1String("?fields=currentCourse%7Bid%7D"),
                Json::serialise(koerper));
}

void Konto::angeboteHolen()
{
    if (!m_angebote.isEmpty())
        return;
    m_netz->ruf(QLatin1String(ANGEBOTE), QLatin1String("GET"),
                QLatin1String("/2017-06-30/config"), QByteArray(), false);
}

void Konto::kurseLesen(const QVariantList &rohe)
{
    m_kurse.clear();
    const QString laufend = m_kurs.value(QLatin1String("id")).toString();
    for (int i = 0; i < rohe.size(); ++i) {
        const QVariantMap k = rohe.at(i).toMap();
        QVariantMap zeile;
        const QString kennung = k.value(QLatin1String("id")).toString();
        zeile.insert(QLatin1String("id"), kennung);
        QString titel = k.value(QLatin1String("title")).toString();
        if (titel.isEmpty())
            titel = spracheName(k.value(QLatin1String("learningLanguage")).toString());
        zeile.insert(QLatin1String("titel"), titel);
        zeile.insert(QLatin1String("sprache"),
                     k.value(QLatin1String("learningLanguage")));
        zeile.insert(QLatin1String("xp"), k.value(QLatin1String("xp")).toInt());
        zeile.insert(QLatin1String("laufend"), kennung == laufend);
        m_kurse.append(zeile);
    }
}

void Konto::pfadLesen(const QVariantMap &kurs)
{
    m_pfad.clear();
    m_stufen.clear();
    const QVariantList abschnitte =
        kurs.value(QLatin1String("pathSectioned")).toList();
    for (int a = 0; a < abschnitte.size(); ++a) {
        const QVariantMap abschnitt = abschnitte.at(a).toMap();
        const QVariantList einheiten =
            abschnitt.value(QLatin1String("units")).toList();
        for (int e = 0; e < einheiten.size(); ++e) {
            const QVariantMap einheit = einheiten.at(e).toMap();
            const QVariantList stufen =
                einheit.value(QLatin1String("levels")).toList();
            for (int s = 0; s < stufen.size(); ++s) {
                const QVariantMap stufe = stufen.at(s).toMap();
                const QVariantMap daten =
                    stufe.value(QLatin1String("pathLevelClientData")).toMap();
                const QString art = stufe.value(QLatin1String("type")).toString();
                const QString zustand =
                    stufe.value(QLatin1String("state")).toString();

                QVariantMap zeile;
                zeile.insert(QLatin1String("nummer"), m_stufen.size());
                zeile.insert(QLatin1String("einheit"),
                             einheit.value(QLatin1String("unitIndex")).toInt() + 1);
                zeile.insert(QLatin1String("abschnitt"),
                             abschnitt.value(QLatin1String("index")).toInt() + 1);
                QString titel = daten.value(QLatin1String("teachingObjective")).toString();
                if (titel.isEmpty())
                    titel = einheit.value(QLatin1String("teachingObjective")).toString();
                if (titel.isEmpty())
                    titel = stufe.value(QLatin1String("debugName")).toString();
                zeile.insert(QLatin1String("titel"), titel);
                zeile.insert(QLatin1String("art"), art);
                zeile.insert(QLatin1String("zustand"), zustand);
                // Eine gesperrte Einheit traegt den Zustand `unit_test`:
                // dort bietet Duolingo den Test an, mit dem man sie
                // ueberspringt. Das ist die Einstufung mitten im Pfad --
                // 21 solche Stellen allein im Lateinkurs.
                const bool pruefung = zustand == QLatin1String("unit_test");
                zeile.insert(QLatin1String("pruefung"), pruefung);
                zeile.insert(QLatin1String("offen"),
                             pruefung
                                 || zustand == QLatin1String("active")
                                 || zustand == QLatin1String("unlocked"));
                zeile.insert(QLatin1String("fertig"),
                             stufe.value(QLatin1String("finishedSessions")).toInt());
                zeile.insert(QLatin1String("gesamt"),
                             stufe.value(QLatin1String("totalSessions")).toInt());
                // Was diese Fassung zeichnen kann. Geschichten, Truhen und
                // Rechnen bleiben in der Liste sichtbar, aber grau.
                const bool schach = art.startsWith(QLatin1String("chess"));
                const bool rechnen = art.startsWith(QLatin1String("math"));
                zeile.insert(QLatin1String("fach"),
                             schach ? QLatin1String("schach")
                                    : (rechnen ? QLatin1String("rechnen")
                                               : QLatin1String("sprache")));
                zeile.insert(QLatin1String("machbar"),
                             art == QLatin1String("skill")
                                 || art == QLatin1String("practice")
                                 || art == QLatin1String("unit_review")
                                 || art == QLatin1String("chess_skill")
                                 || art == QLatin1String("chess_practice")
                                 || art == QLatin1String("chess_puzzles")
                                 || art == QLatin1String("chess_unit_review")
                                 || art == QLatin1String("chess_match")
                                 || art == QLatin1String("math_skill")
                                 || art == QLatin1String("math_match_practice")
                                 || art == QLatin1String("math_unit_review"));
                m_pfad.append(zeile);
                m_stufen.append(stufe);
            }
        }
    }
}

QVariantMap Konto::stufe(int nummer) const
{
    if (nummer < 0 || nummer >= m_stufen.size())
        return QVariantMap();
    return m_stufen.at(nummer);
}

int Konto::naechsteStufe() const
{
    for (int i = 0; i < m_pfad.size(); ++i) {
        const QVariantMap zeile = m_pfad.at(i).toMap();
        if (zeile.value(QLatin1String("offen")).toBool()
            && zeile.value(QLatin1String("machbar")).toBool())
            return i;
    }
    return -1;
}

void Konto::antwort(const QString &kennung, const QVariantMap &daten)
{
    if (kennung == QLatin1String(ANMELDEN)) {
        setzeLaeuft(false);
        // Die Marke hat Netz schon aus dem Antwortkopf genommen.
        if (angemeldet()) {
            emit angemeldetWorden();
            auffrischen();
        } else {
            emit anmeldungAbgelehnt(tr("Der Server hat keine Marke geschickt."));
        }
        return;
    }
    if (kennung == QLatin1String(PASSWORTPOST)) {
        setzeLaeuft(false);
        emit postUnterwegs();
        return;
    }
    if (kennung == QLatin1String(WECHSELN)) {
        setzeLaeuft(false);
        emit kursGewechselt();
        auffrischen();
        return;
    }
    if (kennung == QLatin1String(ANGEBOTE)) {
        // /config nennt zu jeder Muttersprache die lernbaren Sprachen.
        const QVariantMap alle = daten.value(QLatin1String("courses")).toMap();
        const QString meine = m_muttersprache.isEmpty()
                                  ? QLatin1String("en") : m_muttersprache;
        const QVariantList sprachen = alle.value(meine).toList();
        m_angebote.clear();
        // Die drei Faecher ohne Muttersprache stehen nicht in /config --
        // ihre Kennungen sind im Web-Buendel fest verdrahtet.
        static const char *faecher[][2] = {
            {"CHESS_CH", QT_TR_NOOP("Schach")},
            {"MATH_BT", QT_TR_NOOP("Rechnen")},
            {"MUSIC_MT", QT_TR_NOOP("Musik")},
            {0, 0}
        };
        for (int i = 0; faecher[i][0]; ++i) {
            QVariantMap zeile;
            zeile.insert(QLatin1String("sprache"), QString());
            zeile.insert(QLatin1String("titel"), tr(faecher[i][1]));
            zeile.insert(QLatin1String("id"), QLatin1String(faecher[i][0]));
            m_angebote.append(zeile);
        }
        for (int i = 0; i < sprachen.size(); ++i) {
            const QString kuerzel = sprachen.at(i).toString();
            QVariantMap zeile;
            zeile.insert(QLatin1String("sprache"), kuerzel);
            zeile.insert(QLatin1String("titel"), spracheName(kuerzel));
            zeile.insert(QLatin1String("id"),
                         QString::fromLatin1("DUOLINGO_%1_%2")
                             .arg(kuerzel.toUpper(), meine.toUpper()));
            m_angebote.append(zeile);
        }
        emit angeboteGeaendert();
        return;
    }
    if (kennung == QLatin1String(NACHSCHAU)) {
        setzeLaeuft(false);
        const QVariantList treffer = daten.value(QLatin1String("users")).toList();
        if (treffer.isEmpty()) {
            emit anmeldungAbgelehnt(
                tr("Zu %1 gibt es kein Konto. Vertippt?").arg(m_zuletztVersucht));
            return;
        }
        const QVariantMap u = treffer.first().toMap();
        if (u.value(QLatin1String("hasGoogleId")).toBool()
            || u.value(QLatin1String("hasFacebookId")).toBool()) {
            emit anmeldungAbgelehnt(
                tr("Das Konto gibt es, es haengt aber an Google oder "
                   "Facebook. Wenn du noch kein Passwort gesetzt hast: "
                   "unten \"Mit Google angemeldet?\" und die Mail "
                   "anfordern."));
            return;
        }
        emit anmeldungAbgelehnt(tr("Das Konto gibt es -- dann stimmt das "
                                   "Passwort nicht."));
        return;
    }
    if (kennung != QLatin1String(KONTO))
        return;

    setzeLaeuft(false);
    m_name = daten.value(QLatin1String("name")).toString();
    if (m_name.isEmpty())
        m_name = daten.value(QLatin1String("username")).toString();
    m_straehne = daten.value(QLatin1String("streak")).toInt();
    m_xp = daten.value(QLatin1String("totalXp")).toInt();
    m_lernsprache = daten.value(QLatin1String("learningLanguage")).toString();
    m_muttersprache = daten.value(QLatin1String("fromLanguage")).toString();

    const QVariantMap gesundheit = daten.value(QLatin1String("health")).toMap();
    if (!gesundheit.isEmpty()) {
        m_herzen = gesundheit.value(QLatin1String("hearts"),
                                    gesundheit.value(QLatin1String("health"), 5)).toInt();
        m_herzenZaehlen =
            gesundheit.value(QLatin1String("useHealth"), false).toBool();
    }

    m_kurs = daten.value(QLatin1String("currentCourse")).toMap();
    if (m_lernsprache.isEmpty())
        m_lernsprache = m_kurs.value(QLatin1String("learningLanguage")).toString();
    if (m_muttersprache.isEmpty())
        m_muttersprache = m_kurs.value(QLatin1String("fromLanguage")).toString();
    if (m_kurs.value(QLatin1String("healthEnabled")).toBool())
        m_herzenZaehlen = true;
    pfadLesen(m_kurs);
    kurseLesen(daten.value(QLatin1String("courses")).toList());
    emit geaendert();
}

void Konto::fehler(const QString &kennung, int status, const QString &text)
{
    if (kennung == QLatin1String(NACHSCHAU)) {
        setzeLaeuft(false);
        emit anmeldungAbgelehnt(tr("Kennung oder Passwort stimmen nicht."));
        return;
    }
    if (kennung == QLatin1String(WECHSELN)) {
        setzeLaeuft(false);
        emit fehlgeschlagen(tr("Der Kurs liess sich nicht wechseln: %1").arg(text));
        return;
    }
    if (kennung == QLatin1String(ANGEBOTE))
        return;
    if (kennung != QLatin1String(ANMELDEN) && kennung != QLatin1String(KONTO)
        && kennung != QLatin1String(PASSWORTPOST))
        return;
    setzeLaeuft(false);
    if (kennung == QLatin1String(PASSWORTPOST)) {
        emit fehlgeschlagen(text);
        return;
    }
    if (kennung == QLatin1String(ANMELDEN)) {
        if (status != 401 && status != 403) {
            // Alles andere ist nicht die Kennung: Netz weg, Dienst
            // ueberlastet, zu viele Versuche. Dann den Satz des Servers
            // zeigen und nicht stattdessen etwas ueber das Passwort sagen.
            emit anmeldungAbgelehnt(text);
            return;
        }
        // Duolingo antwortet auf jede misslungene Anmeldung mit demselben
        // Satz -- ob die Kennung nicht existiert, das Passwort falsch ist
        // oder gar keines gesetzt wurde (Konto per Google angelegt), steht
        // nicht darin. Also einmal nachsehen, was davon es ist.
        if (!m_zuletztVersucht.isEmpty()) {
            setzeLaeuft(true);
            const QString feld = m_zuletztVersucht.contains(QLatin1Char('@'))
                                     ? QLatin1String("email")
                                     : QLatin1String("username");
            QString wert = m_zuletztVersucht;
            wert.replace(QLatin1String("@"), QLatin1String("%40"));
            m_netz->ruf(QLatin1String(NACHSCHAU), QLatin1String("GET"),
                        QLatin1String("/2017-06-30/users?") + feld
                            + QLatin1Char('=') + wert
                            + QLatin1String("&fields=users%7Bid,username,"
                                            "hasGoogleId,hasFacebookId%7D"),
                        QByteArray(), false);
            return;
        }
        emit anmeldungAbgelehnt(tr("Kennung oder Passwort stimmen nicht."));
        return;
    }
    emit fehlgeschlagen(text);
}
