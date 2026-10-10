#include "lektion.h"
#include "benoter.h"
#include "rechner.h"
#include "json.h"
#include "konto.h"
#include "netz.h"

#include <QDateTime>
#include <QStringList>

namespace {

const char *HOLEN = "sitzung-holen";
const char *ABGEBEN = "sitzung-abgeben";

// Die Aufgabenarten, die diese Oberflaeche zeichnen kann. Was hier nicht
// steht, schickt der Server auch nicht -- `challengeTypes` erklaert der
// Client selbst (doc/BEFUND.md). Draussen bleiben alle Arten mit Mikrofon,
// Zeichenbrett, Schach und Rechnen.
const char *ARTEN[] = {
    "assist", "characterIntro", "characterMatch", "characterPuzzle",
    "characterSelect", "completeReverseTranslation", "definition", "dialogue",
    "extendedListenMatch", "extendedMatch", "form", "freeResponse", "gapFill",
    "judge", "listen", "listenComplete", "listenComprehension",
    "listenIsolation", "listenMatch", "listenTap", "match", "name",
    "partialListen", "partialReverseTranslate", "readComprehension",
    "reverseAssist", "sameDifferent", "select", "selectPronunciation",
    "selectTranscription", "syllableTap", "syllableListenTap", "tapCloze",
    "tapComplete", "tapDescribe", "translate", "transliterate",
    "transliterationAssist", "typeCloze", "typeComplete", "writeComprehension",
    // Die Rechenarten. Die Weboberflaeche haengt sie an jede Anforderung
    // an; in einem Sprachkurs kommen sie einfach nicht vor.
    "mathProductSelect", "mathMatch", "mathMultiSelect", "mathPatternTable",
    "mathTypeFill", "mathIntegerNumberLineFill", "mathEstimateNumberLine",
    "mathExpressionBuild", "mathFactorTree", "mathFractionFill",
    0
};

bool darunter(const char **liste, const QString &was)
{
    for (int i = 0; liste[i]; ++i)
        if (was == QLatin1String(liste[i]))
            return true;
    return false;
}

// Antippen statt Eintippen: Arten, die eine Reihe von Wortfeldern zeigen.
const char *WOERTER[] = {
    "translate", "listenTap", "tapComplete", "completeReverseTranslation",
    "syllableTap", "syllableListenTap", "tapCloze", "partialReverseTranslate",
    "tapDescribe", "characterPuzzle", 0
};

// Zwei Spalten zusammenbringen.
const char *PAARE[] = {
    "match", "listenMatch", "characterMatch", "extendedMatch",
    "extendedListenMatch", 0
};

// Frei eintippen -- diese Arten bringen keine Wahlfelder mit.
const char *TIPPEN[] = {
    "name", "listen", "listenComplete", "typeComplete", "typeCloze",
    "partialListen", "writeComprehension", "transliterate", "freeResponse", 0
};

// Die Beschriftung eines Wahlfeldes. **Sie heisst nicht ueberall `text`:**
// `select` (das Bildraetsel, mit dem jede erste Lektion anfaengt) nennt sie
// `phrase`, die Zeichenaufgaben `character`. Wer nur `text` liest, zeigt
// leere Kaestchen -- genau das war in der ersten Lateinlektion zu sehen.
QString textVon(const QVariant &wahl)
{
    if (wahl.type() != QVariant::Map)
        return wahl.toString();
    const QVariantMap m = wahl.toMap();
    static const char *namen[] = { "text", "phrase", "character", "word",
                                   "transliteration", 0 };
    for (int i = 0; namen[i]; ++i) {
        const QString wert = m.value(QLatin1String(namen[i])).toString();
        if (!wert.isEmpty())
            return wert;
    }
    return QString();
}

// Aus der Bildadresse die PNG-Fassung machen. Was die Aufgaben nennen, ist
// SVG (und bei den Figuren sogar PDF) -- Qt 4.7 zeichnet davon nichts.
// Dieselbe Kennung unter `/images/png/` ist ein richtiges PNG (gemessen:
// 200 image/png, auch ueber http).
QVariant bildAdresse(const QVariant &url)
{
    QString s = url.toString();
    if (s.isEmpty())
        return QVariant(QString());
    if (s.startsWith(QLatin1String("https://")))
        s.replace(0, 5, QLatin1String("http"));
    if (s.contains(QLatin1String("/images/"))
        && !s.contains(QLatin1String("/images/png/")))
        s.replace(QLatin1String("/images/"), QLatin1String("/images/png/"));
    return s;
}

// Der Ton laeuft ueber gstreamer, und **gstreamer auf Harmattan kann kein
// TLS**. Duolingos Tonwolke gibt dieselben Dateien auch ueber http heraus
// (gemessen: 200 audio/mpeg, gleiche Groesse) -- also wird die Adresse
// umgeschrieben statt ein Vermittler gebaut.
QVariant tonAdresse(const QVariant &url)
{
    QString s = url.toString();
    if (s.startsWith(QLatin1String("https://")))
        s.replace(0, 5, QLatin1String("http"));
    return s.isEmpty() ? QVariant(QString()) : QVariant(s);
}

qint64 jetztMs()
{
#if QT_VERSION >= 0x040700
    return QDateTime::currentMSecsSinceEpoch();
#else
    return (qint64) QDateTime::currentDateTime().toTime_t() * 1000;
#endif
}

}  // namespace

