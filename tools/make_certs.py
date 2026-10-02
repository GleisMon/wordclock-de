"""Regenerate firmware/Wortuhr/certs.h from a Mozilla CA bundle (PEM).

usage: python tools/make_certs.py path/to/ca-bundle.crt

Check which roots GitHub currently uses first:
  openssl s_client -connect github.com:443 -showcerts
  openssl s_client -connect release-assets.githubusercontent.com:443 -showcerts
"""
import re
import subprocess
import sys

ROOTS = [
    'USERTrust ECC Certification Authority',
    'USERTrust RSA Certification Authority',
    'Sectigo Public Server Authentication Root E46',
    'Sectigo Public Server Authentication Root R46',
    'ISRG Root X1',
    'ISRG Root X2',
    'DigiCert Global Root G2',
]

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
       "// Root CAs for the GitHub release download (github.com: Sectigo/USERTrust, asset CDN: ISRG/Let's Encrypt).",
       '// Taken from the Mozilla CA bundle; regenerate with tools/make_certs.py if GitHub changes its CAs.',
       'static const char GH_ROOT_CAS[] PROGMEM = R"PEM(']
out += [found[n].strip() for n in ROOTS]
out.append(')PEM";')
with open('firmware/Wortuhr/certs.h', 'w', newline='\n') as f:
    f.write('\n'.join(out) + '\n')
print('written', len(ROOTS), 'roots')
