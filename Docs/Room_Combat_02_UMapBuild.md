# Room_Combat_02 — 正式 umap 建图清单

**引擎：** UE 5.8.4  
**状态：** 正式 umap 已入库 `Content/Maps/Rooms/Room_Combat_02.umap`（Mac Editor Python 烘焙；Git LFS；magic `c1832a9e`；灰盒关；`RoomId=Room_Combat_02`；`FirstWaveDelay=0.75`）。本文件是建图清单，不附带二进制。导演 P0.7 仍只认 Combat_01，本图 Play 不跑波次。  
**规格：** `Content/Maps/Rooms/Room_Combat_02.md`  
**机读表：** `Content/Data/Rooms/Room_Combat_02_{meta,spawns,geometry}.csv`  
**对照：** 章节与 [`Room_Combat_01_UMapBuild.md`](Room_Combat_01_UMapBuild.md) 相同。本房是一层平台（`P_L`/`P_R` TopY=28）+ W2 Air，Early 教学高低差。宽 **1600**，比 Combat_01 更长。

不要改 JumpZ、重力、Lab 级差。Peek 仍是穿过掩体后沿 X 站到对侧。

**运行时边界（勿臆造）：** 当前导演 P0.7 逻辑只认 `RoomId=Room_Combat_01`。本图的 `RoomId` 仍必须是 `Room_Combat_02`。**现在 Play 不会跑本图波次（不会按 Combat_02 CSV 刷 Ground×3 + Air×1）。** 手拼验收查几何和出口。波次以后再接 CSV，本 PR 不改 C++。

---

## 0. 轴与单位

设计文档的 `+Y` 向上。UE 侧视里：

| 设计 | UE |
|------|----|
| X | X |
| 向上的 Y | **Z** |
| 深度 | Y = 0 为板中心；板深 **40**（与现灰盒 `SlabDepth` 一致） |

1 设计 px = 1 uu。房间原点 = 左侧入口内侧地面。PlayerStart 在原点右侧 **(X=40, Z=0)**。

行走面高度用设计 TopY，不要把板厚加进 TopY。地面行走面 Z=0，一层平台行走面 Z=**28**。板厚只往下长：地面厚 **24**，平台厚 **16**。

---

## 1. 建关

1. 内容浏览器：`Content/Maps/Rooms/`。
2. 新建空关卡，保存为 **`Room_Combat_02`**。资产路径必须是 `Content/Maps/Rooms/Room_Combat_02`，对应文件 `Content/Maps/Rooms/Room_Combat_02.umap`。
3. 用 Git LFS 跟踪该 umap。不要提交空文件、占位文件或手写假 uasset。
4. 世界设置里仍用现有 `APixelSideGameMode`。不要为了这张图去改 GameMode C++。

---

## 2. 按 geometry.csv 摆几何

下面每一行的 **Actor Label** 用 CSV 的 `id`。坐标是设计坐标；UE 里写成 `(X, 0, Z)`，Z = 设计 Y。

### 2.1 地板与墙（无坑）

| ID | 种类 | 设计范围 | 行走面 / 墙顶 Z | 摆法 |
|----|------|----------|-----------------|------|
| `G_main` | Floor | X 0–1600 | 顶 Z=**0** | 一块连续地板。宽 1600。不要挖坑、不要断开。平台中间缺口仍是地面。 |
| `WALL_L` | Wall | X -40–0 | 墙顶 Z=**280** | 左边界。宽 40。 |
| `WALL_R` | Wall | X 1600–1640 | 墙顶 Z=**280** | 右边界。宽 40。 |

地板和墙：碰撞挡 Pawn 和弹体（普通 Block）。入口安全区约 X 0–320。

### 2.2 一层平台

| ID | 设计 X | TopY | 角色 |
|----|--------|------|------|
| `P_L` | **520–760** | **28** | 近侧一层台。宽 240。 |
| `P_R` | **820–1120** | **28** | 远侧一层台。宽 300。 |

中间缺口 X **760–820**（约 60）：地面可绕过，不要挖坑、不要封墙。单跳可达，**不是** ≥32 的必过台。不要再加第二层。

### 2.3 掩体（挡弹、人可穿）

| ID | 底心 (X, Z) | 宽 | 高 |
|----|-------------|----|----|
| `CV_A` | (400, 0) | 24 | 44 |
| `CV_B` | (1000, 0) | 24 | 40 |

碰撞与现灰盒 `PassThroughPawn` 一致：

- Object Type：`WorldStatic`
- **Pawn：Ignore**（人穿过去）
- **Visibility：Block**
- **WorldDynamic：Block**（弹体停在这里）
- Camera：Ignore

Peek = 穿过后站到掩体的另一侧（沿 X）。不要做成必须跳上去或绕到深度侧面。

### 2.4 出口 `APixelRoomExit`

类：`APixelRoomExit`（编辑器显示名 **BP_RoomExit**）。插座中心放在下表的设计点，UE 位置 `(X, 0, 0)`。