Lektion::Lektion(Netz *netz, Konto *konto, QObject *eltern)
    : QObject(eltern), m_netz(netz), m_konto(konto), m_laeuft(false),
      m_nummer(0), m_richtige(0), m_strecke(0), m_besteStrecke(0),
      m_herzen(5), m_begonnen(0), m_aufgabeBegonnen(0)
{
    connect(m_netz, SIGNAL(antwort(QString,QVariantMap)),
            this, SLOT(antwort(QString,QVariantMap)));
    connect(m_netz, SIGNAL(fehler(QString,int,QString)),
            this, SLOT(fehler(QString,int,QString)));
}

void Lektion::setzeLaeuft(bool wert)
{
    if (m_laeuft == wert)
        return;
    m_laeuft = wert;
    emit laeuftGeaendert();
}

void Lektion::uebenStarten()
{
    starten(QLatin1String("GLOBAL_PRACTICE"), -1);
}

void Lektion::stufeStarten(int stufennummer)
{
    starten(QLatin1String("LESSON"), stufennummer);
}

void Lektion::starten(const QString &art, int stufennummer)
{
    const QVariantMap kurs = m_konto->kurs();
    if (kurs.isEmpty()) {
        emit fehlgeschlagen(tr("Der Kurs ist noch nicht geladen."));
        return;
    }

    QVariantList arten;
    for (int i = 0; ARTEN[i]; ++i)
        arten.append(QLatin1String(ARTEN[i]));

    QVariantMap koerper;
    koerper.insert(QLatin1String("challengeTypes"), arten);
    koerper.insert(QLatin1String("fromLanguage"),
                   kurs.value(QLatin1String("fromLanguage")));
    koerper.insert(QLatin1String("learningLanguage"),
                   kurs.value(QLatin1String("learningLanguage")));
    koerper.insert(QLatin1String("isFinalLevel"), false);
    koerper.insert(QLatin1String("isRedoingPassedNode"), false);
    koerper.insert(QLatin1String("isV2"), true);
    koerper.insert(QLatin1String("juicy"), true);
    koerper.insert(QLatin1String("smartTipsVersion"), 2);
    koerper.insert(QLatin1String("type"), art);

    if (stufennummer >= 0) {
        // Abgeschrieben aus der Weboberflaeche: ohne `levelId`,
        // `levelSessionIndex` und `pathLevelSessionMetadata` antwortet der
        // Server mit 500. Siehe tools/lektion-koerper-lesen.py.
        const QVariantMap stufe = m_konto->stufe(stufennummer);
        if (stufe.isEmpty()) {
            emit fehlgeschlagen(tr("Diese Stufe gibt es nicht."));
            return;
        }
        const QVariantMap daten =
            stufe.value(QLatin1String("pathLevelClientData")).toMap();
        const QVariantMap beiwerk =
            stufe.value(QLatin1String("pathLevelMetadata")).toMap();
        if (beiwerk.contains(QLatin1String("treeId")))
            koerper.insert(QLatin1String("treeId"),
                           beiwerk.value(QLatin1String("treeId")));
        koerper.insert(QLatin1String("levelId"), stufe.value(QLatin1String("id")));
        koerper.insert(QLatin1String("levelSessionIndex"),
                       stufe.value(QLatin1String("finishedSessions")).toInt());
        koerper.insert(QLatin1String("pathLevelSessionMetadata"),
                       stufe.value(QLatin1String("pathLevelSessionMetadata")));

        const QString stufenart = stufe.value(QLatin1String("type")).toString();
        const QString zustand = stufe.value(QLatin1String("state")).toString();
        if (zustand == QLatin1String("unit_test")) {
            // Der Test, mit dem sich eine gesperrte Einheit ueberspringen
            // laesst. Er braucht nur die Faehigkeiten der Einheit; die
            // Stufenart darunter ist egal.
            koerper.insert(QLatin1String("skillIds"),
                           daten.value(QLatin1String("skillIds")));
            koerper.insert(QLatin1String("type"), QLatin1String("UNIT_TEST"));
        } else if (stufenart == QLatin1String("skill")) {
            koerper.insert(QLatin1String("skillId"),
                           daten.value(QLatin1String("skillId")));
            koerper.insert(QLatin1String("levelIndex"),
                           daten.value(QLatin1String("crownLevelIndex"), 0).toInt());
            koerper.insert(QLatin1String("isCustomIntroSkill"),
                           stufe.value(QLatin1String("subtype")).toString()
                               == QLatin1String("custom_intro"));
            koerper.insert(QLatin1String("isGrammarSkill"),
                           stufe.value(QLatin1String("subtype")).toString()
                               == QLatin1String("grammar"));
            koerper.insert(QLatin1String("showGrammarSkillSplash"), false);
        } else if (stufenart.startsWith(QLatin1String("math"))) {
            // Rechnen: die Stufe bringt ihre Sitzungsbeschreibung mit,
            // `metadata` ist Pflicht (ohne sie: 500).
            const QVariantList beiwerke =
                daten.value(QLatin1String("sessionMetadatas")).toList();
            const int fertig =
                stufe.value(QLatin1String("finishedSessions")).toInt();
            if (!beiwerke.isEmpty())
                koerper.insert(QLatin1String("metadata"),
                               beiwerke.at(fertig < beiwerke.size() ? fertig : 0));
            koerper.insert(QLatin1String("skillId"),
                           daten.value(QLatin1String("skillId")));
            koerper.insert(QLatin1String("levelIndex"),
                           daten.value(QLatin1String("crownLevelIndex"), 0).toInt());
            koerper.insert(QLatin1String("type"), QLatin1String("MATH_LESSON"));
        } else if (stufenart == QLatin1String("practice")) {
            koerper.insert(QLatin1String("skillIds"),
                           daten.value(QLatin1String("skillIds")));
            koerper.insert(QLatin1String("lexemePracticeType"),
                           QLatin1String("practice_level"));
            koerper.insert(QLatin1String("type"), QLatin1String("LEXEME_PRACTICE"));
        } else if (stufenart == QLatin1String("unit_review")) {
            koerper.insert(QLatin1String("skillIds"),
                           daten.value(QLatin1String("skillIds")));
            koerper.insert(QLatin1String("type"), QLatin1String("UNIT_REVIEW"));
        }
    }

    m_sitzung.clear();
    m_aufgaben.clear();
    m_aufgabe.clear();
    m_nummer = 0;
    m_richtige = 0;
    m_strecke = 0;
    m_besteStrecke = 0;
    m_herzen = m_konto->herzenZaehlen() ? m_konto->herzen() : 5;
    setzeLaeuft(true);
    emit geaendert();
    m_netz->ruf(QLatin1String(HOLEN), QLatin1String("POST"),
                QLatin1String("/2023-05-23/sessions"), Json::serialise(koerper));
}

