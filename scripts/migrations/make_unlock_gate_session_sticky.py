#!/usr/bin/env python3
"""Make the standalone owner gate sticky for one credential/approach session.

The gate module owns the sticky state.  main.c only supplies two lifecycle seams:
- reset the latch on the existing credential-session rising edge;
- latch the first owner denial and veto the approach controller once.

Once denied, later unlock actions in that same session are ignored without
re-vetoing or log spam.  Turning the Matter switch back on does not unlock a
phone that is already present; a new credential session is required.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MAIN = ROOT / "apps" / "dwm3001cdk-lock" / "src" / "main.c"
s = MAIN.read_text()

# Reset only at the lifecycle edge the upstream app already owns.  Do not add a
# second session detector to the feature.
session_anchor = '''\t\tif (session_now && !session_was_up) {\n\t\t\t/*\n\t\t\t * A session cannot come up without the phone approaching: the\n'''
if 'unlock_gate_session_reset();' not in s:
    if session_anchor not in s:
        raise SystemExit('credential-session rising-edge anchor not found in main.c')
    # Put the reset before the existing comment/call so the feature hook remains
    # a single obvious line at the session boundary.
    repl = '''\t\tif (session_now && !session_was_up) {\n#ifdef CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH\n\t\t\tunlock_gate_session_reset();\n#endif\n\t\t\t/*\n\t\t\t * A session cannot come up without the phone approaching: the\n'''
    s = s.replace(session_anchor, repl, 1)

old_gate = '''#ifdef CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH\n\t\t\tif (!unlock_gate_allows_passive()) {\n\t\t\t\tultrawidelock_approach_veto(&approach);\n\t\t\t\tLOG_INF("passive unlock withheld by owner gate");\n\t\t\t\tbreak;\n\t\t\t}\n#endif\n'''
new_gate = '''#ifdef CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH\n\t\t\t/* A denial is sticky for this credential session.  The first\n\t\t\t * denial repairs the approach controller once; later unlock\n\t\t\t * actions are simply ignored until the next session edge. */\n\t\t\tif (unlock_gate_session_blocked()) {\n\t\t\t\tbreak;\n\t\t\t}\n\t\t\tif (!unlock_gate_allows_passive()) {\n\t\t\t\tunlock_gate_session_block();\n\t\t\t\tultrawidelock_approach_veto(&approach);\n\t\t\t\tLOG_INF("passive unlock withheld by owner gate for this session");\n\t\t\t\tbreak;\n\t\t\t}\n#endif\n'''

if 'passive unlock withheld by owner gate for this session' not in s:
    if old_gate not in s:
        raise SystemExit('existing standalone owner-gate hook not found in main.c')
    s = s.replace(old_gate, new_gate, 1)

MAIN.write_text(s)
print('Made unlock gate denial sticky until the next credential session.')
