// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// XMPP_TEST_EXPORT
// ────────────────
// Marks a function as having default (exported) symbol visibility so that
// the doctest binary can call it from xmpp.cov.so even though the rest of
// the plugin is compiled with -fvisibility=hidden.
//
// Applied only to the small set of pure/near-pure functions that have unit
// tests in tests/unit.inl.  Removed by setting XMPP_NO_TEST_EXPORT (e.g.
// in a release / hardened build where you want no extra exports).

#pragma once

#if defined(XMPP_NO_TEST_EXPORT)
#  define XMPP_TEST_EXPORT
#else
#  define XMPP_TEST_EXPORT __attribute__((visibility("default")))
#endif
