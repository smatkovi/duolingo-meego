# Schach bei Duolingo -- was dahintersteckt

Gemessen am 10.10.2026 an einem Wegwerfkonto (`tools/schach-ansehen.py`).

**Schach ist nicht die Lektionsmaschine.** Es laeuft ueber einen eigenen,
kleinen Dienst unter `/chess/1/{benutzer}/…`, und zwar in ganz gewoehnlichem
Schachdeutsch: FEN-Stellung und Zugnamen. Die Sitzungsarten `CHESS_*` und
die Aufgabenarten `chessMatch`/`chessPvpMatch` aus dem Buendel braucht man
dafuer gar nicht.

## Was offen steht

| Aufruf | Antwort |
|---|---|
| `PATCH /2023-05-23/users/{id}` mit `currentCourseId: "CHESS_CH"` | 200, Kurs umgestellt |
| `GET /chess/1/{id}/matches?sectionNumber=1` | 200: `activeMatches`, `completedMatches`, `eloRating` (200 am Anfang), `record` |
| `POST /chess/1/{id}/matches` `{sectionNumber, userTimezone}` | **201** mit der ganzen Partie |
| `PATCH /chess/1/{id}/matches/{partie}` `{moveName, fen}` | 200, Brett uebernommen |
| `GET /chess/1/{id}/daily-puzzle` | 200, das taegliche Raetsel |

Die Partie selbst (`match`) ist angenehm klein:
`{id, boardFen, moveHistory, moveTimestamps, playerColor, status,
endCondition, outcome, whitePlayerClock, blackPlayerClock}` -- die
Anfangsstellung kommt als
`rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1`.

Dazu liefert der Dienst die Lehrerstimme gleich mit: `illegalMoveContent`
erklaert je Fehlergrund in einem Satz, warum ein Zug nicht geht ("That
square is under attack. Your king can't go there.") -- mit TTS-Adresse und
Mundbildern. `userMoveHintConfig`, `reaction` und `matchEndReaction`
gehoeren zur Figur daneben.

## Was noch fehlt

Der Zug wird mit `{moveName, fen}` angenommen (200, Brett steht richtig),
aber `moveHistory` bleibt leer und `opponentMove` ist `null` -- der Bot
zieht noch nicht zurueck. Da fehlt also noch ein Feld oder ein zweiter
Aufruf; die Schachoberflaeche der Weboberflaeche liegt in einem eigenen
Buendelstueck, das noch nicht gelesen ist.

## Was daraus geworden ist (0.2.0)

* `src/core/Position` und `third_party/chess-library` kommen aus
  `~/ps/harbour-schachlehrer` -- dasselbe Brett, das dort gegen feste
  Stellungen geprueft ist. Die Figuren ebenso (cburnett, BSD-3-Clause).
* `src/schach.cpp` spricht mit dem Dienst, `meego/qml/Brett.qml` zeichnet,
  `SchachPage.qml` haelt es zusammen.
* **Den Gegenzug rechnet das Telefon** (`src/bot.cpp`): Alpha-Beta ueber
  zwei bis vier Halbzuege, Material plus ein paar Felder. Kein Stockfish --
  Duolingos Bots fangen bei 200 Elo an, und ein 40-MB-Netz hat auf einem N9
  nichts verloren. Die Probe (`meego/build.sh schach`) prueft die Umrechnung
  FEN -> 64 Felder, dass der Gegner nur Erlaubtes zieht, dass eine Partie zu
  Ende kommt und dass er ein Matt in einem Zug findet.
* Elo und Bilanz kommen vom Dienst, jeder Zug geht dorthin zurueck.

**Was daran unsicher ist:** ob Duolingos eigener Bot wirklich im Client
rechnet. Dafuer spricht, dass der Server auf einen Zug nichts zurueckzieht
und dass er die Saetze fuer unmoegliche Zuege gleich beim Anlegen der Partie
mitschickt (die braucht nur, wer selbst gegen den Bot zeichnet). Dagegen
spricht nichts Gemessenes -- aber das Buendelstueck mit der Schachoberflaeche
ist noch nicht gefunden. Solange das offen ist, gilt: **unser Gegner ist
nicht ihr Gegner**, auch wenn die Partie in derselben Bilanz landet.
