#pragma once

#ifndef M_PI
#define M_PI       3.14159265358979323846f
#endif /* M_PI */

#define RADX10 (M_PI / 1800.0f)                  // 0.001745329252f
#define RAD    (M_PI / 180.0f)

int applyDeadband(int value, int deadband);

int constrain(int amt, int low, int high);
int constrainf(int amt, int low, int high);

#define min(a, b) ((int)(a) < (int)(b) ? (int)(a) : (int)(b))
#define max(a, b) ((int)(a) > (int)(b) ? (int)(a) : (int)(b))
#define abs(x) ((x) > 0 ? (x) : -(x))

#ifndef sq
#define sq(x) ((x)*(x))
#endif


