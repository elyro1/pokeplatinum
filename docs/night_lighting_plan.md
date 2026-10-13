# Night lighting and dynamic shadows plan

Status: stages 0–2 implemented, with prototype glow attachments. Stages 3–4
remain proposed. Build on `pokeplatinum-visual` and
`codex/libntr-visual`; keep Android integration separate until the desktop
prototype is validated.

## Current behavior and confirmed issues

Day/night interpolates the original material and light-color templates in
`src/overlay005/area_light.c`, then applies a world tint and atmospheric haze.
It is more than a color filter, but does not recalculate cast shadows. The recent
outdoor key direction stays northwest to agree with existing map shadows.

The previous soft character shadow used sampled collision height and a +5
world-unit Z offset, while the original used its animation position, a -4 Y
offset, a +1 Z offset and model geometry/scale. The alignment fix now passes the
original render anchor and time-dependent scale to the soft shadow and derives
its center/radii from the original model bounds. It no longer resamples terrain
or adds a separate offset. Paired on/off captures were checked in game. Water
profiling, coherent night state and prototype emission/ground pools are now
implemented. Dynamic local relighting and cast shadows remain stages 3–4.

Water shimmer now covers visible loaded tiles, with a ceiling of 2048 quads,
eight phase groups and at most 16 interaction rings. The benchmark shoreline/open-water views
use 65/337 tiles: 130/674 triangles plus rings. CPU costs include collision scanning,
height lookup, hashing and geometry submission; GPU costs
include translucent coverage, wave calculations and blending at the internal
render resolution. Batching reduces draw calls, not pixel work. A controlled Linux on/off benchmark now measures CPU and GPU results at 2×,
3× and 10×; see `water_performance.md`.

## Intended appearance

At night, lower cool ambient light replaces daylight; bright warm windows and
lamps remain visible and illuminate nearby pavement and characters. Sun-cast
shadows fade out at dusk. Weak moonlight may cast subdued shadows on clear
nights. Lamps cast their own shadows where enabled. Compact contact shadows
remain under feet to keep characters grounded, with opacity based on ambient
and nearby light. Night does not mean removing every shadow.

Retain nearest sampling and the game's pixel-art textures. Emission is applied
separately from daylight tint; optional restrained bloom affects emissive
sources only. UI, menus and text do not receive scene lighting or bloom.

## Implementation sequence

| Stage | Work | Effort | Visual impact |
| --- | --- | --- | --- |
| 0 | Match shadow footprint and establish performance measurements | Small–medium | Medium; essential foundation |
| 1 | Separate ambient, sun/moon, contact shadow and emission controls | Medium | High |
| 2 | Nighttime windows and lamp emission, with grounded light pools | Medium | High |
| 3 | Local dynamic lights on surfaces and characters | Large | Very high |
| 4 | Cast shadows and replacement of authored shadow assets | Very large | High, with the most compatibility risk |

### Stage 0: alignment and profiling

1. Capture the default and replacement shadows with an identical camera, time,
   object transform and animation frame. Include north/south/east/west-facing
   player sprites, several NPC sizes, slopes, stairs and jumping.
2. Derive replacement geometry from the original shadow's actual transform and
   local footprint, including the special field-effect projection. Preserve
   visibility flags and special shadow variants. Compare visible centroids,
   extents and ground placement, not just map-object coordinates.
3. Measure water on/off on the same saved scene and deterministic camera path.
   Warm up, disable frame caps for measurements, and collect CPU and GPU
   timings separately, including percentile spikes. Compare shoreline and
   open lake at 2×, 3× and 10× resolution, with AA unchanged.
4. If CPU-bound, scan perimeters without repeatedly walking their interiors,
   precompute phase groups, and replace per-tile immediate-command construction
   with a reusable GPU vertex/instance buffer. If GPU-bound, reduce expensive
   glint math and introduce a lower-resolution water-effects pass or quality
   setting. Track shoreline correctness and ripple blending after optimization.

Deliverable: aligned shadow comparison and a table of measured milliseconds,
not an assumed negligible cost or capped FPS alone.

### Stage 1: a coherent night lighting state

Create a shared rendering-only lighting state derived from preview/real time:
ambient color/intensity, sun color/intensity, moon color/intensity, emission
activation and directional shadow strength. Keep gameplay time untouched.

Blend across dusk/dawn smoothly. Daylight shadow strength follows sun intensity;
contact occlusion is controlled independently. Initially keep the northwest
source direction to preserve compatibility with authored shadows. Moving sun
and moon directions wait until replacement cast shadows are available.

Audit representative maps to identify whether ground shadows are separate
polygons/materials, painted into textures, or a mixture. Fade separately tagged
shadow geometry at night. Painted shadows require explicit masks or clean
texture variants; do not classify arbitrary dark pixels as shadows. Some assets
will therefore retain old shadows until their authoring work is done.

### Stage 2: emission and visible nighttime sources

Audit existing nighttime texture/palette/animation behavior before adding new
emission so lights are not duplicated. Define reusable metadata by prop model
and material: emissive window/lamp regions, local attachment position, color,
intensity, radius and activation schedule. Allow map-specific overrides.

