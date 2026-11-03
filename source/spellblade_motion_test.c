#include "spellblade_motion.h"
int _fltused = 0;

__declspec(dllexport) int sb_run_motion_tests(void)
{
    SBBladePose p;
    SBMotion m = {0};
    SBVec3 base = {1,2,3}, tip = {0,10,0};
    SBQuat identity = {1,0,0,0}, half_turn = {0,0,0,1}, invalid = {0,0,0,0};
    if (!sb_blade_pose(&p, base, identity, tip) || p.tip.y != 12) return 1;
    if (sb_motion_step(&m, &p, .016f) != 0) return 2;
    if (sb_motion_step(&m, &p, .016f) != 0) return 3;
    if (!sb_blade_pose(&p, base, half_turn, tip) || p.tip.y != -8) return 4;
    if (sb_motion_step(&m, &p, .016f) != 1) return 5;
    p.tip.x += 1000;
    if (sb_motion_step(&m, &p, .016f) != -1 || m.has_previous) return 6;
    if (sb_blade_pose(&p, base, invalid, tip)) return 7;
    if (sb_motion_step(&m, &p, 0.0f) != -1) return 8;
    return 0;
}
