#pragma once
#include <vector>
#include "velocityProfile/sinusoidalVelocityProfile.hpp"
using namespace Pronounce;
std::vector<std::pair<PathPlanner::BezierSegment, Pronounce::SinusoidalVelocityProfile*>> ChangeMe = {{PathPlanner::BezierSegment(
PathPlanner::Point(9.88701_in, 79.096_in),
PathPlanner::Point(9.88701_in, 19.774_in),
PathPlanner::Point(24.7175_in, 14.8305_in),
PathPlanner::Point(39.548_in, 9.88701_in)
,false),
{(50_in/second).getValue(), (50_in/second/second), (50_in/second/second)},
};
// PathPlanner made path
