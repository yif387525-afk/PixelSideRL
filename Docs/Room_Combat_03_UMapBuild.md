# Room_Combat_03 — 正式 umap 建图清单

**引擎：** UE 5.8.4  
**状态：** 正式 umap 已入库 `Content/Maps/Rooms/Room_Combat_03.umap`（Mac Editor Python 烘焙；Git LFS；magic `c1832a9e`；灰盒关；`RoomId=Room_Combat_03`；`FirstWaveDelay=0.75`）。本文件是建图清单，不附带二进制。导演 P0.7 仍只认 Combat_01，本图 Play 不跑波次。  
**规格：** `Content/Maps/Rooms/Room_Combat_03.md`  
**机读表：** `Content/Data/Rooms/Room_Combat_03_{meta,spawns,geometry}.csv`  
**对照：** 章节与 [`Room_Combat_02_UMapBuild.md`](Room_Combat_02_UMapBuild.md) / [`Room_Combat_01_UMapBuild.md`](Room_Combat_01_UMapBuild.md) 相同。本房是双高台主次不对称（`P_sec`/`P_step` TopY=28，`P_main` TopY=**56**）。宽 **1920**。中路可冲，禁止左右镜像死克隆。

不要改 JumpZ、重力、Lab 级差。Peek 仍是穿过掩体后沿 X 站到对侧。级差单跳窗约 28：主台经踏板上去，不要做成 ≥32 的必过单级。

**运行时边界（勿臆造）：** 当前导演 P0.7 逻辑只认 `RoomId=Room_Combat_01`。本图的 `RoomId` 仍必须是 `Room_Combat_03`。**现在 Play 不会跑本图波次。** 手拼验收查几何和出口。波次以后再接 CSV，本 PR 不改 C++。

---

## 0. 轴与单位

设计文档的 `+Y` 向上。UE 侧视里：

| 设计 | UE |
|------|----|
| X | X |
| 向上的 Y | **Z** |
| 深度 | Y = 0 为板中心；板深 **40** |

1 设计 px = 1 uu。房间原点 = 左侧入口内侧地面。PlayerStart 在 **(X=48, Z=0)**。

行走面高度用设计 TopY，不要把板厚加进 TopY。地面行走面 Z=0；次台/踏板 Z=**28**；主台 Z=**56**。板厚只往下长：地面厚 **24**，平台厚 **16**（主台也是 16，中心 Z = 56 − 8 = 48）。

---

## 1. 建关

1. 内容浏览器：`Content/Maps/Rooms/`。
2. 新建空关卡，保存为 **`Room_Combat_03`**。路径 `Content/Maps/Rooms/Room_Combat_03`。
3. Git LFS。不要提交空文件或假 uasset。
4. 世界设置仍用 `APixelSideGameMode`。不要改 C++。

---

## 2. 按 geometry.csv 摆几何

Actor Label 用 CSV 的 `id`。UE 位置 `(X, 0, Z)`，Z = 设计 Y。

### 2.1 地板与墙（无坑）

| ID | 种类 | 设计范围 | 行走面 / 墙顶 Z | 摆法 |
|----|------|----------|-----------------|------|
| `G_main` | Floor | X 0–1920 | 顶 Z=**0** | 连续地板。宽 1920。中路可冲，不要挖坑。 |
| `WALL_L` | Wall | X -40–0 | 墙顶 Z=**280** | 左边界。宽 40。 |
| `WALL_R` | Wall | X 1920–1960 | 墙顶 Z=**280** | 右边界。宽 40。 |

入口安全区约 X 0–360。

### 2.2 双高台（主次不对称）

| ID | 设计 X | TopY | 角色 |
|----|--------|------|------|
| `P_sec` | **360–580** | **28** | 次台，靠前短台。宽 220。 |
| `P_step` | **820–940** | **28** | 踏板，接主台。宽 120。 |
| `P_main` | **980–1380** | **56** | 主高台，靠后长台。宽 400。厚度仍 16，只往下长。 |

不要做成左右对称克隆。主台高于次台；从地面经踏板再上主台。

### 2.3 掩体（挡弹、人可穿）

| ID | 底心 (X, Z) | 宽 | 高 |
|----|-------------|----|----|
| `CV_A` | (480, 0) | 24 | 44 |
| `CV_B` | (1100, 0) | 24 | 40 |
| `CV_C` | (1500, 0) | 24 | 44 |

