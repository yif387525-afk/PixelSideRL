# Room_Elite_02 — 正式 umap 建图清单

**引擎：** UE 5.8.4  
**状态：** 正式 umap 已入库 `Content/Maps/Rooms/Room_Elite_02.umap`（Mac Editor Python 烘焙；Git LFS；magic `c1832a9e`；灰盒关；`RoomId=Room_Elite_02`）。本文件是建图清单，不附带二进制。  
**规格：** `Content/Maps/Rooms/Room_Elite_02.md`  
**机读表：** `Content/Data/Rooms/Room_Elite_02_{meta,spawns,geometry}.csv`  
**对照：** 章节与 [`Room_Combat_01_UMapBuild.md`](Room_Combat_01_UMapBuild.md) 相同。本房是高台压制，和 [`Room_Elite_01_UMapBuild.md`](Room_Elite_01_UMapBuild.md) 进池轮换。同一次 Run 仍只抽一间精英房。不要摆成 Elite_01 的中央地坪。

不要改 JumpZ、重力、Lab 级差。Peek 沿 X。级差只有 **28**。无坑。

**运行时边界（勿臆造宿主）：** 没有单独的精英导演类。关卡里放现有 `APixelRoomDirector`。它的 P0.7 逻辑只认 `RoomId=Room_Combat_01`。本图的 `RoomId` 仍必须是 `Room_Elite_02`。**现在 Play 不会把精英刷到高台上，也不会弹三选一。** 手拼验收查几何和出口。波次以后再接 CSV，本 PR 不改 C++。

---

## 0. 轴与单位

设计文档的 `+Y` 向上。UE 侧视里：

| 设计 | UE |
|------|----|
| X | X |
| 向上的 Y | **Z** |
| 深度 | Y = 0 为板中心；板深 **40** |

1 设计 px = 1 uu。原点 = 左侧入口内侧地面。PlayerStart 在 **(X=48, Z=0)**。

地面顶 Z=0。三块台的行走面都是 Z=**28**。地面厚 **24**，平台厚 **16**，只往下长。高台可单跳上去，不是 ≥32 的必过台。

---

## 1. 建关

1. 内容浏览器：`Content/Maps/Rooms/`。
2. 新建空关卡，保存为 **`Room_Elite_02`**。路径 `Content/Maps/Rooms/Room_Elite_02`。
3. Git LFS。不要提交空文件或假 uasset。
4. 仍用现有 `APixelSideGameMode`。不要改 C++。

---

## 2. 按 geometry.csv 摆几何

UE 位置 `(X, 0, Z)`，Z = 设计 Y。Label 用 CSV 的 `id`。

### 2.1 地板与墙（无坑）

| ID | 种类 | 设计范围 | 行走面 / 墙顶 Z | 摆法 |
|----|------|----------|-----------------|------|
| `G_main` | Floor | X 0–1920 | 顶 Z=**0** | 连续地板。宽 1920。台下留穿行缝，不要挖坑。 |
| `WALL_L` | Wall | X -40–0 | 墙顶 Z=**280** | 左边界。宽 40。 |
| `WALL_R` | Wall | X 1920–1960 | 墙顶 Z=**280** | 右边界。宽 40。 |

地板和墙挡 Pawn 和弹体。入口安全区约 X 0–280。

### 2.2 高台与垫脚

| ID | 设计 X | TopY | 角色 |
|----|--------|------|------|
| `P_elite` | **880–1280** | **28** | 精英高台。宽 400。唯一精英站位的台面。 |
| `P_approach_L` | **640–800** | **28** | 左接近垫脚。 |
| `P_approach_R` | **1360–1520** | **28** | 右接近垫脚。 |

台下穿行缝：X **800–880** 与 **1280–1360**（各约 80）。地面可以从台下绕过，不要用墙封死。没有第二层。

### 2.3 掩体（挡弹、人可穿）

| ID | 底心 (X, Z) | 宽 | 高 |
|----|-------------|----|----|
| `CV_A` | (320, 0) | 24 | 44 |
| `CV_B` | (720, 0) | 24 | 40 |
| `CV_C` | (1440, 0) | 24 | 40 |
| `CV_D` | (1680, 0) | 24 | 44 |

