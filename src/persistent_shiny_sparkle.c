#include "global.h"
#include "event_object_movement.h"
#include "field_effect.h"
#include "field_effect_helpers.h"
#include "palette.h"
#include "random.h"
#include "sprite.h"
#include "task.h"

#include "constants/event_objects.h"

extern const struct SpriteTemplate gFieldEffectObjectTemplate_PersistentShinySparkle;
extern const struct SpritePalette gSpritePalette_PersistentGlint;

#define PERSISTENT_SPARKLE_SLOT_COUNT 4
#define PERSISTENT_SPARKLE_MARKER     0x4C53
#define PERSISTENT_SPARKLE_PERIOD     12

static bool8 IsPersistentShinySparkleSprite(u8 spriteId);

static bool8 IsPersistentSparklePixelVisible(
    const struct Sprite *sprite, s16 x, s16 y)
{
    u16 width, height, tile, pixel;
    const u8 *graphics;
    u8 value;

    if (sprite->oam.affineMode != ST_OAM_AFFINE_OFF
     || sprite->oam.bpp != ST_OAM_4BPP)
        return FALSE;

    width = gOamDimensions[sprite->oam.shape][sprite->oam.size].width;
    height = gOamDimensions[sprite->oam.shape][sprite->oam.size].height;
    if (x < 0 || y < 0 || x >= width || y >= height)
        return FALSE;

    if (sprite->oam.matrixNum & 8)
        x = width - 1 - x;
    if (sprite->oam.matrixNum & 16)
        y = height - 1 - y;

    tile = (y / 8) * (width / 8) + x / 8;
    pixel = (y % 8) * 8 + x % 8;
    graphics = (const u8 *)OBJ_VRAM0
             + (sprite->oam.tileNum + tile) * TILE_SIZE_4BPP;
    value = graphics[pixel / 2];

    return ((pixel & 1 ? value >> 4 : value) & 15) != 0;
}

static bool8 IsPersistentSparkleEdgePixel(
    const struct Sprite *sprite, s16 x, s16 y)
{
    s16 dx, dy;

    if (IsPersistentSparklePixelVisible(sprite, x, y))
        return FALSE;

    for (dy = -2; dy <= 2; dy++)
    {
        for (dx = -2; dx <= 2; dx++)
        {
            if (dx * dx + dy * dy <= 4
             && IsPersistentSparklePixelVisible(sprite, x + dx, y + dy))
                return TRUE;
        }
    }

    return FALSE;
}

static bool8 PickPersistentSparklePosition(
    const struct Sprite *sprite,
    const struct ObjectEventGraphicsInfo *graphicsInfo,
    const struct Task *task,
    s16 *x,
    s16 *y)
{
    u16 width = graphicsInfo->width;
    u16 height = graphicsInfo->height;
    s16 minSeparation = 10;
    u8 i;

    if (width >= 64 || height >= 64)
        minSeparation = 12;

    for (i = 0; i < 32; i++)
    {
        s16 px = Random() % width;
        s16 py = Random() % height;
        s16 candidateX;
        s16 candidateY;
        bool8 wantEdge = (Random() % 100) < 15;
        bool8 tooClose = FALSE;
        u8 slot;

        if (wantEdge)
        {
            if (!IsPersistentSparkleEdgePixel(sprite, px, py))
                continue;
        }
        else if (!IsPersistentSparklePixelVisible(sprite, px, py))
        {
            continue;
        }

        candidateX = px - width / 2;
        candidateY = py - height / 2;

        for (slot = 0; slot < PERSISTENT_SPARKLE_SLOT_COUNT; slot++)
        {
            s16 sparkleId = task->data[1 + slot];
            s16 sparkleX;
            s16 sparkleY;
            s16 dx;
            s16 dy;

            if (!IsPersistentShinySparkleSprite(sparkleId))
                continue;

            sparkleX = gSprites[sparkleId].x - sprite->x - sprite->x2;
            sparkleY = gSprites[sparkleId].y - sprite->y - sprite->y2;
            dx = candidateX - sparkleX;
            dy = candidateY - sparkleY;

            if (dx * dx + dy * dy < minSeparation * minSeparation)
            {
                tooClose = TRUE;
                break;
            }
        }

        if (tooClose)
            continue;

        *x = candidateX;
        *y = candidateY;
        return TRUE;
    }

    return FALSE;
}

static u8 SpawnPersistentShinySparkle(s16 x, s16 y, u8 subpriority)
{
    u32 spriteId;

    FieldEffect_LoadFadedPalette(
        (struct SpritePalette *)&gSpritePalette_PersistentGlint,
        COLOR_MAP_DARK_CONTRAST);

    spriteId = CreateSpriteAtEndUnchecked(
        &gFieldEffectObjectTemplate_PersistentShinySparkle,
        x, y, subpriority);

    if (spriteId == MAX_SPRITES)
        return MAX_SPRITES;

    gSprites[spriteId].coordOffsetEnabled = TRUE;
    gSprites[spriteId].data[7] = PERSISTENT_SPARKLE_MARKER;
    return spriteId;
}

static struct Sprite *GetPersistentShinyObjectSprite(u8 objectEventId)
{
    struct ObjectEvent *objectEvent;

    if (objectEventId >= OBJECT_EVENTS_COUNT)
        return NULL;

    objectEvent = &gObjectEvents[objectEventId];

    if (!objectEvent->active
     || objectEvent->spriteId >= MAX_SPRITES
     || !gSprites[objectEvent->spriteId].inUse)
        return NULL;

    return &gSprites[objectEvent->spriteId];
}

