# Installed control providers

`controls_provider.hpp` defines the versioned x86 C ABI. Export
`const fc_provider::Provider* __cdecl FC_ControlsProviderV1()` from the installed
NVSE addon. Return an immutable process-lifetime descriptor with ABI 1 and its
exact size. The host discovers already-loaded modules; it never loads arbitrary
DLLs or enables a disabled MO2 mod.

Each mode has a stable lower-case dotted ID, title, description and 26 labels,
corresponding to the native channels / addon actions in `controls_model.hpp`.
A null label means the provider does not consume that destination. Unused destinations are omitted from the live and desktop editors. These are remapped channels, not unverified semantic hooks.
The current ABI does not add new output channels or spatial actions.

`available()` returns a bit per declared mode once required native dependencies
are ready. `current()` returns the active mode index, or -1 when unmounted.
Callbacks must read atomics only, without game calls, locks or file I/O. They can
run on the XR input thread. If overlapping providers report active modes, the
first discovered mode wins; providers should coordinate exclusive ownership.
Descriptors and their modules must remain loaded for the process lifetime.

The Vehicles addon advertises `vehicles.driving` and `vehicles.flight` even when
neither is active. The UI can edit them while on foot, exports the installed
catalog for offline editing, and switches routing when the provider reports the
mode. The core preserves absent-mode profiles across saves. Mode changes flush
virtual output and require neutral inputs before gameplay resumes.

Profile v3: header `FilthyCasualControls 3`, mode count, then for each stable mode
ID exactly 26 rows: row, source, modifier, gesture, delay-ms. `onfoot` is required.
Parsing is strict, bounded and transactional. v1/v2 migrate their former global
mapping into discovered modes. No player save changes or dependency assets.
