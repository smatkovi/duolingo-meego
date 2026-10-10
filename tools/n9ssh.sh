#!/bin/sh
# SSH zum N9/N950. Deren OpenSSH 5.1p1 kennt weder ed25519 noch SHA-2, also
# ssh-rsa fuer Rechner- und Benutzerschluessel und der eigene Schluessel.
#
#   N9_HOST=192.168.1.8 tools/n9ssh.sh 'befehl'
#
# N9_JUMP springt ueber einen Zwischenwirt. Das ist hier der Normalfall und
# nicht die Ausnahme: der Zugangspunkt trennt seine WLAN-Teilnehmer
# voneinander (client isolation), das Telefon kommt also nicht ans N950,
# der Arch-Rechner aber schon. Der Schluessel bleibt dabei hier -- bei
# ProxyJump handelt der eigene Client den letzten Sprung aus, der
# Zwischenwirt leitet nur Bytes weiter.
set -e
HOST=${N9_HOST:-192.168.1.8}
JUMP=${N9_JUMP:-sebastian@192.168.1.21}

SPRUNG=""
if [ -n "$JUMP" ] && ! ping -c1 -W2 "$HOST" >/dev/null 2>&1; then
    SPRUNG="-J $JUMP"
fi

exec ssh $SPRUNG \
    -oHostKeyAlgorithms=+ssh-rsa -oPubkeyAcceptedAlgorithms=+ssh-rsa \
    -oStrictHostKeyChecking=accept-new \
    -i "$HOME/.ssh/id_rsa_n9" -oConnectTimeout=15 \
    "${N9_USER:-user}@$HOST" "$@"
