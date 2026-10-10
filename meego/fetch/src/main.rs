//! duo-holer: eine HTTPS-Anfrage an Duolingo fuer das N9/N950.
//!
//!     duo-holer <url> [--methode GET|POST|PUT|PATCH] [--marke <jwt>]
//!               [--agent <ua>] [--sprache de-AT]
//!
//! Der Koerper kommt von der Standardeingabe; eine Lektion ist schnell ein
//! paar hundert Kilobyte und hat in der Prozessliste nichts verloren.
//!
//! Die Antwort geht unveraendert auf die Standardausgabe. Auf die
//! Fehlerausgabe kommt `status: <zahl>` und jede Kopfzeile, die den Aufrufer
//! angeht -- vor allem `jwt:`, denn **die Anmeldung gibt die Marke im
//! Antwortkopf zurueck, nicht im Koerper**. Das ist der Vertrag, den
//! `Netz::holerFertig()` in src/netz.cpp liest.
//!
//! Warum es diesen Holer ueberhaupt gibt: Qt 4.7 auf Harmattan sitzt auf
//! einem OpenSSL von 2011 und bringt gegen www.duolingo.com keinen
//! Handschlag zustande. rustls spricht TLS 1.2 und 1.3 selbst und bringt
//! seine Wurzeln mit -- der Zertifikatsspeicher des Geraets ist genauso alt
//! wie sein OpenSSL.
//!
//! Gebaut statisch gegen musl, damit Harmattans glibc 2.10 nichts damit zu
//! tun hat.
use std::env;
use std::io::{Read, Write};
use std::process;
use std::time::Duration;

fn main() {
    let mut url = String::new();
    let mut methode = String::from("GET");
    let mut agent = String::from("Duolingo-MeeGo/0.1 (Linux; MeeGo 1.2 Harmattan)");
    let mut marke = String::new();
    let mut sprache = String::from("en");
    let mut idempotenz = String::new();

    let mut args = env::args().skip(1);
    while let Some(a) = args.next() {
        match a.as_str() {
            "--methode" => methode = args.next().unwrap_or_default().to_uppercase(),
            "--agent" => agent = args.next().unwrap_or_default(),
            "--marke" => marke = args.next().unwrap_or_default(),
            "--sprache" => sprache = args.next().unwrap_or_default(),
            "--idempotenz" => idempotenz = args.next().unwrap_or_default(),
            anderes if url.is_empty() && !anderes.starts_with("--") => {
                url = anderes.to_string()
            }
            anderes => {
                eprintln!("unbekanntes Argument {}", anderes);
                process::exit(2);
            }
        }
    }
    if url.is_empty() {
        eprintln!("Aufruf: duo-holer <url> [--methode GET] [--marke <jwt>]");
        process::exit(2);
    }

    let mut koerper = Vec::new();
    let _ = std::io::stdin().read_to_end(&mut koerper);

    match senden(&url, &methode, &agent, &sprache, &marke, &idempotenz, koerper) {
        Ok((status, kopf, antwort)) => {
            let _ = std::io::stdout().write_all(&antwort);
            let _ = std::io::stdout().flush();
            for z in kopf {
                eprintln!("header: {}", z);
            }
            eprintln!("status: {}", status);
        }
        Err(grund) => {
            eprintln!("grund: {}", grund);
            eprintln!("status: 0");
            process::exit(1);
        }
    }
}

fn senden(
    url: &str,
    methode: &str,
    agent: &str,
    sprache: &str,
    marke: &str,
    idempotenz: &str,
    koerper: Vec<u8>,
) -> Result<(u16, Vec<String>, Vec<u8>), String> {
    let klient = reqwest::blocking::Client::builder()
        // Lieber aufgeben als die Oberflaeche haengen lassen: am Telefon
        // wartet jemand darauf. Eine Lektion ist gross, darum grosszuegig.
        .timeout(Duration::from_secs(90))
        .build()
        .map_err(|e| e.to_string())?;

    let mut anfrage = match methode {
        "POST" => klient.post(url),
        "PUT" => klient.put(url),
        "PATCH" => klient.patch(url),
        "DELETE" => klient.delete(url),
        _ => klient.get(url),
    };
    anfrage = anfrage
        .header("User-Agent", agent)
        .header("Accept", "application/json")
        .header("Accept-Language", sprache);
    if !marke.is_empty() {
        anfrage = anfrage.header("Authorization", format!("Bearer {}", marke));
    }
    if !idempotenz.is_empty() {
        anfrage = anfrage.header("Idempotency-Key", idempotenz);
    }
    if !koerper.is_empty() {
        anfrage = anfrage
            .header("Content-Type", "application/json")
            .body(koerper);
    }

    let antwort = anfrage.send().map_err(|e| e.to_string())?;
    let status = antwort.status().as_u16();
    // Nur das, was der Aufrufer wirklich braucht. `jwt` ist die Marke aus
    // der Anmeldung; `no-retry` sagt, dass ein neuer Versuch zwecklos ist.
    let kopf: Vec<String> = antwort
        .headers()
        .iter()
        .filter(|(n, _)| {
            let n = n.as_str();
            n == "jwt" || n == "no-retry" || n == "retry-after"
        })
        .filter_map(|(n, v)| v.to_str().ok().map(|v| format!("{}: {}", n, v)))
        .collect();
    let daten = antwort.bytes().map_err(|e| e.to_string())?.to_vec();
    Ok((status, kopf, daten))
}
