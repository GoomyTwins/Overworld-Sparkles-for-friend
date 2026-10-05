# Overworld Sparkles for Friend

Standalone export of the persistent, sprite-aware shiny overworld sparkle system developed for a Pokémon Emerald decomp project.

This repository intentionally contains only the reusable sparkle feature and its assets. It has no Git ancestry, submodule, branch relationship, or access path to the private source repository it was extracted from.

Included behavior:
- persistent sparkle streams for shiny overworld Pokémon / followers
- sprite-pixel-aware placement
- edge-biased placement
- anti-clumping between active particles
- four-slot rolling sparkle cadence
- follower graphics/state transition handling
- area / return-to-field restart hooks
- the exact 4-frame sparkle asset and palette

The integration snippets are written for a pokeemerald-expansion-style codebase and may require small context-specific adjustments depending on the target fork.
