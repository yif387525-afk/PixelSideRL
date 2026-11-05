# Room_Elite_01 — 正式 umap 建图清单

**引擎：** UE 5.8.4  
**状态：** Mac Editor Python 已烘焙真图 `Content/Maps/Rooms/Room_Elite_01.umap`（`APixelRoomDirector`；`RoomId=Room_Elite_01`；`bBuildGreyboxFromCsv` 关；Git LFS）。进仓 / COMMENT PR 待审合。导演尚未跑精英波次（预期）。  
**规格：** `Content/Maps/Rooms/Room_Elite_01.md`  
**机读表：** `Content/Data/Rooms/Room_Elite_01_{meta,spawns,geometry}.csv`  
**对照：** 章节与 [`Room_Combat_01_UMapBuild.md`](Room_Combat_01_UMapBuild.md) 相同。本房是中央地坪竞技场，和 [`Room_Elite_02_UMapBuild.md`](Room_Elite_02_UMapBuild.md) 进池轮换，不要摆成同一张图。

不要改 JumpZ、重力、Lab 级差。Peek 仍是穿过掩体后沿 X 站到对侧。级差只有 **28**。无坑。

**运行时边界（勿臆造宿主）：** 仓库里没有单独的精英导演类。能放进关卡的是现有 `APixelRoomDirector`。它的 P0.7 逻辑只认 `RoomId=Room_Combat_01`。本图仍放这只演员，但 **现在 Play 不会刷精英、也不会弹三选一**。手拼验收查几何和出口勾选。波次要等以后的代码接上同一份 CSV，本 PR 不改 C++。

---

## 0. 轴与单位

设计文档的 `+Y` 向上。UE 侧视里：

| 设计 | UE |
|------|----|
| X | X |
| 向上的 Y | **Z** |
| 深度 | Y = 0 为板中心；板深 **40**（与现灰盒 `SlabDepth` 一致） |

1 设计 px = 1 uu。房间原点 = 左侧入口内侧地面，不是 PlayerStart。PlayerStart 在 **(X=48, Z=0)**。

行走面用设计 TopY。地面顶 Z=0，两侧台顶 Z=28。板厚只往下长：地面厚 **24**，平台厚 **16**。不改跳跃。

---

## 1. 建关

1. 内容浏览器：`Content/Maps/Rooms/`。
2. 新建空关卡，保存为 **`Room_Elite_01`**。路径 `Content/Maps/Rooms/Room_Elite_01`，文件 `Room_Elite_01.umap`。
3. Git LFS。不要提交空文件、占位文件或手写假 uasset。
4. 世界设置仍用现有 `APixelSideGameMode`。不要改 GameMode C++。

---

## 2. 按 geometry.csv 摆几何

Actor Label 用 CSV 的 `id`。UE 位置 `(X, 0, Z)`，Z = 设计 Y。

### 2.1 地板与墙（无坑）

| ID | 种类 | 设计范围 | 行走面 / 墙顶 Z | 摆法 |
|----|------|----------|-----------------|------|
| `G_main` | Floor | X 0–1920 | 顶 Z=**0** | 连续地板。宽 1920。中央开阔，不要挖坑。 |
| `WALL_L` | Wall | X -40–0 | 墙顶 Z=**280** | 左边界。宽 40。 |
| `WALL_R` | Wall | X 1920–1960 | 墙顶 Z=**280** | 右边界。宽 40。 |

地板和墙挡 Pawn 和弹体。入口安全区约 X 0–280，里面没有精英点。

### 2.2 两侧短台

| ID | 设计 X | TopY | 角色 |
|----|--------|------|------|
| `P_L` | **360–560** | **28** | 左侧风筝 / 仰角 |
| `P_R` | **1360–1600** | **28** | 右侧风筝 / 仰角 |

没有第二层。中路约 X 600–1300 地面可冲。不要加 TopY≥32 的必过台。

### 2.3 掩体（挡弹、人可穿）

| ID | 底心 (X, Z) | 宽 | 高 |
|----|-------------|----|----|
| `CV_A` | (280, 0) | 24 | 44 |
| `CV_B` | (880, 0) | 24 | 40 |
| `CV_C` | (1480, 0) | 24 | 44 |

碰撞与灰盒 `PassThroughPawn` 一致：WorldStatic；**Pawn Ignore**；**Visibility Block**；**WorldDynamic Block**；Camera Ignore。Peek 沿 X，不要跳上顶。

