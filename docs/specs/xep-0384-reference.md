# XEP-0384 Reference (OMEMO:2)

Canonical spec:
- https://xmpp.org/extensions/xep-0384.html

This file is a working checklist for implementation decisions in this repository.
Use it as a gate: protocol behavior should follow the MUST/SHOULD requirements
from XEP-0384 unless a deviation is explicitly documented.

## Non-Negotiable Wire Format

- Namespace for encrypted payloads: urn:xmpp:omemo:2
- Device list node: urn:xmpp:omemo:2:devices
- Bundle node: urn:xmpp:omemo:2:bundles
- Encrypted message structure:
  - encrypted/header is required
  - payload is optional only for empty OMEMO messages (key transport/heartbeat)
  - header uses sid (sender device id)
  - keys uses jid (bare JID)
  - key uses rid (recipient device id) and optional kex flag

## Device List + Bundle Publishing Rules

- Device id must be unique per account and in range 1..2^31-1.
- Devices MUST subscribe to contacts' devices node and cache latest list.
- devices node access model MUST be open.
- Bundles are published as separate items in bundles node; item id MUST be device id.
- bundles node must allow multiple items (pubsub#max_items = max).
- bundles node access model MUST be open.
- Bundle contents MUST include: spk (+id), spks, ik, prekeys/pk(+id).
- Bundle SHOULD keep around 100 prekeys and MUST have at least 25.

## Session Establishment / Sending

- Build sessions just-in-time before sending.
- Key exchange MUST use a prekey.
- Sender MUST encrypt for recipient devices listed on recipient devices node.
- Sender SHOULD include own other devices for sync behavior.
- If recipient session missing, fetch bundle and establish; do not invent alternate nodes.
- Empty OMEMO messages are valid for session-management flows.

## Receiving / Recovery

- Receiver must locate key element for own bare JID + own device id.
- If message is key exchange, build/replace session and rotate consumed prekey.
- Decryption failures should not silently auto-reset sessions.
- Client should provide explicit manual session replacement path for broken sessions.

## Security / Trust Rules

- Do not opportunistically trust newly discovered devices without trust policy handling.
- Do not initiate new sessions automatically as decryption-error fallback without user interaction.
- Fingerprint representation should be interoperable with OMEMO guidance.

## Interop Policy For This Repository

- Primary target: strict OMEMO:2 conformance to XEP-0384 wire format.
- Any compatibility fallback must be disabled by default and documented with rationale,
  scope, and expected removal conditions.
- Prefer protocol-correct diagnostics over silent fallback when peer/server data is invalid.

## Related Specs

- XEP-0060 (PubSub)
- XEP-0163 (PEP)
- XEP-0420 (SCE envelope)
- XEP-0045 (MUC OMEMO recipient handling)
- XEP-0313 (MAM catch-up interactions)