碰撞：WorldStatic；**Pawn Ignore**；Visibility / WorldDynamic **Block**；Camera Ignore。Peek 沿 X。

### 2.4 出口 `APixelRoomExit`

| ID | 设计 (X, Y) | `Exit Id` | `bRequireClear` |
|----|-------------|-----------|-----------------|
| `Exit_Left` | (-40, 0) | `Exit.Left` | **不勾** |
| `Exit_Right` | (**1840**, 0) | `Exit.Right` | **勾上** |

`Volume Relative Location` 留 0。左门触发盒会自动往房内偏 +50，不要盖住 (48, 0)。

### 2.5 锚点（不手摆怪）

| ID | 设计 (X, Y) | 放什么 |
|----|-------------|--------|
| `PlayerStart` | **(48, 0)** | 引擎 `PlayerStart`。脚底 Z=0；胶囊中心半高 19 + 2。 |
| `AffixAnchor` | (960, 0) | 三选一参考。不必放 Actor。 |

刷怪点不要手摆。CSV 约定以后由导演读：

| ID | 波 | (X, Y) | 占位 |
|----|----|--------|------|
| SP_G1 | 1 | (520, 0) | `Enemy.Grunt.Early` |
| SP_G2 | 1 | (720, 0) | `Enemy.Grunt.Early` |
| SP_G3 | 2 | (1280, 0) | `Enemy.Grunt.Early`，主台下 |
| SP_A1 | 2 | (460, 72) | `Enemy.Flyer.Early`，次台上方 |
| SP_A2 | 2 | (1180, 100) | `Enemy.Flyer.Early`，主台上方 |
| SP_G4 | 3 | (1560, 0) | `Enemy.Grunt.Early`，可选 W3 |

同时存活 ≤6。不要造假 Flyer uasset。

---

## 3. 导演

1. 原点放一只 **`APixelRoomDirector`**。
2. **`RoomId` = `Room_Combat_03`**。不要写成 `Combat_03`，也不要改成 `Room_Combat_01` 借波次。
3. **`bBuildGreyboxFromCsv` = 不勾。**
4. `bAutoStart`、`bPlacePlayerAtAnchor` 勾上。
5. `FirstWaveDelay` = **0.75**。
6. 不要放 `APixelShopRoomHost`。不要手摆怪。

当前导演发现 `RoomId` 不是 `Room_Combat_01` 会停住。这是代码范围。本 PR 不改 C++。

---

## 4. 在编辑器里试

1. 不想看到 Lab 五区时，Play 前开 `PixelSide.DriveRoomCombat01 1`。已有导演则不会再生成一只。该开关不会刷本图波次。
2. 不要开 `DriveRoomShop01`。
3. 不要改 F11、Lab、P0.6。

手拼验收：

- [ ] 宽 1920，无坑。`P_sec`/`P_step` TopY 28，`P_main` TopY 56；布局不是左右镜像。
- [ ] 三块掩体挡弹、人能穿过。
- [ ] 右门 @1840 勾 `bRequireClear`，左门不勾。VolumeRel=0。
- [ ] 导演 `RoomId=Room_Combat_03`，灰盒关，`FirstWaveDelay` 0.75。无第二套程序化几何。
- [ ] 无手摆怪 / 假 uasset / ShopHost。
- [ ] 「只驱动 Room_Combat_01」日志可出现；不要改 `RoomId` 去消掉它。

波次、三选一、右门开等运行验收留到导演接本 CSV 后再勾。

---

## 5. 提交真图时

只提交 `Content/Maps/Rooms/Room_Combat_03.umap` 及真正引用配套。Git LFS。不要交空文件、假 uasset 或新 C++。

说明里写：导演已关灰盒，`RoomId` 为 `Room_Combat_03`。更新 `Docs/RoomPipeline.md`。

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 建图清单。真 umap 仍待 Mac / UE 5.8.4。仓库不放假二进制。导演 P0.7 尚未认本 RoomId。 |
| 2026-11-05 | Mac Editor Python 已烘焙真图（LFS；灰盒关；RoomId=Room_Combat_03；双高台 P_sec/P_step@28、P_main@56；待 COMMENT 开 PR）。 |
