# Duolingo fuer MeeGo Harmattan

Ein eigener Duolingo-Client fuer das Nokia N9 und N950 -- kein Teil von
Duolingo, keine Portierung der Android-App, sondern dieselbe Schnittstelle,
die die Weboberflaeche benutzt, hinter einer com.nokia.meego-Oberflaeche.

## Was drin ist

* **Sprachen**: Kurspfad, Lektionen, Uebung ueber den ganzen Kurs, die
  Tests, mit denen sich eine gesperrte Einheit ueberspringen laesst, und
  41 Aufgabenarten in vier Darstellungen (eine Wahl treffen, Woerter
  antippen, Paare zusammenbringen, frei eintippen).
* **Schach**: der Kurs mit seinen gefuehrten Lektionen (Stellung,
  verlangte Zugfolge, Satz des Lehrers mit Ton) *und* Partien gegen den
  Rechner, mit Duolingos Elo und Bilanz dahinter.
* **Rechnen**: die Formelzeile mit der Luecke, dazu Wahl, Zahlenfeld oder
  Bausteine.
* Ton ueber den Lautsprecherknopf, Straehne, XP und Herzen, Kurse
  wechseln.

Nicht drin: alles mit Mikrofon (die Weboberflaeche erkennt nichts selbst,
sie nimmt Chromes Erkenner -- ohne einen solchen koennten wir die Note nur
erfinden), Zeichenbrett, Geschichten und DuoRadio.

## Einspielen

    aegis-dpkg -i duolingo_<fassung>_armel.deb

**Nicht** mit `dpkg -i`: aus einem so eingespielten eigenen Paket laesst
sich auf Harmattan nichts starten.

## Anmelden

Mit Benutzername oder E-Mail und Passwort. Wurde das Konto mit Google
angelegt, hat es keines -- dann unter *Mit Google angemeldet?* entweder
die Mail zum Passwortsetzen anfordern (die Google-Anmeldung bleibt daneben
bestehen) oder die Marke aus dem Browser einsetzen.

## Wie es gebaut ist

Qt 4.7 mit QtQuick 1.1; HTTPS macht ein kleines Rust-Programm daneben,
weil Qt 4.7 auf Harmattan kein TLS 1.2 kann. Der Ton laeuft ueber http und
gstreamer, weil gstreamer dort gar kein TLS kann. Brett, Figuren und der
Schachkern kommen aus
[harbour-schachlehrer](https://github.com/smatkovi/harbour-schachlehrer).

Was an der Schnittstelle gemessen wurde und was offen ist, steht in
`doc/BEFUND.md`, `doc/SCHACH.md` und `doc/WAS-NOCH-FEHLT.md`.