QVariantMap Lektion::darstellen(const QVariantMap &roh) const
{
    QVariantMap k;
    const QString art = roh.value(QLatin1String("type")).toString();
    k.insert(QLatin1String("art"), art);

    // Rechnen hat mit Saetzen nichts zu tun: eigene Zerlegung, eigene
    // Benotung (src/rechner.h).
    if (Rechner::istRechnen(roh)) {
        QVariantMap r = Rechner::darstellen(roh);
        r.insert(QLatin1String("art"), art);
        r.insert(QLatin1String("darstellung"), QLatin1String("rechnen"));
        return r;
    }
    k.insert(QLatin1String("frage"), roh.value(QLatin1String("prompt")));
    k.insert(QLatin1String("uebersetzung"),
             roh.value(QLatin1String("solutionTranslation")));
    k.insert(QLatin1String("ton"), tonAdresse(roh.value(QLatin1String("tts"))));
    k.insert(QLatin1String("tonLangsam"),
             tonAdresse(roh.value(QLatin1String("slowTts"))));
    k.insert(QLatin1String("bild"),
             bildAdresse(roh.value(QLatin1String("pngUrl"))));
    k.insert(QLatin1String("loesungTon"),
             tonAdresse(roh.value(QLatin1String("solutionTts"))));

    if (darunter(PAARE, art)) {
        k.insert(QLatin1String("darstellung"), QLatin1String("paare"));
        QVariantList links, rechts;
        const QVariantList paare = roh.value(QLatin1String("pairs")).toList();
        for (int i = 0; i < paare.size(); ++i) {
            const QVariantMap p = paare.at(i).toMap();
            QVariantMap l;
            l.insert(QLatin1String("text"),
                     p.value(QLatin1String("learningWord"),
                             p.value(QLatin1String("character"))));
            l.insert(QLatin1String("ton"), tonAdresse(p.value(QLatin1String("tts"))));
            l.insert(QLatin1String("paar"), i);
            QVariantMap r;
            r.insert(QLatin1String("text"),
                     p.value(QLatin1String("translation"),
                             p.value(QLatin1String("transliteration"))));
            r.insert(QLatin1String("paar"), i);
            links.append(l);
            rechts.append(r);
        }
        k.insert(QLatin1String("links"), links);
        k.insert(QLatin1String("rechts"), rechts);
        return k;
    }

    // Die Wahlfelder, einheitlich: Text, Ton, Bild.
    QVariantList wahlen;
    const QVariantList rohWahlen = roh.value(QLatin1String("choices")).toList();
    const QVariantList rohOptionen = roh.value(QLatin1String("options")).toList();
    const QVariantList quelle = rohWahlen.isEmpty() ? rohOptionen : rohWahlen;
    for (int i = 0; i < quelle.size(); ++i) {
        const QVariant eintrag = quelle.at(i);
        QVariantMap w;
        w.insert(QLatin1String("text"), textVon(eintrag));
        if (eintrag.type() == QVariant::Map) {
            const QVariantMap m = eintrag.toMap();
            w.insert(QLatin1String("ton"), tonAdresse(m.value(QLatin1String("tts"))));
            w.insert(QLatin1String("bild"),
                     bildAdresse(m.value(QLatin1String("image"),
                                         m.value(QLatin1String("svg")))));
            w.insert(QLatin1String("umschrift"),
                     m.value(QLatin1String("transliteration")));
        }
        // Bei `assist` und Verwandten steht der Ton in `options`, der Text
        // in `choices` -- dieselbe Reihenfolge, darum zusammenlegen.
        if (!rohWahlen.isEmpty() && i < rohOptionen.size()
            && w.value(QLatin1String("ton")).toString().isEmpty()) {
            w.insert(QLatin1String("ton"), tonAdresse(
                         rohOptionen.at(i).toMap().value(QLatin1String("tts"))));
        }
        w.insert(QLatin1String("nummer"), i);
        wahlen.append(w);
    }
    k.insert(QLatin1String("wahlen"), wahlen);

    if (darunter(WOERTER, art) && !wahlen.isEmpty()) {
        k.insert(QLatin1String("darstellung"), QLatin1String("woerter"));
        // Bei `translate` darf der Server das Eintippen verlangen.
        const QVariantMap anzeige =
            roh.value(QLatin1String("challengeDisplaySettings")).toMap();
        k.insert(QLatin1String("tippenMoeglich"),
                 anzeige.value(QLatin1String("showInputModeToggle"), false));
    } else if (darunter(TIPPEN, art) || wahlen.isEmpty()) {
        k.insert(QLatin1String("darstellung"), QLatin1String("tippen"));
    } else {
        k.insert(QLatin1String("darstellung"), QLatin1String("wahl"));
    }

    // Der Satz mit Luecke (listenComplete, typeCloze, gapFill): die Stuecke
    // stehen einzeln da, eines davon ist die Luecke.
    const QVariantList stuecke =
        roh.value(QLatin1String("displayTokens")).toList();
    if (!stuecke.isEmpty()) {
        QString satz;
        for (int i = 0; i < stuecke.size(); ++i) {
            const QVariantMap s = stuecke.at(i).toMap();
            if (s.value(QLatin1String("isBlank")).toBool())
                satz += QLatin1String("___");
            else
                satz += s.value(QLatin1String("text")).toString();
        }
        k.insert(QLatin1String("satz"), satz);
    }

    // Die Frage einer Leseaufgabe steht nicht in `prompt`.
    if (roh.contains(QLatin1String("question")))
        k.insert(QLatin1String("frage2"), roh.value(QLatin1String("question")));
    if (roh.contains(QLatin1String("passage")))
        k.insert(QLatin1String("text"), roh.value(QLatin1String("passage")));

    return k;
}

