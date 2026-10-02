"""Regenerate firmware/Wortuhr/certs.h from a Mozilla CA bundle (PEM).

usage: python tools/make_certs.py path/to/ca-bundle.crt

Check which roots GitHub currently uses first:
  openssl s_client -connect github.com:443 -showcerts
  openssl s_client -connect release-assets.githubusercontent.com:443 -showcerts
"""
import re
import subprocess
import sys

# Two small lists, each loaded only for its own connection (heap is tight on the ESP8266):
#   github.com           -> Sectigo E36 -> E46 -> USERTrust ECC (ECDSA, fast)
#   release-assets CDN   -> Let's Encrypt YR1 -> Root YR -> ISRG Root X1 (RSA-4096)
GROUPS = {
    'GH_CA_GITHUB': ['USERTrust ECC Certification Authority', 'Sectigo Public Server Authentication Root E46'],
    'GH_CA_ASSETS': ['ISRG Root X1'],  # one root only: the update boot has ~1 KB to spare
}
ROOTS = sorted({n for g in GROUPS.values() for n in g})

bundle = open(sys.argv[1], encoding='utf-8', errors='replace').read()
found = {}
for pem in re.findall(r'-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----', bundle, re.S):
    subj = subprocess.run(['openssl', 'x509', '-noout', '-subject'], input=pem,
                          capture_output=True, text=True).stdout.strip()
    for name in ROOTS:
        if subj.endswith('CN=' + name) or subj.endswith('CN = ' + name):
            found[name] = pem
missing = [n for n in ROOTS if n not in found]
if missing:
    sys.exit('missing in bundle: ' + ', '.join(missing))

out = ['#pragma once',
       '// Root CAs for the GitHub update download; regenerate with tools/make_certs.py if GitHub changes its CAs.',
       '// Taken from the Mozilla CA bundle.']
for var, names in GROUPS.items():
    out.append('static const char %s[] PROGMEM = R"PEM(' % var)
    out += [found[n].strip() for n in names]
    out.append(')PEM";')
with open('firmware/Wortuhr/certs.h', 'w', newline='\n') as f:
    f.write('\n'.join(out) + '\n')
print('written', ', '.join('%s=%d' % (k, len(v)) for k, v in GROUPS.items()))