Render tagged windows and lamps as emission that survives reduced ambient
lighting. Add small depth-tested halos and terrain-aware light pools, fading
on at dusk and off at dawn. Light pools are a first visual milestone; they do
not yet imply that walls or characters are actually being relit.

Prototype on one town block, then apply reusable prop definitions elsewhere.
Keep bloom optional and subtle; bright edges should remain readable.

### Stage 3: actual local lighting

The current backend transforms geometry and evaluates normals on the CPU before
submission. The existing world fragment shader primarily receives clip-space
position, UVs and prelit vertex colors. Accurate local lighting requires retaining
world positions and normals, plus the relevant material/lighting flags; world
position alone is not enough.

Add an optional native lighting path in libntr that preserves this information.
Keep the legacy path available for compatibility. Preserve unlit/emissive
materials and authored vertex colors rather than lighting them twice.

Use a culled list of nearby point/spot lights with smooth radius falloff. Start
with a fixed small per-draw light limit. Treat sprite characters explicitly:
sample local lighting around their feet/body with an approximate billboard
normal or lightweight sprite-lighting model, retaining alpha cutouts and
pixel-art colors. Normal-mapped character art is a later optional asset task.

Deliverable: walking toward a lamp visibly warms both the pavement and the
character; walls receive illumination appropriate to their orientation.

### Stage 4: cast shadows

Capture world-space geometry and classify casters/receivers: opaque buildings,
terrain and props; alpha-cutout trees; separate character proxies. Exclude UI,
glow halos, water shimmer and existing shadow decals from shadow casting.

Start with one bounded, texel-stabilized orthographic shadow map for the sun or
moon. Use configurable resolution, depth bias and modest filtering. Cache
static geometry while invalidating when loaded map cells change; update moving
objects separately. Use character capsules or simple proxies first to avoid
unreliable silhouettes from camera-facing sprites.

Add local-light shadows only after directional shadows are stable. A spot lamp
can use one shadow projection; a genuinely omnidirectional point light requires
multiple views, such as a cubemap. Limit the number of shadowed local lights and
cache stationary ones. A glow halo alone must not be presented as dynamic
occlusion.

Disable tagged authored shadows only where their replacements are ready. Use
explicit texture masks/variants for painted shadows. Avoid double shadows.
Once asset replacement is complete for the prototype region, allow sun/moon
movement and make cast shadows follow the actual active light.

## Validation and performance targets

Use a town block with lamps, windows, trees and NPCs as the primary scene, plus
Lake Verity, a forest and a normal cave. Test dusk/dawn and night previews,
walking past a lamp, an NPC moving behind a building, terrain elevation, wide
views, streamed map boundaries and battle/menu transitions. Preserve Flash,
authored fog, protected weather and scripted scene overrides.

Debug controls should show emission only, light bounds, shadow maps and a
fixed-frame time sweep, alongside individual effect toggles and CPU/GPU timings.

A 60 FPS target has a 16.67 ms total frame budget. Set graphics budgets using the
measured baseline on the target machine. A provisional desktop goal is no more
than about 2 ms of additional GPU time for the combined new lighting/shadow path
at 3× resolution; this is a design target, not a demonstrated result. Keep 10×
and lower-powered/mobile hardware as separate quality/performance cases.

Suggested quality levels:

- Low: improved ambient/emission, aligned contact shadows, no cast-shadow maps.
- Medium: nearby dynamic lights and one directional shadow map.
- High: directional shadows plus a tightly limited number of shadowed lamps.

## Delivered stages 0–2

- The alignment fix continues to use the original character-shadow transform
  and footprint; the new contact-shadow strength varies independently of the sun.
- Water uses a single candidate scan and native batched geometry submission,
  with opt-in asynchronous timing and a reproducible copied-save benchmark.
- A shared render-time night state controls ambient fill, sun/moon strength,
  source activation and shadow opacity. Explicit shadow materials fade at night.
- Tagged glass is emissive; colored light materials preserve source texture hue.
  Reusable house-window attachments and map-specific Jubilife lamp attachments
  add depth-tested halos and ground pools. Emission-only/debug timing controls
  help inspect the output. Initial locations are documented in `visual_upgrades.md`.
- Material overrides are active only within field rendering, preserve existing
  model callbacks, and reset on scene changes. Gameplay time and save data stay
  independent of preview time.

The implementation preserves painted/untagged shadows until explicit masks or
asset replacements are authored. It adds cosmetic source emission and pools;
actual local illumination of characters/walls and new cast shadows require
stages 3–4. Emissive bloom was added afterwards and replaced the per-source
halo and trim-ribbon billboards; see `visual_upgrades.md`.

## Recommended first milestone

Complete stages 0–2 on one town block before expanding the renderer. This gives
aligned shadows, a measured water cost, darker coherent nights and visible
warm windows/lamps. Then use that same block to validate real local lighting and
cast shadows in stages 3–4. Avoid a whole-game shadow rewrite before the asset
and performance audit establishes what can be replaced reliably.
