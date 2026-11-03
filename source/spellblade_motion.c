#include "spellblade_motion.h"

static int finite_range(float v, float limit) { return v == v && v >= -limit && v <= limit; }
static int valid_vec(SBVec3 v, float limit) {
    return finite_range(v.x, limit) && finite_range(v.y, limit) && finite_range(v.z, limit);
}
static SBVec3 cross(SBVec3 a, SBVec3 b) {
    SBVec3 v = {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
    return v;
}

int sb_blade_pose(SBBladePose* out, SBVec3 base, SBQuat rotation, SBVec3 local_tip)
{
    SBVec3 q, t, u;
    float norm;
    if (!out || !valid_vec(base, 10000000.0f) || !valid_vec(local_tip, 10000.0f)) return 0;
    if (!finite_range(rotation.w, 2.0f) || !finite_range(rotation.x, 2.0f) ||
        !finite_range(rotation.y, 2.0f) || !finite_range(rotation.z, 2.0f)) return 0;
    norm = rotation.w*rotation.w + rotation.x*rotation.x + rotation.y*rotation.y + rotation.z*rotation.z;
    if (!(norm >= 0.98f && norm <= 1.02f)) return 0;
    q.x=rotation.x; q.y=rotation.y; q.z=rotation.z;
    t=cross(q, local_tip); u=cross(q, t);
    out->base=base;
    out->tip.x=base.x+local_tip.x+2.0f*(rotation.w*t.x+u.x);
    out->tip.y=base.y+local_tip.y+2.0f*(rotation.w*t.y+u.y);
    out->tip.z=base.z+local_tip.z+2.0f*(rotation.w*t.z+u.z);
    return valid_vec(out->tip, 10010000.0f);
}

void sb_motion_reset(SBMotion* motion) { if (motion) motion->has_previous = 0; }

int sb_motion_step(SBMotion* motion, const SBBladePose* pose, float dt)
{
    float dx, dy, dz, d2;
    if (!motion || !pose || !valid_vec(pose->base, 10000000.0f) ||
        !valid_vec(pose->tip, 10010000.0f) || !(dt > 0.0f && dt <= 0.25f)) {
        sb_motion_reset(motion);
        return -1;
    }
    if (!motion->has_previous) { motion->previous = *pose; motion->has_previous = 1; return 0; }
    dx=pose->tip.x-motion->previous.tip.x;
    dy=pose->tip.y-motion->previous.tip.y;
    dz=pose->tip.z-motion->previous.tip.z;
    d2=dx*dx+dy*dy+dz*dz;
    motion->previous = *pose;
    /* Conservative defaults to be calibrated in-game, not damage logic. */
    if (d2 > 40000.0f) { motion->has_previous = 0; return -1; }
    return d2 > 0.25f ? 1 : 0;
}
