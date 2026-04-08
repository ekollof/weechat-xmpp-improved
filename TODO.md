# OMEMO Refactor: Axolotl-only + BTBV TOFU

Branch: `omemo-axolotl-btbv`

## Goal

Collapse the dual OMEMO:2 / axolotl implementation down to a single
`eu.siacs.conversations.axolotl` stack, replace the XEP-0450 ATM trust model
with Gajim-style BTBV (Blind Trust Before Verification) TOFU, and remove all
dead OMEMO:2 code paths.

---

## Trust Model (BTBV)

| Value | Name       | Meaning                                          |
|-------|------------|--------------------------------------------------|
| 0     | UNTRUSTED  | Explicitly distrusted by user                    |
| 1     | VERIFIED   | Manually verified by user (fingerprint)          |
| 2     | UNDECIDED  | New device for JID that already has trusted keys |
| 3     | BLIND      | Auto-trusted (TOFU — first device for this JID)  |

**`get_default_trust(jid)`**: scan `trust:{jid}:*` LMDB keys; if any has value
VERIFIED(1) or UNTRUSTED(0) → return UNDECIDED(2); else → BLIND(3).

**Encrypt gate**: only VERIFIED(1) and BLIND(3) devices receive key material.

**LMDB key**: `trust:{bare_jid}:{device_id}` → decimal string `"0"`–`"3"`

---

## Status: ALL PHASES COMPLETE ✓

### Phase 0 — Planning & branch setup
- [x] Create `omemo-axolotl-btbv` branch
- [x] Write this TODO.md
- [x] Update `.github/copilot-instructions.md`

### Phase 1 — LMDB trust schema (`internal_prelude.inl`)
- [x] Add `enum class omemo_trust` with UNTRUSTED/VERIFIED/UNDECIDED/BLIND
- [x] Add `key_for_tofu_trust`, `store_tofu_trust`, `load_tofu_trust`
- [x] Add `get_default_trust(self, jid)` (BTBV scan)
- [x] Remove ATM trust helpers (`key_for_atm_trust`, etc.)
- [x] Remove `sce_wrap()`, `sce_wrap_content()`, `make_rpad()`
- [x] Remove `store_device_mode()`, `load_device_mode()`
- [x] Remove OMEMO:2 constants (`kOmemoNs`, `kDevicesNode`, `kBundlesNode`)
- [x] Remove `atm_fingerprint_b64()`, `key_for_devicelist()`, `utc_timestamp_now()`, `xml_escape()`
- [x] Remove truncated `hmac_sha256` declaration

### Phase 2 — Signal store trust callbacks (`internal_signal_store.inl`)
- [x] Rewrite `identity_save()`: BTBV `get_default_trust` on new identity
- [x] Rewrite `identity_is_trusted()`: VERIFIED/BLIND → 1; UNTRUSTED/UNDECIDED → 0
- [x] Remove unused `extract_devices_from_items`
- [x] Remove unused `extract_bundle_from_items` (and its orphaned body in `internal_stanza_parse.inl`)

### Phase 3 — Strip ATM (`session_flow.inl`, `commands.inl`)
- [x] Remove ATM functions from `commands.inl`
- [x] Remove ATM/OMEMO:2 code from `session_flow.inl`
- [x] Remove unused `devicelist_contains_device` static
- [x] Remove stale `request_devicelist` wrapper body
- [x] Replace thin `send_key_transport` wrapper with full inline implementation
- [x] Remove `/omemo approve`, `/omemo optout`, `/omemo optout-ack` subcommands
- [x] Rewrite `distrust_fp` to accept `std::optional<uint32_t>` device_id

### Phase 4 — Collapse encode/decode to axolotl-only (`codec.inl`)
- [x] Remove OMEMO:2 encode path
- [x] Remove OMEMO:2 decode path
- [x] Remove `peer_mode` dispatch
- [x] Fix `static_cast<int>(trust)` where trust is `std::optional<omemo_trust>`

### Phase 5 — Remove OMEMO:2 crypto (`internal_crypto.inl`)
- [x] Remove `omemo2_encrypt()`, `omemo2_decrypt()`
- [x] Remove orphaned `hmac_sha256` function body tail

### Phase 6 — Remove OMEMO:2 lifecycle (`lifecycle.inl`)
- [x] Remove `get_bundle()` (OMEMO:2 PEP publish path)
- [x] Remove `handle_devicelist()` (OMEMO:2 handler)
- [x] Remove `missing_omemo2_devicelist` tracking

### Phase 7 — Remove OMEMO:2 from connect lifecycle (`connect_lifecycle.inl`)
- [x] Stop fetching/publishing OMEMO:2 devicelist/bundle on connect

### Phase 8 — Remove OMEMO:2 from IQ/message handlers
- [x] `iq_handler.inl`: remove OMEMO:2 IQ dispatch, unused variables
- [x] `message_handler.inl`: remove ATM blocks, OMEMO:2 PEP handler, XEP-0434 feature ad

### Phase 9 — Clean up `omemo.hh`
- [x] Remove `peer_mode` enum
- [x] Remove `pending_atm_trust_from_unauthenticated`, `pending_atm_trust_for_unknown_key`
- [x] Remove `missing_omemo2_devicelist`, `omemo_opted_out_peers`
- [x] Remove OMEMO:2 / ATM method declarations
- [x] Add `trust_jid`, updated `distrust_fp` declarations

### Phase 10 — Config cleanup
- [x] `omemo_atm` config boolean left in place (referenced by existing config files)
       but no longer used by any OMEMO code path

### Phase 11 — Update `/omemo` commands
- [x] `commands.inl`: trust/distrust use `omemo_trust::` enum values
- [x] `command/encryption.inl`: `distrust_fp` call-site updated
- [x] `command/channel.inl`: `request_axolotl_devicelist` call updated
- [x] `command.cpp`: help text cleaned

### Phase 12 — Account API
- [x] `account.hh` / `account.cpp`: `get_legacy_devicelist()` renamed to `get_devicelist()`;
       OMEMO:2 `get_devicelist()` removed

### Phase 13 — Build & tests
- [x] `make` — clean build, no errors
- [x] All 27 unit tests pass (27/27)
- [x] Fixed stale `.cov.o` ODR mismatch (touch `src/omemo/api.cpp` before test run)

---

## Remaining / Future Work

- [ ] `README.org` — update feature list, trust model, command docs
- [ ] `DOAP.xml` — update XEP-0384 status, remove XEP-0450
- [ ] Consider removing `omemo_atm` config option entirely (separate PR)
- [ ] Consider adding BTBV fingerprint display to `/omemo devices` output
