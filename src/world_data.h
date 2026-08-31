#pragma once

/*
 * PROJECT NOCTIS - world_data.h
 * District: "Campanario" (Bell tower district), starting zone.
 *
 * Self-contained geometry data for the opening gothic megastructure.
 * No includes, no functions: pure POD data only.
 *
 * Coordinate system:
 *   (x, y, z) is the base-center of each box on the ground plane.
 *   y is the base height (0.0f = sitting on the ground).
 *   (w, d, h) are width (x), depth (z) and height (y) of the box.
 *
 * Color packing: 0xAABBGGRR (little-endian RGBA).
 *   byte0 = R, byte1 = G, byte2 = B, byte3 = A.
 *   value = (A << 24) | (B << 16) | (G << 8) | R.  Alpha is always 0xFF.
 *
 * Palette (cold dark stone; tall spires slightly lighter/cooler):
 *   STONE_BLACK  RGB( 30, 32, 40) -> 0xFF28201E
 *   STONE_DARK   RGB( 38, 41, 52) -> 0xFF342926
 *   STONE_BLUE   RGB( 45, 50, 64) -> 0xFF40322D
 *   STONE_MID    RGB( 52, 56, 70) -> 0xFF463834
 *   RUIN_LOW     RGB( 34, 37, 45) -> 0xFF2D2522
 *   SPIRE_COOL   RGB( 68, 74, 92) -> 0xFF5C4A44
 *   SPIRE_LIGHT  RGB( 78, 85,105) -> 0xFF69554E
 *
 * A clear spawn area of radius ~10 around the origin is kept free of
 * structure footprints (ground-level collectibles may still sit there).
 */

struct Structure { float x, y, z; float w, d, h; unsigned int color; };

static const Structure kStructures[] = {
    /* -- MONUMENTAL SPIRES / BELL TOWERS (tall, thin) -- */
    {  28.0f, 0.0f,  26.0f,   9.0f,  9.0f, 132.0f, 0xFF69554E }, /* NE great spire   */
    { -30.0f, 0.0f, -22.0f,  10.0f, 10.0f, 118.0f, 0xFF5C4A44 }, /* SW spire         */
    {  -4.0f, 0.0f,  34.0f,   8.0f,  8.0f, 140.0f, 0xFF69554E }, /* the Campanario   */
    {  24.0f, 0.0f, -30.0f,   7.0f,  7.0f,  96.0f, 0xFF5C4A44 }, /* SE watch spire   */

    /* -- MID BUILDINGS (h 25..55) -- */
    {  16.0f, 0.0f,  14.0f,  12.0f, 10.0f,  42.0f, 0xFF463834 },
    { -18.0f, 0.0f,  12.0f,  11.0f, 13.0f,  38.0f, 0xFF40322D },
    { -14.0f, 0.0f, -16.0f,  10.0f, 10.0f,  52.0f, 0xFF342926 },
    {  18.0f, 0.0f, -12.0f,  13.0f, 11.0f,  34.0f, 0xFF463834 },
    {  34.0f, 0.0f,  -4.0f,   9.0f, 14.0f,  48.0f, 0xFF40322D },
    { -34.0f, 0.0f,   6.0f,  12.0f,  9.0f,  44.0f, 0xFF342926 },
    {   6.0f, 0.0f, -26.0f,  14.0f, 10.0f,  30.0f, 0xFF463834 },
    {  -8.0f, 0.0f,  22.0f,  10.0f, 10.0f,  36.0f, 0xFF40322D },
    {  40.0f, 0.0f,  18.0f,   8.0f,  8.0f,  28.0f, 0xFF342926 },
    { -40.0f, 0.0f,  -8.0f,   9.0f, 11.0f,  40.0f, 0xFF463834 },

    /* -- LOW PLATFORMS / RUINS (h 8..20, climbable) -- */
    {  14.0f, 0.0f,  11.0f,   8.0f,  8.0f,  14.0f, 0xFF2D2522 },
    { -12.0f, 0.0f,  13.0f,   9.0f,  7.0f,  10.0f, 0xFF28201E },
    {   8.0f, 0.0f, -15.0f,  10.0f,  8.0f,  16.0f, 0xFF2D2522 },
    { -15.0f, 0.0f,  -6.0f,   8.0f,  9.0f,  18.0f, 0xFF28201E },
    {  22.0f, 0.0f,   6.0f,   7.0f,  7.0f,   9.0f, 0xFF2D2522 },
    { -22.0f, 0.0f, -14.0f,   9.0f,  8.0f,  20.0f, 0xFF28201E },

    /* -- OUTER FILL -- */
    {  30.0f, 0.0f, -16.0f,  10.0f,  9.0f,  26.0f, 0xFF342926 },
    { -26.0f, 0.0f,  20.0f,   9.0f, 10.0f,  32.0f, 0xFF40322D },
};

static const int kStructureCount = (int)(sizeof(kStructures) / sizeof(kStructures[0]));

/*
 * Collectible spawn points. type 0..5 (resource category).
 * Rooftop spawns sit at y = the host structure's h; ground spawns at y = 0.
 */
struct ResourceSpawn { float x, y, z; int type; };

static const ResourceSpawn kResources[] = {
    /* -- ON ROOFTOPS (y = host structure height) -- */
    {  -4.0f, 140.0f,  34.0f, 0 }, /* atop the Campanario   */
    {  28.0f, 132.0f,  26.0f, 1 }, /* atop NE great spire   */
    { -14.0f,  52.0f, -16.0f, 2 }, /* atop tall mid tower   */
    {  16.0f,  42.0f,  14.0f, 3 }, /* atop mid building     */
    {   8.0f,  16.0f, -15.0f, 4 }, /* atop low ruin         */
    { -15.0f,  18.0f,  -6.0f, 5 }, /* atop low ruin         */

    /* -- ON THE GROUND (scattered) -- */
    {   6.0f,   0.0f,   8.0f, 0 },
    { -20.0f,   0.0f,   4.0f, 2 },
    {  12.0f,   0.0f,  -8.0f, 3 },
    {  -6.0f,   0.0f, -12.0f, 1 },
};

static const int kResourceCount = (int)(sizeof(kResources) / sizeof(kResources[0]));
