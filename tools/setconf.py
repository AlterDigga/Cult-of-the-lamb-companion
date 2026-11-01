# Setzt Werte in ~/cotl-run/sdl-config.ini und markiert sie als nicht-Standard (eden-cli ignoriert sonst den Wert).
# Aufruf: python3 tools/setconf.py cpuopt_fastmem=false backend=1
import sys, os
p = os.path.expanduser('~/cotl-run/sdl-config.ini')
want = dict(kv.split('=', 1) for kv in sys.argv[1:])
out = []
for line in open(p).read().split('\n'):
    k = line.split('=', 1)[0]
    base = k[:-len('\\default')] if k.endswith('\\default') else k
    if base in want:
        line = (base + '\\default=false') if k.endswith('\\default') else (base + '=' + want[base])
    out.append(line)
open(p, 'w').write('\n'.join(out))
print([l for l in out if l.split('=', 1)[0].replace('\\default', '') in want])
