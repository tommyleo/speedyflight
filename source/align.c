#include "board.h"
#include "mw.h"

#include "align.h"

static bool standardBoardAlignment = true;     // board orientation correction
static float boardRotation[3][3];              // matrix

void boardAlignmentInit(void)
{
    float roll, pitch, yaw;
    float cosx, sinx, cosy, siny, cosz, sinz;
    float coszcosx, coszcosy, sinzcosx, coszsinx, sinzsinx;

    // standard alignment, nothing to calculate
    if (!mcfg.board_align_roll && !mcfg.board_align_pitch && !mcfg.board_align_yaw)
        return;

    standardBoardAlignment = false;

    // deg2rad
    roll = mcfg.board_align_roll * M_PI / 180.0f;
    pitch = mcfg.board_align_pitch * M_PI / 180.0f;
    yaw = mcfg.board_align_yaw * M_PI / 180.0f;

    cosx = cosf(roll);
    sinx = sinf(roll);
    cosy = cosf(pitch);
    siny = sinf(pitch);
    cosz = cosf(yaw);
    sinz = sinf(yaw);

    coszcosx = cosz * cosx;
    coszcosy = cosz * cosy;
    sinzcosx = sinz * cosx;
    coszsinx = sinx * cosz;
    sinzsinx = sinx * sinz;

    // define rotation matrix
    boardRotation[0][0] = coszcosy;
    boardRotation[0][1] = -cosy * sinz;
    boardRotation[0][2] = siny;

    boardRotation[1][0] = sinzcosx + (coszsinx * siny);
    boardRotation[1][1] = coszcosx - (sinzsinx * siny);
    boardRotation[1][2] = -sinx * cosy;

    boardRotation[2][0] = (sinzsinx) - (coszcosx * siny);
    boardRotation[2][1] = (coszsinx) + (sinzcosx * siny);
    boardRotation[2][2] = cosy * cosx;
}

void alignBoard(int16_t *vec)
{
    int16_t x = vec[ROLL];
    int16_t y = vec[PITCH];
    int16_t z = vec[YAW];

    vec[ROLL] = lrintf(boardRotation[0][0] * x + boardRotation[1][0] * y + boardRotation[2][0] * z);
    vec[PITCH] = lrintf(boardRotation[0][1] * x + boardRotation[1][1] * y + boardRotation[2][1] * z);
    vec[YAW] = lrintf(boardRotation[0][2] * x + boardRotation[1][2] * y + boardRotation[2][2] * z);
}

void alignSensors(int16_t *src, int16_t *dest, uint8_t rotation)
{
    switch (rotation) {
        case CW0_DEG:
            dest[ROLL] = src[ROLL];
            dest[PITCH] = src[PITCH];
            dest[YAW] = src[YAW];
            break;
        case CW90_DEG:
            dest[ROLL] = src[PITCH];
            dest[PITCH] = -src[ROLL];
            dest[YAW] = src[YAW];
            break;
        case CW180_DEG:
            dest[ROLL] = -src[ROLL];
            dest[PITCH] = -src[PITCH];
            dest[YAW] = src[YAW];
            break;
        case CW270_DEG:
            dest[ROLL] = -src[PITCH];
            dest[PITCH] = src[ROLL];
            dest[YAW] = src[YAW];
            break;
        case CW0_DEG_FLIP:
            dest[ROLL] = -src[ROLL];
            dest[PITCH] = src[PITCH];
            dest[YAW] = -src[YAW];
            break;
        case CW90_DEG_FLIP:
            dest[ROLL] = src[PITCH];
            dest[PITCH] = src[ROLL];
            dest[YAW] = -src[YAW];
            break;
        case CW180_DEG_FLIP:
            dest[ROLL] = src[ROLL];
            dest[PITCH] = -src[PITCH];
            dest[YAW] = -src[YAW];
            break;
        case CW270_DEG_FLIP:
            dest[ROLL] = -src[PITCH];
            dest[PITCH] = -src[ROLL];
            dest[YAW] = -src[YAW];
            break;
        default:
            break;
    }

    if (!standardBoardAlignment)
        alignBoard(dest);

}
