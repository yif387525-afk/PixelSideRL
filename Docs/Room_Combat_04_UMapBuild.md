# Room_Combat_04 — 正式 umap 建图清单

**引擎：** UE 5.8.4  
**状态：** 正式 umap 已入库 `Content/Maps/Rooms/Room_Combat_04.umap`（Mac Editor Python 烘焙；Git LFS；magic `c1832a9e`；灰盒关；`RoomId=Room_Combat_04`；`FirstWaveDelay=0.75`）。本文件是建图清单，不附带二进制。导演 P0.7 仍只认 Combat_01，本图 Play 不跑波次。  
**规格：** `Content/Maps/Rooms/Room_Combat_04.md`  
**机读表：** `Content/Data/Rooms/Room_Combat_04_{meta,spawns,geometry}.csv`  
**对照：** 章节与 [`Room_Combat_03_UMapBuild.md`](Room_Combat_03_UMapBuild.md) / [`Room_Combat_02_UMapBuild.md`](Room_Combat_02_UMapBuild.md) 相同。本房是 **坑 + 桥**：宽 **1760**；坑区 X **720–960**（宽 240，可读）**无地板**；桥 `BRIDGE` 同跨度 TopY=**28** 常驻可走；远侧另有 `P_far` TopY 28。

不要改 JumpZ、重力、Lab 级差。Peek 仍是穿过掩体后沿 X 站到对侧。

**运行时边界（勿臆造）：** 当前导演 P0.7 逻辑只认 `RoomId=Room_Combat_01`。本图的 `RoomId` 仍必须是 `Room_Combat_04`。**现在 Play 不会跑本图波次。** 手拼验收查几何和出口。波次以后再接 CSV，本 PR 不改 C++。不要在关卡里放 SoftRecover 碰撞块（规格 RecoverPoint 只是锚点约定）。

---

## 0. 轴与单位

设计文档的 `+Y` 向上。UE 侧视里：

| 设计 | UE |
|------|----|
| X | X |
| 向上的 Y | **Z** |
| 深度 | Y = 0 为板中心；板深 **40** |

1 设计 px = 1 uu。房间原点 = 左侧入口内侧地面。PlayerStart 在 **(X=48, Z=0)**。

行走面高度用设计 TopY。地面行走面 Z=0；桥与远台 Z=**28**。板厚只往下长：地面厚 **24**，平台厚 **16**。

---

## 1. 建关

1. 内容浏览器：`Content/Maps/Rooms/`。
2. 新建空关卡，保存为 **`Room_Combat_04`**。路径 `Content/Maps/Rooms/Room_Combat_04`。
3. Git LFS。不要提交空文件或假 uasset。
4. 世界设置仍用 `APixelSideGameMode`。不要改 C++。

---

## 2. 按 geometry.csv 摆几何

Actor Label 用 CSV 的 `id`。UE 位置 `(X, 0, Z)`，Z = 设计 Y。

### 2.1 地板与墙（坑区无地板）

| ID | 种类 | 设计范围 | 行走面 / 墙顶 Z | 摆法 |
|----|------|----------|-----------------|------|
| `G_left` | Floor | X 0–720 | 顶 Z=**0** | 入口侧地板。宽 720。中心 X360。 |
| （坑） | Pit | X **720–960** | — | **不要铺地板**。可读坑宽 240。 |
| `G_right` | Floor | X 960–1760 | 顶 Z=**0** | 远侧地板。宽 800。中心 X1360。 |
| `WALL_L` | Wall | X -40–0 | 墙顶 Z=**280** | 左边界。宽 40。 |
| `WALL_R` | Wall | X 1760–1800 | 墙顶 Z=**280** | 右边界。宽 40。 |

入口安全区约 X 0–280。不要把坑做成一落即死的暗坑；也不要在坑里塞假地板。

### 2.2 桥与远台

| ID | 设计 X | TopY | 角色 |
|----|--------|------|------|
| `BRIDGE` | **720–960** | **28** | 常驻桥，跨坑。宽 240。厚 16 只往下长。 |
| `P_far` | **1200–1440** | **28** | 远侧风筝台。宽 240。 |

桥面单跳可达，不是 ≥32 必过台。

### 2.3 掩体（挡弹、人可穿）

| ID | 底心 (X, Z) | 宽 | 高 |
|----|-------------|----|----|
| `CV_A` | (400, 0) | 24 | 44 |
| `CV_B` | (1100, 0) | 24 | 40 |
| `CV_C` | (1560, 0) | 24 | 44 |

