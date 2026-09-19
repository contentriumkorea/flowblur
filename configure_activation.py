"""Generate private build verifiers without storing a plaintext password."""
import getpass
import hashlib
import secrets
from pathlib import Path

password = getpass.getpass('Activation password: ')
if not password or password != getpass.getpass('Confirm password: '):
    raise SystemExit('Passwords must be non-empty and match.')
salt = secrets.token_hex(24)
verifier = hashlib.pbkdf2_hmac('sha256', password.encode(), salt.encode(), 210000).hex()
target = Path(__file__).parent / 'native/activation_config.local.h'
target.write_text('#pragma once\n#define FLOWBLUR_ACTIVATION_SALT "' + salt +
                  '"\n#define FLOWBLUR_ACTIVATION_HASH "' + verifier + '"\n', encoding='utf-8')
print('Private activation configuration written. Do not commit this file.')
