# Duolingo fuer MeeGo Harmattan

<img src="meego/icons/icon-320.png" width="128" align="right" alt="Das Startersymbol: die Eule in der Squircle-Silhouette der Harmattan-Standard-Apps">

Ein eigener Duolingo-Client fuer das Nokia N9 und N950. Kein Teil von
Duolingo, keine Portierung der Android-App: dieselbe Schnittstelle, die die
Weboberflaeche benutzt, hinter einer com.nokia.meego-Oberflaeche.

    Anmelden  ->  Pfad  ->  Lektion oder Ueben  ->  Aufgaben  ->  abgeben

Was drin ist: der Kurspfad, Lektionen und Uebungen, vier Darstellungen von
Aufgaben (eine Wahl treffen, Woerter antippen, Paare zusammenbringen, frei
eintippen), Ton ueber den Lautsprecherknopf, Straehne und XP.

Was nicht: alles mit Mikrofon oder Zeichenbrett, Geschichten, DuoRadio,
Schach und Rechnen. Der Server schickt diese Aufgaben gar nicht erst -- beim
Anfordern einer Lektion sagt der Client, welche Arten er zeichnen kann.

Repo: `smatkovi/duolingo-meego` (privat -- oeffentlich machen geht mit
`gh repo edit --visibility public`, andersherum nicht so leicht). Die
Pakete haengen am Release.

## Bauen

Alles Schwere laeuft auf dem Baurechner (Cross-Toolchain, MADDE-Sysroot,
Rusts musl-Ziel):

    meego/remote-build.sh          # spiegelt, baut, holt das .deb

Von Hand, auf dem Baurechner im gespiegelten Baum:

    meego/build-rust.sh            # duo-holer (HTTPS), statisch gegen musl
    meego/build.sh arm             # das Programm
    meego/build-deb.sh             # das Paket
    meego/build.sh test            # die Probe fuer den Benoter
    meego/tests/check-qml.sh       # das QML einlesen, ohne Geraet

Installiert wird mit `aegis-dpkg -i` -- aus `dpkg -i` laesst sich nichts
starten.

## Werkzeuge

    tools/bundle-lesen.py          # Routen und Arten aus dem Web-Buendel
    tools/aufgaben-lesen.py        # je Aufgabenart ihre Benotungsregel
    tools/lektion-koerper-lesen.py # wie der Koerper fuer POST /sessions aussieht
    tools/sitzung-abziehen.py      # eine echte Lektion abziehen (Wegwerfkonto)
    tools/sitzung-abgeben.py       # den ganzen Durchlauf durchspielen
    tools/benoter-pruefen.py       # die Benotung gegen echte Rohdaten
    tools/icon-bauen.py            # das Startersymbol in die Squircle-Form
    tools/repo-anlegen.sh          # das Repo auf GitHub anlegen (gh laeuft auf arch)
    tools/release-hochladen.sh     # ein .deb ans Release haengen

**Nie mit dem eigenen Konto ausprobieren.** Benotet wird im Client, der
Server glaubt es -- ein Fehler verdirbt den echten Kursfortschritt. Die
Werkzeuge legen sich dafuer ein Wegwerfkonto an, so wie es die
Weboberflaeche fuer Besucher ohne Anmeldung tut.

Was gemessen ist und was noch offen: `doc/BEFUND.md`.