void Lektion::aufgabeZeigen()
{
    if (m_nummer < m_aufgaben.size())
        m_aufgabe = darstellen(m_aufgaben.at(m_nummer).toMap());
    else
        m_aufgabe.clear();
    m_aufgabeBegonnen = jetztMs();
    emit geaendert();
}

void Lektion::antworten(const QVariantList &reihe, const QString &text)
{
    if (m_nummer >= m_aufgaben.size())
        return;
    QVariantMap roh = m_aufgaben.at(m_nummer).toMap();

    QList<int> gedrueckt;
    for (int i = 0; i < reihe.size(); ++i)
        gedrueckt.append(reihe.at(i).toInt());

    // Der Text, der benotet wird: bei angetippten Woertern die Felder in
    // der angetippten Reihenfolge, sonst das Eingetippte.
    QString antwortText = text;
    if (!gedrueckt.isEmpty() && antwortText.isEmpty()) {
        const QVariantList wahlen = roh.value(QLatin1String("choices")).toList();
        QStringList teile;
        for (int i = 0; i < gedrueckt.size(); ++i) {
            const int k = gedrueckt.at(i);
            if (k >= 0 && k < wahlen.size())
                teile << textVon(wahlen.at(k));
        }
        antwortText = teile.join(QLatin1String(" "));
    }

    const bool richtig = Rechner::istRechnen(roh)
                             ? Rechner::benoten(roh, reihe, text)
                             : Benoter::benoten(roh, antwortText, gedrueckt);

    roh.insert(QLatin1String("correct"), richtig);
    roh.insert(QLatin1String("guess"), antwortText);
    roh.insert(QLatin1String("skipped"), false);
    roh.insert(QLatin1String("timeTaken"),
               (int) qMax((qint64) 1, jetztMs() - m_aufgabeBegonnen));
    QVariantMap ergebnis;
    ergebnis.insert(QLatin1String("correct"), richtig);
    ergebnis.insert(QLatin1String("guess"), antwortText);
    roh.insert(QLatin1String("gradingResult"), ergebnis);
    m_aufgaben[m_nummer] = roh;

    if (richtig) {
        ++m_richtige;
        ++m_strecke;
        if (m_strecke > m_besteStrecke)
            m_besteStrecke = m_strecke;
    } else {
        m_strecke = 0;
        if (m_konto->herzenZaehlen() && m_herzen > 0)
            --m_herzen;
    }
    emit geaendert();
    emit benotet(richtig, Rechner::istRechnen(roh)
                              ? Rechner::loesung(roh)
                              : Benoter::musterloesung(roh));
}

