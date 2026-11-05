# Room_Boss_01 — 正式 umap 建图清单

**引擎：** UE 5.8.4  
**状态：** Mac Editor Python 已烘焙真图 `Content/Maps/Rooms/Room_Boss_01.umap`（有 `APixelRoomDirector`；`RoomId=Room_Boss_01`；灰盒关；无 `APixelShopRoomHost`；无手摆怪 / 伤害体积 / AffixAnchor；Git LFS）。进仓 / COMMENT PR 待审合。  
**规格：** `Content/Maps/Rooms/Room_Boss_01.md`  
**机读表：** `Content/Data/Rooms/Room_Boss_01_{meta,spawns,geometry}.csv`

本房是 **正式 Boss 房**：宽 **2560**；**双阶段**场地；`P_sideL`/`P_sideR` TopY=**28**；`P_p2` TopY=**56**；连续地板无坑。入口仅自 `Room_BossAnte_01`（`Exit.FromAnte`，Intro 后锁）；结算出口 `Exit.Settle` 在 **BossDefeated** 后解锁。**无三选一**。Play 几何验收记明早清单，勿半夜吵用户。不要改 JumpZ、重力、Lab 级差，也不要改手感表。不要改 BossAnte 文件。

和 Combat 清单的差异：出口 Tag 是 `Exit.FromAnte` / `Exit.Settle`（不是 Left/Right）；清房规则是 `BossDefeated`；无 AffixAnchor；`SP_Boss` 只放 TargetPoint 标记（Tag=`Spawn.Boss`），杂兵 SP 不手摆。

---

## 0. 轴与单位

设计文档的 `+Y` 向上。UE 侧视里：

| 设计 | UE |
|------|----|
| X | X |
| 向上的 Y | **Z** |
| 深度 | Y = 0 为板中心；板深 **40**（与现灰盒 `SlabDepth` 一致） |

1 设计 px = 1 uu。房间原点 = 左侧入口内侧地面。PlayerStart 脚底在 **(X=80, Z=0)**；胶囊中心 Z≈21（半高 19 + 离地 2）。

行走面高度用设计 TopY。地面行走面 Z=0；侧台 Z=**28**；P2 中央台 Z=**56**。板厚只往下长：地面厚 **24**，平台厚 **16**。级差 ≤28（地面→侧台 +28 → P_p2 再 +28）。

---

## 1. 建关

1. 内容浏览器：`Content/Maps/Rooms/`。
2. 新建空关卡，保存为 **`Room_Boss_01`**。资产路径必须是 `Content/Maps/Rooms/Room_Boss_01`。
3. 用 Git LFS 跟踪该 umap。不要提交空文件、占位文件或手写假 uasset。
4. 世界设置里仍用现有 `APixelSideGameMode`。不要为了这张图去改 GameMode C++。
5. 放一只 **`APixelRoomDirector`**（规格挂则挂）。**不要**放 `APixelShopRoomHost`。
6. **不要**放 AffixAnchor、手摆怪、真伤害体积。

---

## 2. 按 geometry.csv 摆几何

Actor Label 用 CSV 的 `id`。UE 位置 `(X, 0, Z)`，Z = 设计 Y。

### 2.1 地板与墙（无坑）

| ID | 种类 | 设计范围 | 行走面 / 墙顶 Z | 摆法 |
|----|------|----------|-----------------|------|
| `G_main` | Floor | X 0–**2560** | 顶 Z=**0** | 主场连续。宽 2560。中心 X1280。**无暗坑**。 |
| `WALL_L` | Wall | X -40–0 | 墙顶 Z=**280** | 左边界。宽 40。Intro 后入口锁。 |
| `WALL_R` | Wall | X 2560–2600 | 墙顶 Z=**280** | 右边界。宽 40。 |

### 2.2 平台（厚度 16；级差 ≤28）

| ID | 设计 X | TopY | 阶段 | 角色 |
|----|--------|------|------|------|
| `P_sideL` | **400–640** | **28** | P1+ | 左侧半开放高台。宽 240。 |
| `P_sideR` | **1760–2080** | **28** | P1+ | 右侧半开放高台。宽 320。 |
| `P_p2` | **1120–1520** | **56** | **P2** | 转场后启用中央高台。宽 400。 |

跳接：地面→侧台 (+28)→`P_p2`（再 +28）。无单级 ≥32 必过。

### 2.3 短掩体（挡弹；Pawn 可穿）

| ID | 底心 (X, Z) | 宽 | 高 |
|----|-------------|----|----|
| `CV_A` | (720, 0) | 24 | 44 |
| `CV_B` | (1440, 0) | 24 | 40 |
| `CV_C` | (2160, 0) | 24 | 44 |

