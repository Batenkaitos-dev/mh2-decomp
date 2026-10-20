#ifndef SHADOW_H
#define SHADOW_H

/*
 * The shadows drawn under players and monsters: stage objects of kind 1
 * (Shadow_Spawn), drawn by Shadow_Draw. The type is the shadow's shape.
 */

enum ShadowType {
    SHADOW_MODEL = 0,        /* the plain shadow model under the owner (most monsters) */
    SHADOW_PLAYER = 1,       /* a player's: two part shadows under its parts 5 and 8, turned away from the sky's far point */
    SHADOW_PARTS = 2,        /* five part shadows, one under each of five bones of the owner */
    SHADOW_GROUND_BONES = 3, /* the shadow model's bones laid on the ground one by one (Bone_SnapToGround) */
    SHADOW_OWN_MODEL = 4     /* the monster's own model set, drawn as its shadow */
};

#endif