void Lektion::auslassen()
{
    if (m_nummer >= m_aufgaben.size())
        return;
    QVariantMap roh = m_aufgaben.at(m_nummer).toMap();
    roh.insert(QLatin1String("correct"), false);
    roh.insert(QLatin1String("guess"), QString());
    roh.insert(QLatin1String("skipped"), true);
    roh.insert(QLatin1String("timeTaken"),
               (int) qMax((qint64) 1, jetztMs() - m_aufgabeBegonnen));
    m_aufgaben[m_nummer] = roh;
    m_strecke = 0;
    emit geaendert();
    emit benotet(false, Benoter::musterloesung(roh));
}

void Lektion::weiter()
{
    if (m_nummer < m_aufgaben.size())
        ++m_nummer;
    aufgabeZeigen();
}

void Lektion::abbrechen()
{
    m_sitzung.clear();
    m_aufgaben.clear();
    m_aufgabe.clear();
    m_nummer = 0;
    emit geaendert();
}

void Lektion::abgeben()
{
    if (m_sitzung.isEmpty())
        return;

    QVariantMap sitzung = m_sitzung;
    sitzung.insert(QLatin1String("challenges"), m_aufgaben);

    // Die Felder, die die Weboberflaeche bei END_SESSION setzt. Die Zeiten
    // sind Sekunden mit Nachkomma, nicht Millisekunden.
    const qint64 jetzt = jetztMs();
    sitzung.insert(QLatin1String("startTime"),
                   (double) (m_begonnen ? m_begonnen : jetzt) / 1000.0);
    sitzung.insert(QLatin1String("endTime"), (double) jetzt / 1000.0);
    sitzung.insert(QLatin1String("enableBonusPoints"), false);
    sitzung.insert(QLatin1String("failed"),
                   m_konto->herzenZaehlen() && m_herzen <= 0);
    sitzung.insert(QLatin1String("heartsLeft"), m_herzen);
    sitzung.insert(QLatin1String("maxInLessonStreak"), m_besteStrecke);
    sitzung.insert(QLatin1String("shouldLearnThings"), true);
    // Was die Sitzung verspricht; was es wirklich gibt, sagt die Antwort.
    sitzung.insert(QLatin1String("xpPromised"), 10);

    setzeLaeuft(true);
    const QString kennung = m_sitzung.value(QLatin1String("id")).toString();
    m_netz->ruf(QLatin1String(ABGEBEN), QLatin1String("PUT"),
                QLatin1String("/2023-05-23/sessions/") + kennung,
                Json::serialise(sitzung), true, kennung);
}

