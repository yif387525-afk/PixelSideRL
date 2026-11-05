# Room_Combat_01 — 正式 umap 建图清单

**引擎：** UE 5.8.4  
**状态：** Mac Editor Python 已烘焙真图 `Content/Maps/Rooms/Room_Combat_01.umap`（`bBuildGreyboxFromCsv` 关；Git LFS）。进仓 / COMMENT PR 待审合。  
**规格：** `Content/Maps/Rooms/Room_Combat_01.md`  
**机读表：** `Content/Data/Rooms/Room_Combat_01_{meta,spawns,geometry}.csv`  
**空关对照：** `Docs/Combat_P0_LabSetup.md` §7。空关继续用 `PixelSide.DriveRoomCombat01 1`，由导演按 CSV 程序化铺灰盒。正式图落地后，关卡里的导演必须关掉 `bBuildGreyboxFromCsv`。和商店清单的验收勾选差异见 [`Room_UMapBuild_AcceptDiff.md`](Room_UMapBuild_AcceptDiff.md)。

不要改 JumpZ、重力、Lab 级差。Peek 仍是穿过掩体后沿 X 站到对侧，不是跳上掩体顶，也不是绕 Y。

---

## 0. 轴与单位

设计文档的 `+Y` 向上。UE 侧视里：

| 设计 | UE |
|------|----|
| X | X |
| 向上的 Y | **Z** |
| 深度 | Y = 0 为板中心；板深 **40**（与现灰盒 `SlabDepth` 一致，让豌豆打到板面） |

1 设计 px = 1 uu。房间原点 = 左侧入口内侧地面（玩家脚底参考），不是 PlayerStart。PlayerStart 在原点右侧 **(X=40, Z=0)**。

行走面高度用设计 TopY，不要把板厚加进 TopY。地面行走面 Z=0，风筝台行走面 Z=28。板厚只往下长：地面厚 **24**，平台厚 **16**（与现灰盒相同，不改跳跃）。

---

## 1. 建关

1. 内容浏览器：`Content/Maps/Rooms/`。
2. 新建空关卡，保存为 **`Room_Combat_01`**。资产路径必须是 `Content/Maps/Rooms/Room_Combat_01`，对应文件 `Content/Maps/Rooms/Room_Combat_01.umap`。
3. 用 Git LFS 跟踪该 umap。不要提交空文件、占位文件或手写假 uasset。
4. 世界设置里仍用现有 `APixelSideGameMode`。不要为了这张图去改 GameMode C++。

---

## 2. 按 geometry.csv 摆几何

下面每一行的 **Actor Label** 用 CSV 的 `id`。坐标是设计坐标；UE 里写成 `(X, 0, Z)`，Z = 设计 Y。

### 2.1 地板与墙（无坑）

| ID | 种类 | 设计范围 | 行走面 / 墙顶 Z | 摆法 |
|----|------|----------|-----------------|------|
| `G_main` | Floor | X 0–1280 | 顶 Z=**0** | 一块连续地板。宽 1280。不要挖坑、不要断开。 |
| `WALL_L` | Wall | X -40–0 | 墙顶 Z=**280** | 左边界。宽 40。 |
| `WALL_R` | Wall | X 1280–1320 | 墙顶 Z=**280** | 右边界。宽 40。 |

地板和墙：碰撞挡 Pawn 和弹体（普通 Block）。

### 2.2 风筝台

| ID | 中心 X | 宽 | TopY |
|----|--------|----|------|
| `P_kite` | **700** | **160** | **28** |

水平范围 X **620–780**，行走面 Z=**28**。单跳可达，**不是**必过台阶。不要再加 TopY≥32 的必过台。

### 2.3 掩体（挡弹、人可穿）

| ID | 底心 (X, Z) | 宽 | 高 |
|----|-------------|----|----|
| `CV_A` | (640, 0) | 24 | 44 |
| `CV_B` | (880, 0) | 24 | 40 |

碰撞与现灰盒 `PassThroughPawn` 一致：

- Object Type：`WorldStatic`
- **Pawn：Ignore**（人穿过去）
- **Visibility：Block**
- **WorldDynamic：Block**（弹体停在这里）
- Camera：Ignore

顶面不是必走平台。Peek = 穿过后站到掩体的另一侧（沿 X），再探出挡线。不要做成必须跳上去或绕到深度侧面。

### 2.4 出口 `APixelRoomExit`

类：`APixelRoomExit`（编辑器显示名 **BP_RoomExit**）。插座中心放在下表的设计点，UE 位置 `(X, 0, 0)`。

| ID | 设计 (X, Y) | `Exit Id` | `bRequireClear` |
|----|-------------|-----------|-----------------|
| `Exit_Left` | (-40, 0) | `Exit.Left` | **不勾** |
| `Exit_Right` | (1200, 0) | `Exit.Right` | **勾上** |

