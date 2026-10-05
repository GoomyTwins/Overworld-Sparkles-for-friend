# Integration Notes

These snippets show the finished implementation hooks with all Grotto-specific naming removed.

## Field-effect helper declarations

Add to `include/field_effect_helpers.h`:

```c
void UpdatePersistentShinySparkle(struct Sprite *sprite);
void StartPersistentShinySparkles(void);
```

## Persistent sparkle lifecycle

Add to `src/field_effect_helpers.c`:

```c
void UpdatePersistentShinySparkle(struct Sprite *sprite)
{
    if (sprite->animEnded)
        DestroySprite(sprite);
}
```

## Palette tag

Add a free field-effect palette tag to `include/constants/field_effects.h`:

```c
#define FLDEFF_PAL_TAG_PERSISTENT_GLINT 0x1015
```

Change the numeric value if that tag is already occupied in the target fork.

## Graphics registration

Add to `src/data/object_events/object_event_graphics.h`:

```c
const u16 gFieldEffectPalette_PersistentGlint[] =
    INCGFX_U16("graphics/field_effects/palettes/persistent_shiny_glint.pal", ".gbapal");

const u32 gFieldEffectObjectPic_PersistentShinyGlint[] =
    INCGFX_U32("graphics/field_effects/pics/persistent_shiny_glint.png", ".4bpp", "-mwidth 1 -mheight 1");
```

## Sprite template and exact animation timing

Add to `src/data/field_effects/field_effect_objects.h`:

```c
static const struct SpriteFrameImage sPicTable_PersistentShinyGlint[] = {
    overworld_frame(gFieldEffectObjectPic_PersistentShinyGlint, 1, 1, 0),
    overworld_frame(gFieldEffectObjectPic_PersistentShinyGlint, 1, 1, 1),
    overworld_frame(gFieldEffectObjectPic_PersistentShinyGlint, 1, 1, 2),
    overworld_frame(gFieldEffectObjectPic_PersistentShinyGlint, 1, 1, 3),
};

static const union AnimCmd sAnim_PersistentShinyGlint[] = {
    ANIMCMD_FRAME(0, 3),
    ANIMCMD_FRAME(1, 4),
    ANIMCMD_FRAME(2, 4),
    ANIMCMD_FRAME(3, 4),
    ANIMCMD_END,
};

static const union AnimCmd *const sAnimTable_PersistentShinyGlint[] = {
    sAnim_PersistentShinyGlint,
};

const struct SpritePalette gSpritePalette_PersistentGlint = {
    gFieldEffectPalette_PersistentGlint,
    FLDEFF_PAL_TAG_PERSISTENT_GLINT,
};

const struct SpriteTemplate gFieldEffectObjectTemplate_PersistentShinySparkle = {
    .tileTag = TAG_NONE,
    .paletteTag = FLDEFF_PAL_TAG_PERSISTENT_GLINT,
    .oam = &gObjectEventBaseOam_8x8,
    .anims = sAnimTable_PersistentShinyGlint,
    .images = sPicTable_PersistentShinyGlint,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = UpdatePersistentShinySparkle,
};
```

## Controller

Compile `src/persistent_shiny_sparkle.c` and expose:

```c
void StartPersistentShinySparkles(void);
```

The target test uses the project's existing overworld Pokémon encoding:

```c
(objectEvent->graphicsId & OBJ_EVENT_MON) && OW_SHINY(objectEvent)
```

If the target follower system stores shiny state differently, adapt only `IsPersistentShinySparkleTarget()`.

## Follower graphics/state update hook

Immediately after follower graphics are updated:

```c
FollowerSetGraphics(objEvent, species, shiny, female);
objEvent->invisible = TRUE;

if (shiny)
    StartPersistentShinySparkles();
```

Keeping an existing task alive while the follower is temporarily normal/non-shiny is intentional. Switching back to shiny resumes the stream cleanly.

## Area-transition hooks

After `UpdateFollowingPokemon()` in both the local return-to-field path and normal object-event initialization path:

```c
UpdateFollowingPokemon();
StartPersistentShinySparkles();
```

Starting the sparkle controller after the follower has been rebuilt lets it bind to the new ObjectEvent/sprite instead of a stale sprite slot.

## Preserved behavior

- 32 random placement attempts
- 15% preference for edge-adjacent pixels
- 4 concurrent rolling slots
- 12-frame spawn cadence
- 10 px anti-clump distance
- 12 px anti-clump distance for sprites at least 64 px wide or tall
- sprite-ID / graphics-ID rebinding after rebuilds
- suppression while the target ObjectEvent is invisible