碰撞：WorldStatic；Pawn Ignore；Visibility 与 WorldDynamic Block；Camera Ignore。Peek 沿 X。

### 2.4 出口 `APixelRoomExit`

| ID | 设计 (X, Y) | `Exit Id` | `bRequireClear` |
|----|-------------|-----------|-----------------|
| `Exit_Left` | (-40, 0) | `Exit.Left` | **不勾** |
| `Exit_Right` | (1840, 0) | `Exit.Right` | **勾上** |

规格是清空后三选一，再开右门。左门不重置已清。`Volume Relative Location` 留 0。不要让左门触发盒盖住 (48, 0)。

### 2.5 锚点与刷怪（不手摆怪）

| ID | 设计 (X, Y) | 放什么 |
|----|-------------|--------|
| `PlayerStart` | **(48, 0)** | 引擎 `PlayerStart`。脚底 Z=0。 |
| `AffixAnchor` | (960, 0) | 三选一参考，在台下中场。不必放 Actor。 |

不要把精英 Actor 手摆到台上。CSV 约定以后由导演读：

| ID | 波 | (X, Y) | 占位 |
|----|----|--------|------|
| SP_G1 | 1 | (400, 0) | `Enemy.Grunt.Early`，左廊 |
| SP_G2 | 1 | (720, 0) | `Enemy.Grunt.Early` |
| SP_G3 | 1 | (1520, 0) | `Enemy.Grunt.Early`，右廊 |
| SP_E1 | 2 | **(1080, 28)** | `Enemy.Elite.Mid`，高台中心，唯一 |

Y=28 是台面，不是地面。不要改成 (1100, 0)，那是 Elite_01。同时 ≤8。精英不要压在右门上。现在没有精英类可摆，不要造假 uasset。

---

## 3. 导演

1. 原点放一只 **`APixelRoomDirector`**。不要新造精英宿主。
2. **`RoomId` = `Room_Elite_02`**。不要写成 `Elite_02` 或 `Room_Combat_01`。
3. **`bBuildGreyboxFromCsv` = 不勾。**
4. `bAutoStart`、`bPlacePlayerAtAnchor` 保持勾上。
5. 不要放 `APixelShopRoomHost`。

当前导演会因 `RoomId` 不是 `Room_Combat_01` 而停住。这是代码范围，不是摆台摆错。

---

## 4. 在编辑器里试

1. 不想看到 Lab 五区时，Play 前开 `PixelSide.DriveRoomCombat01 1`。已有导演则不会再生成一只。这个开关不会刷本图的高台精英。
2. 不要开 `DriveRoomShop01`。
3. 不要改 F11、Lab、P0.6。

手拼验收：

- [ ] 宽 1920，无坑。`P_elite` 与两块垫脚都是 TopY 28，台下两条约 80 的地面缝还在。
- [ ] 四块掩体挡弹、人能穿过。
- [ ] 右门勾 `bRequireClear`，左门不勾。精英站位记在 CSV 的 (1080, 28)，没有手摆第二只精英。
- [ ] 导演 `RoomId` 是 `Room_Elite_02`，灰盒开关关着。没有第二套程序化几何。
- [ ] 布局不是 Elite_01 的两侧短台加地面中场精英。
- [ ] 「只驱动 Room_Combat_01」的日志可以出现。不要改 `RoomId` 去消掉它。

两波、登台、三选一后右门开、左门不重置，等导演读这张 CSV 后再验收。现在不把它们写成已经能玩过。

---

## 5. 提交真图时

只提交 `Content/Maps/Rooms/Room_Elite_02.umap` 和它真正引用的配套。Git LFS。不要交空文件、假 uasset 或新 C++。

说明里写：导演已关灰盒，`RoomId` 为 `Room_Elite_02`。登记表把「Mac/UE 待拼」改成 umap 已入库。

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 建图清单。真 umap 仍待 Mac / UE 5.8.4。仓库不放假二进制。导演尚未跑精英波次。 |
| 2026-11-05 | Mac Editor Python 已烘焙真图（LFS；灰盒关；RoomId=Room_Elite_02；高台压制 P_elite/P_approach_*；待 COMMENT 开 PR）。 |
