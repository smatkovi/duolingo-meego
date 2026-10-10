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

## Was es zu bauen gaebe

Ein Brett, das FEN zeichnet und Zuege abschickt -- und genau das gibt es
schon: `~/ps/harbour-schachlehrer`. Der Rest ist diese Handvoll Aufrufe.
Kein Vergleich mit der Lektionsmaschine; eher ein Nachmittag als ein Projekt.
