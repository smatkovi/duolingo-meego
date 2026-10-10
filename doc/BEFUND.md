# Duolingo auf MeeGo -- was gemessen ist

Stand 10.10.2026. Alles hier ist nachgemessen oder aus dem Web-Buendel
gelesen, nichts geraten. Die Werkzeuge, die es erzeugt haben, stehen dabei.

## Warum ein eigener Client und keine Portierung

* **Weboberflaeche**: React-Einseiter, ueber 2 MB JavaScript, dazu
  recaptcha-enterprise. Der Browser des N9 spielt das nicht.
* **APK**: Kotlin auf ART, dazu Schach, Rechnen und DuoRadio. Nicht
  portierbar.
* Also wie [[tgtg-sfos-meego-projekt]] und [[radelt-app-project]]: eigener
  Client gegen dieselbe Schnittstelle, die die Weboberflaeche benutzt.

## Die Schnittstelle steht offen

* `GET /2017-06-30/users?username=duo` -> 200. **Kein Keks noetig, kein
  DataDome, kein Cloudflare-Wall** -- auch nicht von der Heimleitung, die
  bei Too Good To Go als Roboternetz gefuehrt wird
  ([[heimanschluss-cgnat]]). `api.duolingo.com` und
  `android-api-cf.duolingo.com` antworten gleich.
* **Zwei Basen**: `/2017-06-30` fuer Konto und Anmeldung, `/2023-05-23`
  fuer Sitzungen (im Buendel: `qn = ue("/2023-05-23")`).
* **Anmeldung**: `POST /2023-05-23/login?fields=jwt` mit
  `{identifier, password, distinctId}`. **Die Marke kommt im Antwortkopf
  `jwt`**, nicht im Koerper -- wer nur den Koerper liest, haelt eine
  geglueckte Anmeldung fuer leer. Die Benutzerkennung steht als `sub` in
  der Marke.
* **Und sie haengt am Benutzeragenten** (10.10.2026 an einem Probekonto mit
  bekanntem Passwort gemessen, `tools/anmeldung-matrix.py`):

  | Agentenzeile | Antwort |
  |---|---|
  | `Duodroid/6.0.0` | **200, Marke** |
  | `Duodroid/6.0.0 Dalvik/2.1.0 (Linux; U; MeeGo 1.2 Harmattan; Nokia N9)` | **200, Marke** |
  | `Duodroid/6.0.0 Qt/4.7.4 (…)` | 401 |
  | `Duodroid/0.1.0-meego5` | 401 |
  | eigener Name, Chrome, Chrome + Origin/Referer | 401 |

  Der Grund steht im Buendel: die Weboberflaeche legt der Anmeldung ein
  **reCAPTCHA-Enterprise-Token** bei -- Feld **`signal`**, aus
  `c.e("LOGIN")`. Wer kein Browser ist, kommt nur ueber den Weg der App
  hinein, und der Server prueft die Zeile streng:
  `Duodroid/<zahl.zahl.zahl>`, danach nichts oder `Dalvik/…`.
  **Alle anderen Antworten sehen gleich aus**: derselbe 401 mit demselben
  Satz, ob Kennung falsch, Passwort falsch, kein Passwort gesetzt oder
  Agentenzeile verkehrt. Ohne ein Konto zum Gegenhalten ist da nichts zu
  unterscheiden.
* **Ein Probekonto mit Passwort** bekommt man ohne reCAPTCHA: frisches
  Wegwerfkonto, dann `PATCH /2023-05-23/users/{id}` erst mit `username`,
  dann mit `email`+`password`+`name`. Nur das Passwort allein aendern geht
  nicht (`IDENTITY_INVALID` / `PASSWORD_INCORRECT`), und beides in einem Zug
  am ganz frischen Konto gibt 403.
* **Ohne Konto geht auch etwas**: `POST /2023-05-23/users` mit
  `{distinctId, fromLanguage, learningLanguage, timezone}` legt den
  Besucher an, den die Weboberflaeche fuer "ohne Anmeldung ausprobieren"
  benutzt. Genau das nimmt `tools/sitzung-abziehen.py` als **Wegwerfkonto**
  -- damit beim Ausprobieren der Benotung kein echter Kurs leidet.

## Die Lektion

* Holen: `POST /2023-05-23/sessions`. **`challengeTypes` erklaert der
  Client selbst** -- der Server schickt nur, was dasteht. Das ist der
  Hebel: wir erklaeren 30 Arten und muessen nicht 64 Bauteile schreiben.
* Abgeben: `PUT /2023-05-23/sessions/{id}` mit Kopfzeile
  `Idempotency-Key: {id}`. 202 heisst "angenommen, noch nicht verrechnet".