### 2.4 出口 `APixelRoomExit`

| ID | 设计 (X, Y) | `Exit Id` | `bRequireClear` |
|----|-------------|-----------|-----------------|
| `Exit_Left` | (-40, 0) | `Exit.Left` | **不勾** |
| `Exit_Right` | (1840, 0) | `Exit.Right` | **勾上** |

规格是清空精英和杂兵后三选一，再开右门。左门不重置已清。`Volume Relative Location` 留 0。左门触发盒会自己往房内偏 +50，不要盖住 (48, 0)。

### 2.5 锚点与刷怪（不手摆怪）

| ID | 设计 (X, Y) | 放什么 |
|----|-------------|--------|
| `PlayerStart` | **(48, 0)** | 引擎 `PlayerStart`。脚底 Z=0，胶囊中心再抬半高 + 2 uu。 |
| `AffixAnchor` | (960, 0) | 三选一参考。不必放 Actor。 |

刷怪点不要放进关卡。以后导演若接上这张 CSV，读的是：

| ID | 波 | (X, Y) | 占位 |
|----|----|--------|------|
| SP_G1 | 1 | (480, 0) | `Enemy.Grunt.Early` |
| SP_G2 | 1 | (720, 0) | `Enemy.Grunt.Early` |
| SP_G3 | 1 | (1200, 0) | `Enemy.Grunt.Early` |
| SP_G4 | 1 | (1520, 0) | `Enemy.Grunt.Early` |
| SP_E1 | 2 | **(1100, 0)** | `Enemy.Elite.Mid`，地面中后场，唯一 |

同时存活 ≤8。不要放第二只精英，不要把精英放在右门 (1840, 0) 上。现在没有精英 Actor 类可摆，也不要造假 uasset。

---

## 3. 导演

1. 原点 `(0, 0, 0)` 放一只 **`APixelRoomDirector`**。没有别的精英宿主类，不要新造一个。
2. **`RoomId` = `Room_Elite_01`**。不要写成 `Elite_01`，也不要改成 `Room_Combat_01` 来借战斗波次。
3. **`bBuildGreyboxFromCsv` = 不勾。**
4. `bAutoStart`、`bPlacePlayerAtAnchor` 保持勾上。
5. 不要放 `APixelShopRoomHost`。

当前 `StartRoom` 发现 `RoomId` 不是 `Room_Combat_01` 会打日志并停住。这是预期，不是摆错图。

---

## 4. 在编辑器里试

1. 若不想同时看到 Lab 五区，Play 前开 `PixelSide.DriveRoomCombat01 1`。关卡里已有导演时不会再生成第二只。这个开关**不会**让本图开始刷精英。
2. 不要开 `DriveRoomShop01`。两者同时开时商店让路，但本图也不是商店。
3. 不要改 F11、Lab、P0.6。

手拼验收：

- [ ] 宽 1920，地面连续无坑。两侧台 TopY 28，中路可冲。
- [ ] 三块掩体挡弹、人能穿过。
- [ ] 右门 `Exit.Right` 勾了 `bRequireClear`，左门没勾。
- [ ] 导演在原点，`RoomId` 是 `Room_Elite_01`，`bBuildGreyboxFromCsv` 关着。没有第二套程序化灰盒。
- [ ] 关卡里没有手摆的怪，也没有假 uasset。
- [ ] 日志里出现「只驱动 Room_Combat_01」是当前代码的范围，不要为了消掉它去改 `RoomId`。

规格上的运行验收（两波、唯一地面精英、三选一后右门开、左门不重置）留到导演真正读这张 CSV 之后再勾。本清单不把它写成现在就能玩过。

---

## 5. 提交真图时

只提交编辑器保存的 `Content/Maps/Rooms/Room_Elite_01.umap` 和它真正引用的配套。Git LFS。不要交空文件、假 uasset、Lab 改动或新的 C++ 宿主。

说明里写：导演已关灰盒，`RoomId` 为 `Room_Elite_01`。登记表把「Mac/UE 待拼」改成 umap 已入库。

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 建图清单。真 umap 仍待 Mac / UE 5.8.4。仓库不放假二进制。导演尚未跑精英波次。 |
| 2026-11-05 | Mac Editor Python 已烘焙真图（LFS；灰盒关；RoomId=Room_Elite_01；待 COMMENT 开 PR）。 |
