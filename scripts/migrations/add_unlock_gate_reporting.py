#!/usr/bin/env python3
"""Add deferred Matter subscription reporting for the standalone unlock gate.

Mirrors the existing DoorLock LockState notification path: the InvokeResponse is
built first, then a system-work-queue item originates a small ReportData message
for endpoint 3 / OnOff / OnOff to every active CASE subscription.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
P = ROOT / "apps" / "dwm3001cdk-lock" / "src" / "matter_commission.c"
s = P.read_text()

# 1) Add the endpoint-3 report helper immediately before the existing notify work.
marker = '''/**\n * Work callback that sends lock state subscription reports to all CASE sessions.\n */\nstatic void notify_work_fn(struct k_work *w)\n'''
if 'static void notify_unlock_gate_state(struct sub_state *s)' not in s:
    fn = r'''#ifdef CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH
/** Send the owner-gate OnOff attribute to one active CASE subscription. */
static void notify_unlock_gate_state(struct sub_state *s)
{
	struct matter_im_read one;
	size_t tlv_len = 0u;
	size_t framed = 0u;
	uint8_t slot;
	int rc;

	if (!s->in_use || !s->active || s->session_id == 0u || !s->peer.valid) {
		return;
	}
	slot = case_slot_of(s->session_id);
	if (slot >= MATTER_CASE_SESSIONS) {
		return;
	}

	memset(&one, 0, sizeof(one));
	one.n_paths = 1u;
	one.paths[0].endpoint = MATTER_ENDPOINT_UNLOCK_GATE;
	one.paths[0].have_endpoint = true;
	one.paths[0].cluster = MATTER_CLUSTER_ON_OFF;
	one.paths[0].have_cluster = true;
	one.paths[0].attribute = MATTER_ATTR_ON_OFF_ON_OFF;
	one.paths[0].have_attribute = true;
	one.subscription_id = s->id;

	rc = matter_im_report_data_encode(&s_im, &one, s_notify_tlv, sizeof(s_notify_tlv),
					  &tlv_len, NULL);
	if (rc != MATTER_OK) {
		LOG_ERR("  cannot build the UnlockGate report (%d)", rc);
		return;
	}

	rc = matter_exchange_send_initiator(&s_case_x[slot], s_next_init_exchange++,
					    MATTER_PROTOCOL_INTERACTION_MODEL,
					    MATTER_IM_OP_REPORT_DATA, s_notify_tlv, tlv_len,
					    s_notify_out, sizeof(s_notify_out), &framed);
	if (rc != MATTER_OK) {
		LOG_ERR("  cannot frame the UnlockGate report (%d)", rc);
		return;
	}
	rc = matter_thread_send_to(&s->peer, s_notify_out, framed);
	LOG_INF("  UnlockGate report to subscription 0x%08x, %u B, rc=%d",
		(unsigned int)s->id, (unsigned int)framed, rc);
}
#endif

'''
    if marker not in s:
        raise SystemExit('notify work marker not found')
    s = s.replace(marker, fn + marker, 1)

# 2) Add a dedicated work item and submission helper next to LockState's work item.
anchor = '''static K_WORK_DEFINE(s_notify_work, notify_work_fn);\nstatic void heartbeat_work_fn(struct k_work *w);\n'''
if 's_unlock_gate_notify_work' not in s:
    block = r'''static K_WORK_DEFINE(s_notify_work, notify_work_fn);
#ifdef CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH
static void unlock_gate_notify_work_fn(struct k_work *w)
{
	ARG_UNUSED(w);
	for (uint8_t i = 0u; i < MATTER_CASE_SESSIONS; i++) {
		notify_unlock_gate_state(&s_subs[i]);
	}
}
static K_WORK_DEFINE(s_unlock_gate_notify_work, unlock_gate_notify_work_fn);

static void notify_unlock_gate_changed(void)
{
	k_work_submit(&s_unlock_gate_notify_work);
}
#endif
static void heartbeat_work_fn(struct k_work *w);
'''
    if anchor not in s:
        raise SystemExit('s_notify_work anchor not found')
    s = s.replace(anchor, block, 1)

# 3) Trigger it after a successful endpoint-3 OnOff command, at the same safe point
# where DoorLock queues LockState reporting (after response encoding, before send).
anchor = '''\tif (inv.cluster == MATTER_CLUSTER_DOOR_LOCK &&\n\t    (inv.command == MATTER_CMD_DL_LOCK_DOOR || inv.command == MATTER_CMD_DL_UNLOCK_DOOR)) {\n\t\tnotify_lock_state_changed();\n\t}\n'''
if 'notify_unlock_gate_changed();' not in s:
    addition = anchor + r'''#ifdef CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH
	if (inv.endpoint == MATTER_ENDPOINT_UNLOCK_GATE && inv.cluster == MATTER_CLUSTER_ON_OFF &&
	    (inv.command == MATTER_CMD_ON_OFF_OFF || inv.command == MATTER_CMD_ON_OFF_ON ||
	     inv.command == MATTER_CMD_ON_OFF_TOGGLE)) {
		notify_unlock_gate_changed();
	}
#endif
'''
    if anchor not in s:
        raise SystemExit('DoorLock notify trigger anchor not found')
    s = s.replace(anchor, addition, 1)

P.write_text(s)
print('Installed deferred endpoint-3 OnOff subscription reporting.')