碰撞：WorldStatic；**Pawn Ignore**；Visibility / WorldDynamic **Block**；Camera Ignore。Peek 沿 X。

### 2.4 出口 `APixelRoomExit`

| ID | 设计 (X, Y) | `Exit Id` | `bRequireClear` |
|----|-------------|-----------|-----------------|
| `Exit_Left` | (-40, 0) | `Exit.Left` | **不勾** |
| `Exit_Right` | (**1680**, 0) | `Exit.Right` | **勾上** |

`Volume Relative Location` 留 0。左门触发盒会自动往房内偏 +50，不要盖住 (48, 0)。

### 2.5 锚点（不手摆怪 / 不放 SoftRecover 块）

| ID | 设计 (X, Y) | 放什么 |
|----|-------------|--------|
| `PlayerStart` | **(48, 0)** | 引擎 `PlayerStart`。脚底 Z=0；胶囊中心半高 19 + 2。 |
| `RecoverPoint` | (680, 0) | 落坑软回收参考。**不必**放 Actor / 碰撞块。 |
| `AffixAnchor` | (880, 28) | 三选一参考（桥附近）。不必放 Actor。 |

刷怪点不要手摆。CSV 约定以后由导演读：

| ID | 波 | (X, Y) | 占位 |
|----|----|--------|------|
| SP_G1 | 1 | (520, 0) | `Enemy.Grunt.Early` |
| SP_G2 | 1 | (680, 0) | `Enemy.Grunt.Early`，坑缘左 |
| SP_G3 | 2 | (1000, 0) | `Enemy.Grunt.Early`，坑缘右 |
| SP_G4 | 2 | (1320, 0) | `Enemy.Grunt.Early`，远侧 |
| SP_A1 | 2 | (840, 72) | `Enemy.Flyer.Early`，可选，桥上 |

同时存活 ≤6。不要造假 Flyer uasset。不要放 ShopHost。

---

## 3. 导演

1. 原点放一只 **`APixelRoomDirector`**。
2. **`RoomId` = `Room_Combat_04`**。不要写成 `Combat_04`，也不要改成 `Room_Combat_01` 借波次。
3. **`bBuildGreyboxFromCsv` = 不勾。**
4. `bAutoStart`、`bPlacePlayerAtAnchor` 勾上。
5. `FirstWaveDelay` = **0.75**。
6. 不要放 `APixelShopRoomHost`。不要手摆怪。不要放 SoftRecover 碰撞块。

当前导演发现 `RoomId` 不是 `Room_Combat_01` 会停住。这是代码范围。本 PR 不改 C++。

---

## 4. 在编辑器里试

1. 不想看到 Lab 五区时，Play 前开 `PixelSide.DriveRoomCombat01 1`。已有导演则不会再生成一只。该开关不会刷本图波次。
2. 不要开 `DriveRoomShop01`。
3. 不要改 F11、Lab、P0.6。

手拼验收：

- [ ] 宽 1760。`G_left`/`G_right` 分开；X720–960 **无地板**；`BRIDGE` TopY 28 常驻跨坑；`P_far` TopY 28。
- [ ] 三块掩体挡弹、人能穿过。
- [ ] 右门 @1680 勾 `bRequireClear`，左门不勾。VolumeRel=0。
- [ ] 导演 `RoomId=Room_Combat_04`，灰盒关，`FirstWaveDelay` 0.75。无第二套程序化几何。
- [ ] 无手摆怪 / 假 uasset / ShopHost / SoftRecover 碰撞块。
- [ ] 「只驱动 Room_Combat_01」日志可出现；不要改 `RoomId` 去消掉它。

波次、三选一、右门开等运行验收留到导演接本 CSV 后再勾。

---

## 5. 提交真图时

只提交 `Content/Maps/Rooms/Room_Combat_04.umap` 及真正引用配套。Git LFS。不要交空文件、假 uasset 或新 C++。

说明里写：导演已关灰盒，`RoomId` 为 `Room_Combat_04`。更新 `Docs/RoomPipeline.md`。

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 建图清单。真 umap 仍待 Mac / UE 5.8.4。仓库不放假二进制。导演 P0.7 尚未认本 RoomId。 |
| 2026-11-05 | Mac Editor Python 已烘焙真图（LFS；灰盒关；RoomId=Room_Combat_04；坑+桥 G_left/G_right + BRIDGE@28；待 COMMENT 开 PR）。 |
