# Was es sonst noch gibt -- und was es kosten wuerde

Gemessen am 10.10.2026 am Wegwerfkonto. Keine Schaetzung aus dem Bauch:
jede Zeile hier hat eine Antwort des Dienstes hinter sich.

## Schach ist weit mehr als Spielen gegen den Rechner

Der Kurs `CHESS_CH` hat **6 Abschnitte mit 179 Stufen**:

| Stufenart | Anzahl | was es ist |
|---|---|---|
| `chess_skill` | 56 | gefuehrte Lektionen ("Move your rook and bishop") |
| `chess_practice` | 33 | Uebungen ("Which piece can capture?") |
| `chess_puzzles` | 28 | Aufgabensammlungen |
| `chess_unit_review` | 18 | Wiederholung je Einheit |
| `chess_match` | 20 | **Partie gegen den Rechner** -- nur das kann 0.2.0 |
| `chest` | 24 | Truhen (Belohnung) |

Eine Lektion holt man wie eine Sprachlektion, nur mit
`type: "CHESS_SESSION"` und der `metadata` aus `pathLevelClientData`
(darin steckt `chessJson` mit `contentId` und `type: CHESS_LESSON`).
Zurueck kommen Aufgaben der Art **`star`**:

```
fen              "8/8/1p2p3/8/8/8/8/4R3 w - - 0 1"
chessPuzzleInfo  correctMoves ["e1e6 e6b6"], enemyMoves [],
                 highlight ["e1e6 blue"], Lehrersatz samt TTS
indicatorType    "NEW_PIECE"
```

Also: Stellung zeigen, die verlangte Zugfolge spielen lassen, dazu ein
Satz vom Lehrer. **Das ist einfacher als die Partie** -- es braucht keinen
Gegner, nur den Vergleich mit `correctMoves`, und das Brett steht schon.
Dazu kommen `GET /chess/1/{id}/daily-puzzle` und die Partien gegen andere
(ueber Ably, Echtzeit).

## Mikrofon

**Die Weboberflaeche erkennt gar nichts selbst**: sie nimmt
`window.webkitSpeechRecognition`, also Chromes Spracherkennung. Benotet
wird danach rein oertlich -- `guess.score >= challenge.threshold`.

Auf Harmattan gibt es keinen Erkenner. Wir koennten die Note also nur
erfinden, und eine Sprechuebung, die alles durchwinkt, ist keine. **Darum
bleibt `speak` draussen**, bis gemessen ist, was die Android-App mit
`speechHost: speech.duolingo.com` macht.

## Zeichenbrett

`characterTrace` wird **oertlich** benotet: `currentStrokeIndex >=
strokes.length` -- die Aufgabe bringt die Striche mit, der Finger faehrt
sie nach, der Client zaehlt. Das liesse sich bauen. Es betrifft aber nur
Sprachen mit eigener Schrift (Chinesisch, Japanisch, Koreanisch,
Alphabete) -- fuer Latein und die lateinischen Sprachen kommt es nie vor.

## Geschichten

Eigener Weg (`storyId`, `mode`), bebildert und durchgehend vertont. Machbar,
aber eine eigene Oberflaeche von der Groesse der Lektionsmaschine.

## Rechnen

Der Kurs `MATH_BT` hat 2 Abschnitte, 211 `math_skill`-Stufen, 64
`math_match_practice`, 38 Wiederholungen. Eine Lektion kommt mit
`type: "MATH_LESSON"` und der `metadata` der Stufe -- **gemessen: 200, 15
Aufgaben** der Arten `mathProductSelect`, `mathTypeFill`,
`mathExpressionBuild`.

Die Aufgaben sind ein kleiner Formelbaum:

```
promptInputChallenge.prompt.body.mathSequenceElement.elements[]
   taggedTextElement {text: "3"} | {text: "<span>+</span>"}
   blankElement      + underlyingEntity.entity.integer.integer = 4
```

Die Luecke traegt die richtige Zahl selbst bei sich; benotet wird also
wieder im Client. Zu bauen waere eine Zeile aus Kaestchen und je Art ein
Eingabeteil -- mechanisch, aber zehn Arten.

## Reihenfolge, wenn man weitermacht

1. **Schachlektionen** -- groesster Gewinn, das Brett steht schon.
2. **Rechnen** -- mechanisch, klar beschrieben.
3. Zeichenbrett -- nur bei einer Sprache mit eigener Schrift.
4. Geschichten -- eigene Oberflaeche.
5. Mikrofon -- nicht, solange wir nur so tun koennten.
