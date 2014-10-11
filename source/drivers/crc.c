#include "board.h"
#include "crc.h"

/*
 \brief  Feed a uint32_t buffer to the STMs CRC engine. The buffer
 must have a size that is a multiple of 4 bytes.
 \param  start  start of buffer pointer.
 \param  end    past-end of buffer pointer.
 */
void crc32Feed(uint32_t* start, uint32_t* end)
{
    while (start < end)
        crc32Write(*start++);
    //assert(start==end);
}

/*
 \brief  CRC32B a uint32_t buffer using the STMs CRC engine. The buffer
 must have a size that is a multiple of 4 bytes. The CRC engine
 is reset at the start, and the result is NOTed as expected for
 ethernet or pk CRCs.
 \param start  start of buffer pointer.
 \param end    past-end of buffer pointer.
 */
uint32_t crc32B(uint32_t* start, uint32_t* end)
{
    crc32Reset();
    crc32Feed(start, end);
    return ~crc32Read();
}
