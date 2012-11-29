import json
import sys

# Note: tr stands for translate.
to_tr = ('attackername', 'inflictorname', 'sourcename', 'targetname', 'targetsourcename')

# Fuck, I love python. Had to resist not to do this in one line!
replay = map(json.loads, sys.stdin)
tr = replay[-1]['stringtable']  # The last element of a replay is the full combatlog stringtable.
combatlogs = filter(lambda ge: ge['evname'] == 'dota_combatlog', replay)
combatlogs_tr = map(lambda ge: {k: tr[v] if k in to_tr else v for k, v in ge.iteritems()}, combatlogs)
print('\n'.join(map(str, combatlogs_tr)))