static bool8 IsPersistentShinySparkleTarget(u8 objectEventId)
{
    struct ObjectEvent *objectEvent;

    if (objectEventId >= OBJECT_EVENTS_COUNT)
        return FALSE;

    objectEvent = &gObjectEvents[objectEventId];

    return objectEvent->active
        && (objectEvent->graphicsId & OBJ_EVENT_MON)
        && OW_SHINY(objectEvent);
}

static bool8 IsPersistentShinySparkleSprite(u8 spriteId)
{
    return spriteId < MAX_SPRITES
        && gSprites[spriteId].inUse
        && gSprites[spriteId].template ==
           &gFieldEffectObjectTemplate_PersistentShinySparkle
        && gSprites[spriteId].data[7] == PERSISTENT_SPARKLE_MARKER;
}

static void DestroyPersistentShinySparkleSlot(s16 *spriteId)
{
    if (*spriteId < MAX_SPRITES
     && IsPersistentShinySparkleSprite(*spriteId))
        DestroySprite(&gSprites[*spriteId]);

    *spriteId = MAX_SPRITES;
}

/*
 * data[0] = spawn timer
 * data[1..4] = active sparkle sprite IDs
 * data[5] = object event ID
 * data[6] = object event sprite ID
 * data[7] = object event graphics ID
 * data[8] = next sparkle slot
 */
static void Task_PersistentShinySparkle(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    u8 objectEventId = task->data[5];
    struct ObjectEvent *objectEvent;
    struct Sprite *targetSprite;
    const struct ObjectEventGraphicsInfo *graphicsInfo;
    s16 *slot;
    s16 offsetX;
    s16 offsetY;
    u8 i;

    if (objectEventId >= OBJECT_EVENTS_COUNT
     || !gObjectEvents[objectEventId].active)
    {
        for (i = 0; i < PERSISTENT_SPARKLE_SLOT_COUNT; i++)
            DestroyPersistentShinySparkleSlot(&task->data[1 + i]);

        DestroyTask(taskId);
        return;
    }

    objectEvent = &gObjectEvents[objectEventId];

    if (!IsPersistentShinySparkleTarget(objectEventId)
     || objectEvent->invisible)
    {
        for (i = 0; i < PERSISTENT_SPARKLE_SLOT_COUNT; i++)
            DestroyPersistentShinySparkleSlot(&task->data[1 + i]);

        return;
    }

    targetSprite = GetPersistentShinyObjectSprite(objectEventId);
    if (targetSprite == NULL)
        return;

    if (task->data[6] != objectEvent->spriteId
     || task->data[7] != objectEvent->graphicsId)
    {
        for (i = 0; i < PERSISTENT_SPARKLE_SLOT_COUNT; i++)
            DestroyPersistentShinySparkleSlot(&task->data[1 + i]);

        task->data[6] = objectEvent->spriteId;
        task->data[7] = objectEvent->graphicsId;
        task->data[0] = 0;
        task->data[8] = 0;
        return;
    }

    for (i = 0; i < PERSISTENT_SPARKLE_SLOT_COUNT; i++)
    {
        if (task->data[1 + i] < MAX_SPRITES
         && !IsPersistentShinySparkleSprite(task->data[1 + i]))
            task->data[1 + i] = MAX_SPRITES;
    }

    if (++task->data[0] < PERSISTENT_SPARKLE_PERIOD)
        return;

    task->data[0] = 0;
    slot = &task->data[1 + (task->data[8] & 3)];

    if (*slot < MAX_SPRITES)
        return;

    graphicsInfo = GetObjectEventGraphicsInfo(objectEvent->graphicsId);

    if (!PickPersistentSparklePosition(
            targetSprite,
            graphicsInfo,
            task,
            &offsetX,
            &offsetY))
        return;

    *slot = SpawnPersistentShinySparkle(
        targetSprite->x + targetSprite->x2 + offsetX,
        targetSprite->y + targetSprite->y2 + offsetY,
        targetSprite->subpriority > 0
            ? targetSprite->subpriority - 1
            : 0);

    if (*slot < MAX_SPRITES)
        task->data[8] = (task->data[8] + 1) & 3;
}

static bool8 IsPersistentShinySparkleTaskForObject(
    u8 taskId, u8 objectEventId)
{
    return gTasks[taskId].func == Task_PersistentShinySparkle
        && gTasks[taskId].data[5] == objectEventId;
}

static bool8 StartPersistentShinySparkleForObject(u8 objectEventId)
{
    struct Sprite *targetSprite;
    u8 taskId;

    if (!IsPersistentShinySparkleTarget(objectEventId))
        return FALSE;

    targetSprite = GetPersistentShinyObjectSprite(objectEventId);
    if (targetSprite == NULL)
        return FALSE;

    for (taskId = 0; taskId < NUM_TASKS; taskId++)
    {
        if (IsPersistentShinySparkleTaskForObject(taskId, objectEventId))
            return TRUE;
    }

    taskId = CreateTask(Task_PersistentShinySparkle, 81);

    gTasks[taskId].data[0] = 0;
    gTasks[taskId].data[1] = MAX_SPRITES;
    gTasks[taskId].data[2] = MAX_SPRITES;
    gTasks[taskId].data[3] = MAX_SPRITES;
    gTasks[taskId].data[4] = MAX_SPRITES;
    gTasks[taskId].data[5] = objectEventId;
    gTasks[taskId].data[6] = gObjectEvents[objectEventId].spriteId;
    gTasks[taskId].data[7] = gObjectEvents[objectEventId].graphicsId;
    gTasks[taskId].data[8] = 0;

    return TRUE;
}

void StartPersistentShinySparkles(void)
{
    u8 objectEventId;

    for (objectEventId = 0;
         objectEventId < OBJECT_EVENTS_COUNT;
         objectEventId++)
    {
        StartPersistentShinySparkleForObject(objectEventId);
    }
}