* **Fuer LESSON reicht die Stufe allein nicht.** Ohne `levelId`,
  `levelSessionIndex` und `pathLevelSessionMetadata` antwortet der Server
  mit **500**. Die richtige Zusammenstellung steht in
  `tools/lektion-koerper-lesen.py` (aus dem Buendel) und in
  `src/lektion.cpp`: dazu `skillId` und `levelIndex` aus
  `pathLevelClientData`, `treeId` aus `pathLevelMetadata`.
* Groessenunterschied, der auf einer Mobilfunkleitung zaehlt:
  **LESSON ~24 kB, GLOBAL_PRACTICE ~440 kB** (dessen
  `adaptiveChallenges` und `ttsAnnotations` machen den Unterschied).

## Benotet wird im Client, der Server glaubt es

Jede Aufgabe traegt beim Abgeben ihr `correct` selbst. **Eine falsch
gerechnete Benotung verdirbt den echten Kursfortschritt**, nicht nur die
Anzeige. Darum:

* Die Regel je Aufgabenart ist aus der Weboberflaeche abgeschrieben:
  `tools/aufgaben-lesen.py` schreibt `doc/aufgabenarten.md` (64 Arten mit
  ihrem `grade`, `validate`, `getTts`).
* Getippte Antworten benotet ein **Graph**, den jede Aufgabe mitbringt
  (`grader`): `vertices[i]` sind Kanten `{to, lenient, orig}`, leeres
  `lenient` ist ein Leerschritt. **Das Leerzeichen ist eine eigene Kante,
  kein Trenner** -- daran scheiterte der erste Versuch (10 von 15
  Musterloesungen abgelehnt).
* Angetippte Antworten: `choices` + `correctIndices`, Reihenfolge zaehlt;
  stimmt sie nicht, faellt die Benotung auf den Graphen zurueck -- so macht
  es die Weboberflaeche auch.
* `match`/`listenMatch` gelten als richtig, sobald eine Antwort da ist
  (`correct: void 0!==e.guess`); die Darstellung laesst nur richtige Paare zu.
* Geprueft wird das zweifach gegen echte Rohdaten -- in Python
  (`tools/benoter-pruefen.py`) und in C++ (`meego/tests/benoter_test.cpp`,
  `meego/build.sh test`): **jede Aufgabe muss ihre eigene Musterloesung
  annehmen und eine halbe oder verdrehte ablehnen.** Stand: 31 von 31
  anerkannt, 15 von 15 falschen erkannt.

## Der Durchlauf ist durchgespielt

Am Wegwerfkonto, nicht am eigenen (`tools/sitzung-abgeben.py`):

| Schritt | Ergebnis |
|---|---|
| `POST /sessions` GLOBAL_PRACTICE | 200, 16 Aufgaben |
| alle richtig benotet, `PUT /sessions/{id}` | 200, `xpGain: 13` |
| Konto danach | XP 0 -> 13, Straehne 0 -> 1 |
| `POST /sessions` LESSON (Pfadstufe) | 200, 12 Aufgaben |
| `PUT` | 200, `xpGain: 10`, Kurs-XP waechst mit |

**Offen, und es ist die einzige offene Frage:** `finishedSessions` der
Pfadstufe bleibt am Wegwerfkonto auf 0 -- XP und Straehne kommen an, der
Pfad ruckt nicht weiter. Mitgeschickte `levelId`/`pathLevelSessionMetadata`
aendern daran nichts. Wahrscheinlich speichert Duolingo den Pfadfortschritt
fuer ein nicht angemeldetes Konto gar nicht (die Weboberflaeche rechnet ihn
fuer solche Besucher selbst aus -- im Buendel steht dafuer eine eigene
Funktion, "Offline path update"). **Das entscheidet sich beim ersten Lauf
mit dem richtigen Konto**, und das ist eine Frage an den Nutzer, kein
Versuch, den man heimlich macht.

## Ton und Bilder

* Tonwolke: `d1vq87e9lcf771.cloudfront.net`. **Gibt dieselben Dateien ueber
  `http` heraus** (200 audio/mpeg, gleiche Groesse). Damit braucht
  gstreamer keinen Vermittler ([[harmattan-tls-stacks]]) -- `src/lektion.cpp`
  schreibt jede Tonadresse auf `http` um.
* Bilder der Figuren (`character.url`) sind **PDF**, nicht PNG -- fuer Qt 4.7
  wertlos, bleiben in 0.1 weg. Die Aufgabe `name` bringt ein echtes
  `pngUrl` mit, das wird gezeigt.

## Das Startersymbol

`https://d35aaqx5ub95lt.cloudfront.net/images/duolingo-touch-icon2.png`
(aus `rel="apple-touch-icon"`). Es ist fuer iOS geschnitten und hat
**durchsichtige Ecken** -- die Falle aus [[meego-squircle-icons]]: erst zum
vollen Quadrat in der gemessenen Grundfarbe (8ee000) auffuellen, dann die
Squircle-Maske. `tools/icon-bauen.py`.