`Exit Id` 必须带 `Exit.` 前缀，和 CSV 的 facing 列一致。右门 Id 写成 `Right` 或 `Exit_Right` 导演认不出。

`Volume Relative Location` 留 `(0,0,0)`。C++ 里左门触发盒会自动往房间内偏 **+50** uu，避免出生点 (40, 0) 一进来就算踩到左门。不要把左门触发盒盖住 PlayerStart。

右门在清房并确认三选一之前应挡住人。左门始终可走，且**不会**把已清状态清掉，也不会把怪再刷出来。

### 2.5 锚点（不生成碰撞块）

| ID | 设计 (X, Y) | 放什么 |
|----|-------------|--------|
| `PlayerStart` | **(40, 0)** | 引擎 `PlayerStart`。脚底在地面 Z=0 上；胶囊中心再抬半高，与现导演一致（半高 + 离地 2 uu）。 |
| `AffixAnchor` | (600, 0) | 只是三选一参考点。UI 可以跟相机，不必放 Actor。 |

刷怪点**不要**手摆进关卡。导演仍读 `Room_Combat_01_spawns.csv`：

| ID | 波 | (X, Y) | 朝向 |
|----|----|--------|------|
| SP_G1 | 1 | (420, 0) | Left |
| SP_G2 | 1 | (560, 0) | Left |
| SP_G3 | 2 | (720, 0) | Left |
| SP_G4 | 2 | (900, 0) | Left |

全部 `Enemy.Grunt.Early`。同时存活 ≤4。入口安全区 X≈0–320 内没有刷怪点。

---

## 3. 导演

1. 在房间原点 `(0, 0, 0)` 放一只 **`APixelRoomDirector`**（显示名 **RoomDirector**）。全关只放一只。
2. **`RoomId` = `Room_Combat_01`**。这是 CSV 文件名和 P0.7 校验用的 id。写成 `Combat_01` 时导演会拒绝启动，也找不到 `Room_Combat_01_*.csv`。
3. **`bBuildGreyboxFromCsv` = 不勾。** 几何已在关卡里。勾上会再程序化铺一套，和手摆重叠。
4. `bAutoStart` 保持勾上。`bPlacePlayerAtAnchor` 保持勾上（把人放到 CSV 的 PlayerStart）。
5. `FirstWaveDelay` 默认 0.75（运行时夹在 0.5–1.0s）。`GruntEarlyClass` 留空即 `APixelEnemyGrunt`。不要为这张图新建敌人 uasset。

刷怪、波次、三选一仍由这只导演读 CSV 完成。手摆只替换灰盒几何和出口插座。

---

## 4. 在编辑器里试

正式图自己当当前关卡 Play 时：

1. 控制台先执行 `PixelSide.DriveRoomCombat01 1`（或在 GameMode 上勾 `bDriveRoomCombat01`）。这样不会再铺 Lab 五区。关卡里已经有导演时，GameMode **不会**再生成第二只。
2. 不要为了这张图去改 F11、Shop、P0.6 三选一或 Lab 数值。

验收（与商店图的差异见 [`Room_UMapBuild_AcceptDiff.md`](Room_UMapBuild_AcceptDiff.md)）：

- [ ] 进房约 0.75s 后只出 W1（G1、G2），两只都死后才出 W2（G3、G4）。同时不超过 4 只。
- [ ] 两波清空后弹出三选一（`RequestAffixChoice(NormalRoom)`）。没选中就关掉，右门仍然挡。
- [ ] 确认一张之后 `Exit.Right` 打开，可以走出去。
- [ ] 清房后踩 `Exit.Left`，已清状态还在，怪不会重刷。
- [ ] 能从地面单跳上 `P_kite`（TopY 28）。没有坑。掩体挡弹、人能穿过，Peek 沿 X。
- [ ] 关卡里看不到第二套程序化灰盒（`bBuildGreyboxFromCsv` 是关的）。

---

## 5. 提交真图时

只提交编辑器保存出来的 `Content/Maps/Rooms/Room_Combat_01.umap`，以及 UE 确实生成、且这张图引用的配套资产。走 Git LFS。

不要提交：

- 空的或手写的 `.umap` / `.uasset`
- 为了占位而复制的别的关卡
- Lab 沙盒改动

提交说明里写明：导演已关 `bBuildGreyboxFromCsv`，`RoomId` 为 `Room_Combat_01`。并更新 `Docs/RoomPipeline.md` 登记表，把「Mac/UE 待拼」改成 umap 已入库。

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 建图清单。真 umap 仍待 Mac / UE 5.8.4。仓库不放假二进制。 |
| 2026-11-05 | 验收勾选与 Shop_01 的差异见 `Docs/Room_UMapBuild_AcceptDiff.md`。 |
