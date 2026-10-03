# Apollo final polish audit

## Family references

The Bubbles and Nimbus repositories establish a restrained family grammar:
Montserrat provides a small regular/medium/semibold hierarchy separating values, labels, and section
names; controls use consistent label/value baselines; and interaction states
change contrast or accent colour rather than adding decoration. Apollo carries
those conventions into its existing ivory-and-graphite instrument architecture
instead of copying either sibling's composition.

## Applied visual system

- The pinned upstream Montserrat 7.222 release supplies the four font payloads
  at configure time. JUCE embeds them in the plugin and caches each typeface;
  no host font installation is consulted at runtime.
- The wordmark uses Bold with deliberate tracking, while section names use
  SemiBold, labels and selectors use Medium, and values use Regular.
- Shared type sizes, hairline width, and corner radius are defined in
  `ApolloTheme::Metrics` so future controls follow the same hierarchy.
- Knob captions and value rows use common heights and offsets. Tone endpoints
  use the compact `HI CUT / FLAT / LO CUT` convention.
- Popup menus, tooltips, selectors, buttons, readouts, and status text all use
  the shared family. The APOLLO header gains authority through weight,
  tracking, vertical alignment, and one short orange datum line rather than a
  larger decorative enclosure.
- Existing flat, low-profile knobs, segmented selectors, ivory chassis,
  graphite work surface, restrained orange accents, and structural panel
  divisions remain unchanged because they are Apollo's spacecraft-instrument
  identity.

## Control and state audit

All 15 public parameters remain represented by visible, focusable controls and
APVTS attachments. Their IDs, version hints, ranges, defaults, choice order,
automation paths, bidirectional attachment behaviour, and XML state recall are
covered by `test_ui.cpp`. No engine-only tuning value has clear enough musical
value to add to this deliberately concise surface, so no new parameter was
exposed. The polish does not change DSP, parameter creation, or serialization.

The existing choices were deliberately retained: the mix control remains a
vertical DRY/WET fader; Perform remains a momentary automation-aware gate; and
octave controls remain editable while visually dimmed. These behaviours convey
their musical roles more clearly than replacing them with family-generic
widgets would.
