# Room_BossAnte_01 — 正式 umap 建图清单

**引擎：** UE 5.8.4  
**状态：** Mac Editor Python 已烘焙真图 `Content/Maps/Rooms/Room_BossAnte_01.umap`（无 `APixelRoomDirector` / 无 `APixelShopRoomHost` / 无 BossAnte 专用宿主；灰盒关；Git LFS）。进仓 / COMMENT PR 待审合。  
**规格：** `Content/Maps/Rooms/Room_BossAnte_01.md`  
**机读表：** `Content/Data/Rooms/Room_BossAnte_01_{meta,spawns,geometry}.csv`

本房是 **Boss 前厅（BossAnte / BossGate）**：宽 **1280**；平层无坑；无战斗波次、无三选一、无商店、无 RequireClear。前进门 Tag=`Exit.BossGate` → `Room_Boss_01`；左门回退常开。仓库无 BossAnte 宿主类——本图只验几何 / 出口 / BossPreview 锚点。可选 Guide SP 默认关——**不要**在图里放刷怪点。Play 几何验收记明早清单，勿半夜吵用户。不要改 JumpZ、重力、Lab 级差，也不要改手感表。**本批不开** `Room_Boss_01`。

和 Event / Shop 清单的验收勾选差异见 [`Room_UMapBuild_AcceptDiff.md`](Room_UMapBuild_AcceptDiff.md)（BossAnte 更接近 Event：无导演、门常开；但前进门是 `Exit.BossGate` 而非 `Exit.Right`；无 ShopHost）。

---

## 0. 轴与单位

设计文档的 `+Y` 向上。UE 侧视里：

| 设计 | UE |
|------|----|
| X | X |
| 向上的 Y | **Z** |
| 深度 | Y = 0 为板中心；板深 **40**（与现灰盒 `SlabDepth` 一致） |

1 设计 px = 1 uu。房间原点 = 左侧入口内侧地面（玩家脚底参考），不是 PlayerStart。PlayerStart 在原点右侧 **(X=48, Z=0)**。

行走面高度用设计 TopY，不要把板厚加进 TopY。地面行走面 Z=0，门楣台行走面 Z=28。板厚只往下长：地面厚 **24**，平台厚 **16**（与现灰盒相同，不改跳跃）。门楣台不是必过台阶。

---

## 1. 建关

1. 内容浏览器：`Content/Maps/Rooms/`。
2. 新建空关卡，保存为 **`Room_BossAnte_01`**。资产路径必须是 `Content/Maps/Rooms/Room_BossAnte_01`，对应文件 `Content/Maps/Rooms/Room_BossAnte_01.umap`。
3. 用 Git LFS 跟踪该 umap。不要提交空文件、占位文件或手写假 uasset。
4. 世界设置里仍用现有 `APixelSideGameMode`。不要为了这张图去改 GameMode C++。
5. **不要**放 `APixelRoomDirector`、`APixelShopRoomHost`，也不要新建 BossAnteHost 类。
6. **不要**放 AffixAnchor、手摆怪、引导刷点 actor、伤害体积。

---

## 2. 按 geometry.csv 摆几何

下面每一行的 **Actor Label** 用 CSV 的 `id`。坐标是设计坐标；UE 里写成 `(X, 0, Z)`，Z = 设计 Y。

### 2.1 地板与墙（无坑）

| ID | 种类 | 设计范围 | 行走面 / 墙顶 Z | 摆法 |
|----|------|----------|-----------------|------|
| `G_main` | Floor | X 0–1280 | 顶 Z=**0** | 一块连续地板。宽 1280。中心 X640。安全区，不要挖坑、不要断开。 |
| `WALL_L` | Wall | X -40–0 | 墙顶 Z=**280** | 左边界。宽 40。 |
| `WALL_R` | Wall | X 1280–1320 | 墙顶 Z=**280** | 右边界。宽 40。 |

地板和墙：碰撞挡 Pawn 和弹体（普通 Block）。入口安全区约 X 0–200，里面没有威胁。

### 2.2 门楣气氛台

| ID | 设计 X | TopY | 角色 |
|----|--------|------|------|
| `P_lintel` | **960–1160** | **28** | 门楣侧短台。宽 200。厚 16 只往下长。**不是**必过台阶。 |

行走面 Z=**28**。不要再加 TopY≥32 的台阶。无第二层；无战斗压制台。

### 2.3 短掩体（挡弹、人可穿；气氛）

| ID | 底心 (X, Z) | 宽 | 高 |
|----|-------------|----|----|
| `CV_A` | (480, 0) | 24 | 40 |

