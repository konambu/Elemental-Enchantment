#ifndef SPELLBLADE_MOTION_H
#define SPELLBLADE_MOTION_H

typedef struct SBVec3 { float x, y, z; } SBVec3;
typedef struct SBQuat { float w, x, y, z; } SBQuat;
typedef struct SBBladePose { SBVec3 base, tip; } SBBladePose;
typedef struct SBMotion { SBBladePose previous; int has_previous; } SBMotion;

/* Caller supplies measured attachment transforms. This module retains no game pointers. */
int sb_blade_pose(SBBladePose* out, SBVec3 base, SBQuat rotation, SBVec3 local_tip);
/* Returns 1 for a plausible moving blade, 0 for still/first sample, -1 for invalid/teleport. */
int sb_motion_step(SBMotion* motion, const SBBladePose* pose, float dt);
void sb_motion_reset(SBMotion* motion);

#endif
