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
    "currentCourse%7Bid,title,fromLanguage,learningLanguage,xp,healthEnabled,"
    "pathSectioned%7D";

const char *ANMELDEN = "anmelden";
const char *KONTO = "konto";

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
    m_netz->ruf(QLatin1String(ANMELDEN), QLatin1String("POST"),
                QLatin1String("/2017-06-30/login?fields=jwt"),
                Json::serialise(koerper), false);
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
    m_netz->ruf(QLatin1String(KONTO), QLatin1String("GET"),
                QString::fromLatin1("/2017-06-30/users/%1?fields=%2")
                    .arg(m_netz->benutzerId(), QLatin1String(FELDER)));
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
                zeile.insert(QLatin1String("offen"),
                             zustand == QLatin1String("active")
                                 || zustand == QLatin1String("unlocked"));
                zeile.insert(QLatin1String("fertig"),
                             stufe.value(QLatin1String("finishedSessions")).toInt());
                zeile.insert(QLatin1String("gesamt"),
                             stufe.value(QLatin1String("totalSessions")).toInt());
                // Was diese Fassung zeichnen kann. Geschichten, Schach und
                // Rechnen bleiben in der Liste sichtbar, aber grau.
                zeile.insert(QLatin1String("machbar"),
                             art == QLatin1String("skill")
                                 || art == QLatin1String("practice")
                                 || art == QLatin1String("unit_review"));
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
    emit geaendert();
}

void Konto::fehler(const QString &kennung, int status, const QString &text)
{
    if (kennung != QLatin1String(ANMELDEN) && kennung != QLatin1String(KONTO))
        return;
    setzeLaeuft(false);
    if (kennung == QLatin1String(ANMELDEN)) {
        if (status == 401 || status == 403)
            emit anmeldungAbgelehnt(
                tr("Kennung oder Passwort stimmen nicht."));
        else
            emit anmeldungAbgelehnt(text);
        return;
    }
    emit fehlgeschlagen(text);
}