碰撞与现灰盒 `PassThroughPawn` 一致：

- Object Type：`WorldStatic`
- **Pawn：Ignore**
- **Visibility：Block**
- **WorldDynamic：Block**（弹体停在这里）
- Camera：Ignore

无战斗需求；掩体仅打破空白盒感。不要做成必走平台。

### 2.4 出口 `APixelRoomExit`（两扇都不锁）

类：`APixelRoomExit`（编辑器显示名 **BP_RoomExit**）。插座中心放在下表的设计点，UE 位置 `(X, 0, 0)`。

| ID | 设计 (X, Y) | `Exit Id` | `bRequireClear` |
|----|-------------|-----------|-----------------|
| `Exit_Left` | (-40, 0) | `Exit.Left` | **不勾** |
| `Exit_BossGate` | (1200, 0) | **`Exit.BossGate`** | **不勾** |

左门回退常开；前进门进 `Room_Boss_01`（Tag 必须是 `Exit.BossGate`，不是 `Exit.Right`）。没有清房锁，没有 `RequireClear`，没有三选一。`Exit Id` 必须带 `Exit.` 前缀。

`Volume Relative Location` 留 `(0,0,0)`。C++ 里左门触发盒会自动往房间内偏 **+50** uu，避免出生点 (48, 0) 一进来就算踩到左门。不要把左门触发盒盖住 PlayerStart。

本房没有战斗导演，也没有商店宿主。门的开关不交给 `APixelRoomDirector`。

### 2.5 锚点（不生成碰撞块）

| ID | 设计 (X, Y) | 放什么 |
|----|-------------|--------|
| `PlayerStart` | **(48, 0)** | 引擎 `PlayerStart`。脚底在地面 Z=0 上；胶囊中心再抬半高（半高 + 离地 2 uu → Z≈21）。 |
| `BossPreview` | **(1100, 0)** | Boss 预告镜头 / 门扉参考（TargetPoint / Note）。**不要**放假 uasset。 |

`AffixAnchor`：**不使用**（无三选一）。

可选 Guide SP（`SP_G1` / `SP_G2`，`Optional=true`）默认关——**不要**在正式 umap 里放刷怪点 actor。`Room_BossAnte_01_spawns.csv` 仅作规格表。不要放 `APixelEnemyGrunt`，不要放伤害体积。

---

## 3. 进房宿主

**本图不放宿主。**

- 仓库无 BossAnte 专用宿主 C++。不要新建类，不要从 ShopHost / RoomDirector 拷贝改名。
- 本图只验几何、出口 Tag、BossPreview 锚点。Boss 房本体本批不开。

---

## 4. 在编辑器里试

正式图自己当当前关卡 Play 时（几何验收，**记明早清单**）：

1. 不要开战斗驱动脚本指望本图跑逻辑——本图只验几何与门。
2. 不要为了这张图去改 Lab 数值或 P0.6 三选一。
3. **`Room_Boss_01` 本批仍不开。**

验收勾选：

- [ ] 无刷怪点、无伤害体积、无 `APixelRoomDirector`、无 `APixelShopRoomHost`、无 AffixAnchor。
- [ ] `Exit_Left` / `Exit_BossGate` 均为 `bRequireClear=false`；前进门 ExitId=`Exit.BossGate`。
- [ ] 无坑。门楣台 TopY 28，不是必过台；宽 1280。
- [ ] BossPreview @X=1100；PlayerStart @X=48。
- [ ] 无三选一；关卡里无假 uasset。
- [ ] Play 几何走查记明早，勿半夜吵用户。

---

## 5. 提交真图时

只提交编辑器保存出来的 `Content/Maps/Rooms/Room_BossAnte_01.umap`，以及本批配套的：

- `Tools/BakeRoomBossAnte01.py`
- `Docs/Room_BossAnte_01_UMapBuild.md`
- `Docs/RoomPipeline.md`（登记表更新）

走 Git LFS。不要提交空的或手写的 `.umap` / `.uasset`，不要为了占位而复制别的关卡，不要改 C++ / Config / 手感表。

提交说明里写明：无 Director / 无 ShopHost；两门 `RequireClear=false`；`Exit.BossGate` → Boss_01；宽 1280；无战斗。并更新 `Docs/RoomPipeline.md` 登记表。**未授权前勿 push / 开 PR。**

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 建图清单 + Mac Editor Python 烘焙真图（LFS；灰盒关；无宿主类；待 COMMENT 开 PR）。 |
