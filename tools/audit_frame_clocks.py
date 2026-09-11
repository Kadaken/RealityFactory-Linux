#!/usr/bin/env python3
"""List lexical wall/CPU clock call sites in the exact inherited build graph."""
import collections
import json
import pathlib
import re

root = pathlib.Path(__file__).resolve().parents[1]
project = (root / 'upstream/RealityFactory.vcproj').read_text()
names = sorted(set(re.findall(r'RelativePath="([^"]+\.(?:c|cpp))"', project)))
assert len(names) == 157, 'Inherited build graph changed'
sites = []
counter_reads = []
for name in names:
    name = name.replace('\\', '/')
    if name == 'drawbbox.c':
        name = 'Drawbbox.c'
    text = (root / 'upstream' / name).read_text(errors='replace')
    # Preserve newlines/offsets while discarding comments and string literals.
    code = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
        lambda m: re.sub(r'[^\n]', ' ', m.group()), text, flags=re.S)
    for match in re.finditer(r'\b(timeGetTime|clock|time)\s*\(', code):
        sites.append(dict(file=name, line=code.count('\n', 0, match.start())+1,
                          api=match.group(1)))
    for match in re.finditer(r'(?:->|\.)\s*(FreeRunningCounter(?:_F)?)\s*\(', code):
        counter_reads.append(dict(file=name, line=code.count('\n', 0, match.start())+1,
                                  api=match.group(1)))
counts = collections.Counter(site['file'] for site in sites)
print(json.dumps(dict(units=len(names), lexical_sites=len(sites),
    top_five=counts.most_common(5), sites=sites, timer_counter_reads=counter_reads), indent=2))