碰撞与现灰盒 `PassThroughPawn` 一致：WorldStatic；**Pawn Ignore**；Visibility / WorldDynamic **Block**；Camera Ignore。

### 2.4 转场危害锚点（占位；勿放真伤害体积）

| ID | 底心 (X, Z) | 放什么 |
|----|-------------|--------|
| `Haz_A` | (960, 0) | TargetPoint / Note 标记。TelegraphThenActive 规格；**不要**放伤害体积。 |
| `Haz_B` | (1680, 0) | 同上。 |

### 2.5 出口 `APixelRoomExit`

| ID | 设计 (X, Y) | `Exit Id` | `bRequireClear` |
|----|-------------|-----------|-----------------|
| `Exit_FromAnte` | (-40, 0) | **`Exit.FromAnte`** | **不勾**（VolumeRel=0；Intro 后锁回退） |
| `Exit_Settle` | (2480, 0) | **`Exit.Settle`** | **勾上**（BossDefeated 后解锁；进结算；无三选一） |

`Volume Relative Location` 留 `(0,0,0)`。C++ 左门触发盒会自动往房间内偏，避免盖住 PlayerStart。

### 2.6 锚点

| ID | 设计 (X, Y) | 放什么 |
|----|-------------|--------|
| `PlayerStart` | **(80, 0)** | 引擎 `PlayerStart`。脚底 Z=0；胶囊中心 ≈21。 |
| `CameraIntro` | **(640, 80)** | Intro 镜头参考（TargetPoint / Note）。 |
| `SP_Boss` | **(1280, 0)** | TargetPoint；Tag=`Spawn.Boss`。**唯一** Boss 点；**不要**手摆怪 actor。 |
| `AffixAnchor` | — | **不使用**（无三选一）。 |

可选杂兵 `SP_A1`/`SP_A2`/`SP_A3`：**不要**手摆。Director 读 CSV 即可。同时存活 Boss 1 + 杂兵 ≤3。

---

## 3. 导演

1. 原点放一只 **`APixelRoomDirector`**。
2. **`RoomId` = `Room_Boss_01`**。
3. **`bBuildGreyboxFromCsv` = 不勾。**
4. `bAutoStart`、`bPlacePlayerAtAnchor` 勾上。
5. `FirstWaveDelay` = **0.75**（对照 Combat_06）。
6. **断言无** `APixelShopRoomHost`。不要手摆怪 / 伤害体积 / AffixAnchor。

当前导演 P0.7 可能只认 Combat_01；本图 Play 几何验收即可，波次 / BossDefeated 门逻辑以后再接。本 PR 不改 C++。

---

## 4. 在编辑器里试

正式图自己当当前关卡 Play 时（几何验收，**记明早清单**）：

1. 不要为了这张图去改 Lab 数值或 P0.6 三选一。
2. 不要开 `DriveRoomShop01`。
3. 不要改 BossAnte / 其他房文件。

验收勾选：

- [ ] 宽 2560。`G_main` 连续无坑；`P_sideL`/`P_sideR` TopY 28；`P_p2` TopY 56。
- [ ] 三块掩体挡弹、人能穿过。Haz 仅标记，无真伤害体积。
- [ ] `Exit_FromAnte` RequireClear=false；`Exit_Settle` RequireClear=true；VolumeRel=0。
- [ ] 导演 `RoomId=Room_Boss_01`，灰盒关；无 ShopHost。
- [ ] `SP_Boss` @1280 Tag=`Spawn.Boss`；无手摆怪；无 AffixAnchor；无假 uasset。
- [ ] Play 几何走查记明早，勿半夜吵用户。

---

## 5. 提交真图时

只提交编辑器保存出来的 `Content/Maps/Rooms/Room_Boss_01.umap`，以及本批配套的：

- `Tools/BakeRoomBoss01.py`
- `Docs/Room_Boss_01_UMapBuild.md`
- `Docs/RoomPipeline.md`（登记表更新）

走 Git LFS。不要提交空的或手写的 `.umap` / `.uasset`，不要改 C++ / Config / 手感表，不要改 BossAnte 文件。

提交说明里写明：有 Director / 无 ShopHost；`Exit.FromAnte`/`Exit.Settle`；宽 2560 双阶段；无三选一。并更新 `Docs/RoomPipeline.md` 登记表。**未授权前勿 push / 开 PR。**

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 建图清单 + Mac Editor Python 烘焙真图（LFS；灰盒关；RoomId=Room_Boss_01；双阶段；待 COMMENT 开 PR）。 |
