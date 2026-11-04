#pragma once

// Lab_CombatP0 greybox numbers. Source of truth for the prose table:
// Docs/Lab_CombatP0_Layout.md (已拍板). Procedural spawn only — no umap.
//
// Unit system matches the old flat sandbox and CombatFeel (do not add a second scale):
//   1 design px = 1 uu
//   design +X right -> Unreal +X
//   design +Y up    -> Unreal +Z  (origin: nominal PlayerStart feet on G_A)
//   Unreal Y is depth. The camera looks down -Y; slabs are centered at Y = 0.
// Engine BasicShapes/Cube is CubeSize uu on a side. Actor scale = size / CubeSize.
//
// Slab depth is thicker than the old 16 uu floor. A 16 uu slab (±8) does not reach
// the projectile plane (VisualPlaneY 12 + pea radius 3), so covers would not stop
// a shot. Half-depth 20 covers the pawn capsule (±12) and that shot plane.
//
// Playtest hotfix 2026-11-05. Live jump is ~32 uu (JumpZ 400, GravityScale 2.55).
// Steps are 28, island top is 24, E high platform is 56. This file does not
// retune JumpZ or gravity.

namespace PixelSideLabLayout
{
constexpr int CubeSize = 100;
constexpr int SlabDepth = 40;
constexpr int SlabCenterY = 0;
constexpr int SpawnClearance = 2;

constexpr int PlayWidth = 3200;
constexpr int GroundThickness = 24;
constexpr int PlatformThickness = 16;

// --- §3.1 floors and pit. BRIDGE is data only; it is not a resident actor. ---
// Island X stays 1400–1480 (center 1440, width 80). The pit is recentered on
// that island with a 60 uu gap on each side. G_A runs up to the near lip so
// the jump is the gap, not the gap plus an extra hole.
constexpr int G_A_X0 = 0;
constexpr int G_A_TopY = 0;

constexpr int IslandX0 = 1400;
constexpr int IslandX1 = 1480;
constexpr int IslandTopY = 24;
constexpr int PitSideGap = 60;

constexpr int PitX0 = IslandX0 - PitSideGap;
constexpr int PitX1 = IslandX1 + PitSideGap;
constexpr int PitDeathY = -80;
constexpr int G_A_X1 = PitX0;

// The capsule can lose the floor while its center is still ~1 radius back from
// the lip, so the catch starts a little before the pit and ends a little after
// the far lip. Feet Z (not the pit X alone) is what triggers recovery.
constexpr int PitCatchMargin = 64;
constexpr int PitCatchX0 = G_A_X1 - PitCatchMargin;
constexpr int PitCatchX1 = PitX1 + PitCatchMargin;

constexpr int G_C_Right_X0 = PitX1;
constexpr int G_C_Right_X1 = PlayWidth;
constexpr int G_C_Right_TopY = 0;

constexpr int BridgeX0 = PitX0;
constexpr int BridgeX1 = PitX1;
constexpr int BridgeTopY = 56;
constexpr bool bSpawnBridgeByDefault = false;

// --- §3.2 platforms (thickness PlatformThickness, top at TopY) ---
constexpr int P1CenterX = 720;
constexpr int P1Width = 160;
constexpr int P1TopY = 28;

constexpr int P2CenterX = 920;
constexpr int P2Width = 140;
constexpr int P2TopY = 56;

constexpr int P3CenterX = 1120;
constexpr int P3Width = 120;
constexpr int P3TopY = 84;

constexpr int P_E_CenterX = 2880;
constexpr int P_E_Width = 180;
constexpr int P_E_TopY = 56;

// --- §3.3 covers. Bottom center on the floor. Heights stay above the live jump.
// SpawnLabSandbox lets Pawns through these three only. Shots and visibility still hit.
// Peek is the far side along X (stand past the slab), not the top and not around Y. ---
constexpr int CV1_X = 2080;
constexpr int CV1_BottomY = 0;
constexpr int CV1_Height = 44;
constexpr int CV1_Width = 24;

constexpr int CV2_X = 2280;
constexpr int CV2_BottomY = 0;
constexpr int CV2_Height = 48;
constexpr int CV2_Width = 24;

constexpr int CV3_X = 2480;
constexpr int CV3_BottomY = 0;
constexpr int CV3_Height = 40;
constexpr int CV3_Width = 24;

// --- §4 feet positions. Facing is -X (left) for every mark. ---
constexpr int D1_FeetX = 600;
constexpr int D1_FeetY = 0;
constexpr int D2_FeetX = 920;
constexpr int D2_FeetY = 56;
constexpr int D3_FeetX = 2700;
constexpr int D3_FeetY = 0;
constexpr int D4_FeetX = 2880;
constexpr int D4_FeetY = 56;
constexpr int D5_FeetX = 3050;
constexpr int D5_FeetY = 0;

constexpr int T1_FeetX = 2360;
constexpr int T1_FeetY = 0;
// APixelDebugHurtTurret body scale Z is 0.36 on the 100 uu cube (36 tall, centered).
constexpr int TurretBodyHalfHeight = 18;

// Soft recovery lands on G_A, just left of the pit lip. 0 damage. Lab only.
// 80 uu back from the lip keeps the 12 uu capsule out of the catch window.
constexpr int SafeFeetX = 1260;
constexpr int SafeFeetY = 0;

// Design origin is the left edge of G_A (doc: PlayerStart feet). The pawn capsule
// is 12 uu in radius and the left wall's inner face is X = 0, so the actor is
// inset this far onto G_A instead of spawning inside the wall.
constexpr int PlayerSpawnFeetX = 32;
constexpr int PlayerSpawnFeetY = 0;

// Inner face meets the playable edge. Authored height is above the walkable plane;
// spawn sinks the extra GroundThickness through the floor slab.
constexpr int WallLeftX = -40;
constexpr int WallRightX = 3240;
constexpr int WallHeight = 280;
constexpr int WallThickness = 80;

static_assert(CubeSize == 100, "BasicShapes cube extent");
static_assert(SlabDepth / 2 >= 15, "slab half-depth reaches VisualPlaneY + pea radius");
static_assert(PlayWidth == 3200, "five screens of 640");
static_assert(G_A_X1 == PitX0, "G_A meets the near pit lip");
static_assert(G_A_X1 > 1280, "floor includes the zone C approach");
static_assert(PitSideGap == 60, "each pit side gap is 60");
static_assert(PitSideGap >= 56 && PitSideGap <= 64, "gap stays in the playtest band");
static_assert(IslandX0 - PitX0 == PitSideGap && PitX1 - IslandX1 == PitSideGap, "equal side gaps");
static_assert((IslandX0 + IslandX1) == (PitX0 + PitX1), "island shares the pit center");
static_assert(PitX1 - PitX0 == 80 + 2 * PitSideGap, "pit is the island plus two gaps");
static_assert(PitCatchX0 < G_A_X1 && PitCatchX1 > PitX1, "catch covers the lip and the edge overhang");
static_assert(PitCatchX0 > G_A_X0 && PitCatchX1 < G_C_Right_X1, "catch stays inside the lab");
static_assert(IslandX1 - IslandX0 == 80, "island width");
static_assert(IslandX0 > PitX0 && IslandX1 < PitX1, "island sits in the pit");
static_assert(IslandTopY == 24, "island top");
static_assert(G_C_Right_X0 == PitX1 && G_C_Right_X1 == PlayWidth, "floor resumes at the far pit lip");
static_assert(!bSpawnBridgeByDefault, "BRIDGE is not resident");
static_assert(BridgeX0 == PitX0 && BridgeX1 == PitX1, "debug bridge spans the pit");
static_assert(P1TopY == 28 && P2TopY == 56 && P3TopY == 84, "platform tops");
static_assert(P_E_TopY == 56, "E high platform");
static_assert(P2TopY - P1TopY == 28 && P3TopY - P2TopY == 28, "uniform 28px rise");
static_assert(P1TopY < 32 && IslandTopY < 32, "first rises sit under the live ~32 jump");
static_assert(D2_FeetX == P2CenterX && D2_FeetY == P2TopY, "D2 stands on P2");
static_assert(D4_FeetX == P_E_CenterX && D4_FeetY == P_E_TopY, "D4 stands on P_E_high");
static_assert(CV1_Height == 44 && CV2_Height == 48 && CV3_Height == 40, "cover heights are not the route fix");
static_assert(CV1_Height > 32 && CV2_Height > 32 && CV3_Height > 32, "covers stay above the live ~32 jump");
static_assert(CV2_X + CV2_Width / 2 < T1_FeetX, "CV2 is between the lane and T1");
static_assert(T1_FeetX >= 2000, "T1 is outside the opening screen");
static_assert(SafeFeetX == PitX0 - 80, "safe point is 80 uu left of the pit");
static_assert(SafeFeetX > G_A_X0 && SafeFeetX + 12 < PitCatchX0, "safe point is on G_A, outside the catch");
static_assert(PitDeathY == -80, "pit death line");
static_assert(WallLeftX + WallThickness / 2 == G_A_X0, "left wall seals X=0");
static_assert(WallRightX - WallThickness / 2 == PlayWidth, "right wall seals X=3200");
static_assert(PlayerSpawnFeetX > 12 && PlayerSpawnFeetX < 80, "spawn inset onto G_A");
static_assert(PlayerSpawnFeetY == G_A_TopY && SafeFeetY == G_A_TopY, "feet rest on G_A; design Y is Unreal Z");
}