| ID | 设计 (X, Y) | `Exit Id` | `bRequireClear` |
|----|-------------|-----------|-----------------|
| `Exit_Left` | (-40, 0) | `Exit.Left` | **不勾** |
| `Exit_Right` | (**1520**, 0) | `Exit.Right` | **勾上** |

`Exit Id` 必须带 `Exit.` 前缀。`Volume Relative Location` 留 `(0,0,0)`。C++ 里左门触发盒会自动往房间内偏 **+50** uu，不要盖住 PlayerStart (40, 0)。

右门在清房并确认三选一之前应挡住人。左门始终可走，且**不会**把已清状态清掉。

### 2.5 锚点（不生成碰撞块）

| ID | 设计 (X, Y) | 放什么 |
|----|-------------|--------|
| `PlayerStart` | **(40, 0)** | 引擎 `PlayerStart`。脚底在地面 Z=0 上；胶囊中心再抬半高 + 2 uu（半高 19）。 |
| `AffixAnchor` | (720, 0) | 只是三选一参考点。不必放 Actor。 |

刷怪点**不要**手摆进关卡。CSV 约定以后由导演读：

| ID | 波 | (X, Y) | 占位 |
|----|----|--------|------|
| SP_G1 | 1 | (480, 0) | `Enemy.Grunt.Early`，近侧 |
| SP_G2 | 1 | (640, 0) | `Enemy.Grunt.Early`，台下 |
| SP_G3 | 2 | (1080, 0) | `Enemy.Grunt.Early`，远侧 |
| SP_A1 | 2 | (960, 88) | `Enemy.Flyer.Early`，P_R 上方，仅 W2 |

同时存活 ≤5。入口安全区无刷怪点。现在没有 Flyer 假 uasset 可摆，也不要造。

---

## 3. 导演

1. 在房间原点 `(0, 0, 0)` 放一只 **`APixelRoomDirector`**（显示名 **RoomDirector**）。全关只放一只。
2. **`RoomId` = `Room_Combat_02`**。不要写成 `Combat_02`，也不要改成 `Room_Combat_01` 来借战斗波次。
3. **`bBuildGreyboxFromCsv` = 不勾。** 几何已在关卡里。勾上会再程序化铺一套。
4. `bAutoStart`、`bPlacePlayerAtAnchor` 保持勾上。
5. `FirstWaveDelay` = **0.75**（运行时夹在 0.5–1.0s）。
6. 不要放 `APixelShopRoomHost`。不要手摆怪。

当前 `StartRoom` 发现 `RoomId` 不是 `Room_Combat_01` 会打日志并停住。这是代码范围，不是摆台摆错。本 PR 不改 C++。

---

## 4. 在编辑器里试

1. 若不想同时看到 Lab 五区，Play 前开 `PixelSide.DriveRoomCombat01 1`。关卡里已有导演时不会再生成第二只。这个开关**不会**让本图按 Combat_02 CSV 刷波。
2. 不要开 `DriveRoomShop01`。
3. 不要改 F11、Lab、P0.6。

手拼验收：

- [ ] 宽 1600，地面连续无坑。`P_L`/`P_R` TopY 28，中间缺口 760–820 可从地面绕过。
- [ ] 两块掩体挡弹、人能穿过，Peek 沿 X。
- [ ] 右门 `Exit.Right` @1520 勾了 `bRequireClear`，左门没勾。VolumeRel=0。
- [ ] 导演在原点，`RoomId` 是 `Room_Combat_02`，`bBuildGreyboxFromCsv` 关着，`FirstWaveDelay` 0.75。没有第二套程序化灰盒。
- [ ] 关卡里没有手摆的怪，也没有假 uasset / ShopHost。
- [ ] 日志里出现「只驱动 Room_Combat_01」是当前代码的范围，不要为了消掉它去改 `RoomId`。

规格上的运行验收（W1 Ground×2 → W2 Ground+Air、三选一后右门开、左门不重置）留到导演真正读这张 CSV 之后再勾。本清单不把它写成现在就能玩过。

---

## 5. 提交真图时

只提交编辑器保存出来的 `Content/Maps/Rooms/Room_Combat_02.umap`，以及 UE 确实生成、且这张图引用的配套资产。走 Git LFS。

不要提交空文件、假 uasset、Lab 沙盒改动或新的 C++。

说明里写明：导演已关 `bBuildGreyboxFromCsv`，`RoomId` 为 `Room_Combat_02`。并更新 `Docs/RoomPipeline.md` 登记表。

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 建图清单。真 umap 仍待 Mac / UE 5.8.4。仓库不放假二进制。导演 P0.7 尚未认本 RoomId。 |
| 2026-11-05 | Mac Editor Python 已烘焙真图（LFS；灰盒关；RoomId=Room_Combat_02；一层平台 P_L/P_R；待 COMMENT 开 PR）。 |