void Lektion::antwort(const QString &kennung, const QVariantMap &daten)
{
    if (kennung == QLatin1String(HOLEN)) {
        setzeLaeuft(false);
        m_sitzung = daten;
        m_sitzung.remove(QLatin1String("__status"));
        m_aufgaben = m_sitzung.value(QLatin1String("challenges")).toList();
        m_nummer = 0;
        m_begonnen = jetztMs();
        if (m_aufgaben.isEmpty()) {
            m_sitzung.clear();
            emit fehlgeschlagen(tr("Diese Lektion kam ohne Aufgaben."));
            emit geaendert();
            return;
        }
        aufgabeZeigen();
        // Die Toene der ganzen Lektion im Voraus holen, damit der
        // Lautsprecherknopf sofort antwortet statt nach einer Sekunde.
        QStringList toene;
        for (int i = 0; i < m_aufgaben.size(); ++i) {
            const QVariantMap sicht = darstellen(m_aufgaben.at(i).toMap());
            const char *felder[] = { "ton", "tonLangsam", "loesungTon", 0 };
            for (int f = 0; felder[f]; ++f) {
                const QString a = sicht.value(QLatin1String(felder[f])).toString();
                if (!a.isEmpty())
                    toene << a;
            }
            const QVariantList wahlen = sicht.value(QLatin1String("wahlen")).toList();
            for (int w = 0; w < wahlen.size(); ++w) {
                const QString a = wahlen.at(w).toMap()
                                      .value(QLatin1String("ton")).toString();
                if (!a.isEmpty())
                    toene << a;
            }
            const QVariantList links = sicht.value(QLatin1String("links")).toList();
            for (int w = 0; w < links.size(); ++w) {
                const QString a = links.at(w).toMap()
                                      .value(QLatin1String("ton")).toString();
                if (!a.isEmpty())
                    toene << a;
            }
        }
        if (!toene.isEmpty())
            emit toeneVorladen(toene);
        return;
    }
    if (kennung != QLatin1String(ABGEBEN))
        return;

    setzeLaeuft(false);
    const int status = daten.value(QLatin1String("__status")).toInt();
    if (status == 202) {
        // Angenommen, aber noch nicht verrechnet. Die Weboberflaeche
        // schickt dann noch einmal; wir sagen es und lassen den Knopf.
        emit fehlgeschlagen(tr("Der Server verrechnet noch. Noch einmal "
                               "abschicken?"));
        return;
    }
    const int xp = daten.value(QLatin1String("xpGain")).toInt();
    m_sitzung.clear();
    m_aufgaben.clear();
    m_aufgabe.clear();
    emit geaendert();
    emit abgegeben(xp);
    m_konto->auffrischen();
}

void Lektion::fehler(const QString &kennung, int status, const QString &text)
{
    if (kennung != QLatin1String(HOLEN) && kennung != QLatin1String(ABGEBEN))
        return;
    setzeLaeuft(false);
    if (kennung == QLatin1String(HOLEN))
        m_sitzung.clear();
    emit geaendert();
    emit fehlgeschlagen(status > 0
                            ? tr("%1 (%2)").arg(text).arg(status)
                            : text);
}
