# Room_Shop_01 — 正式 umap 建图清单

**引擎：** UE 5.8.4  
**状态：** 清单已写。**仓库里还没有** `Room_Shop_01.umap`。本文件不附带任何 `.umap` / `.uasset`。Mac 上用编辑器按下面摆好之后，再单独提交真图（Git LFS）。  
**规格：** `Content/Maps/Rooms/Room_Shop_01.md`  
**机读表：** `Content/Data/Rooms/Room_Shop_01_{meta,spawns,geometry}.csv`  
**进房：** `APixelShopRoomHost`（显示名 **ShopRoomHost**）。空关用 `PixelSide.DriveRoomShop01 1`（或 GameMode 上勾 `bDriveRoomShop01`）。宿主经 `AC_ShopPresenter` 调 `OpenShop`，读真 `RunScrap`。**不走** `APixelRoomDirector`，也不靠 `bBuildGreyboxFromCsv` 铺正式图。与 `DriveRoomCombat01` 同时打开时，商店让路。见 `Room_Shop_01.md` §7。和战斗清单的验收勾选差异见 [`Room_UMapBuild_AcceptDiff.md`](Room_UMapBuild_AcceptDiff.md)。

不要改 JumpZ、重力、Lab 级差，也不要改 F11、Presenter 或 P0.6 三选一。本房无刷怪、无三选一、无强制购买。门常开。

---

## 0. 轴与单位

设计文档的 `+Y` 向上。UE 侧视里：

| 设计 | UE |
|------|----|
| X | X |
| 向上的 Y | **Z** |
| 深度 | Y = 0 为板中心；板深 **40**（与现灰盒 `SlabDepth` 一致） |

1 设计 px = 1 uu。房间原点 = 左侧入口内侧地面（玩家脚底参考），不是 PlayerStart。PlayerStart 在原点右侧 **(X=48, Z=0)**。

行走面高度用设计 TopY，不要把板厚加进 TopY。地面行走面 Z=0，货架台行走面 Z=28。板厚只往下长：地面厚 **24**，平台厚 **16**（与现灰盒相同，不改跳跃）。货架台不是必过台阶。

---

## 1. 建关

1. 内容浏览器：`Content/Maps/Rooms/`。
2. 新建空关卡，保存为 **`Room_Shop_01`**。资产路径必须是 `Content/Maps/Rooms/Room_Shop_01`，对应文件 `Content/Maps/Rooms/Room_Shop_01.umap`。
3. 用 Git LFS 跟踪该 umap。不要提交空文件、占位文件或手写假 uasset。
4. 世界设置里仍用现有 `APixelSideGameMode`。不要为了这张图去改 GameMode C++。

---

## 2. 按 geometry.csv 摆几何

下面每一行的 **Actor Label** 用 CSV 的 `id`。坐标是设计坐标；UE 里写成 `(X, 0, Z)`，Z = 设计 Y。

### 2.1 地板与墙（无坑）

| ID | 种类 | 设计范围 | 行走面 / 墙顶 Z | 摆法 |
|----|------|----------|-----------------|------|
| `G_main` | Floor | X 0–1280 | 顶 Z=**0** | 一块连续地板。宽 1280。安全区，不要挖坑、不要断开。 |
| `WALL_L` | Wall | X -40–0 | 墙顶 Z=**280** | 左边界。宽 40。 |
| `WALL_R` | Wall | X 1280–1320 | 墙顶 Z=**280** | 右边界。宽 40。 |

地板和墙：碰撞挡 Pawn 和弹体（普通 Block）。入口安全区约 X 0–240，里面没有威胁。

### 2.2 货架台

| ID | 设计 X | TopY | 角色 |
|----|--------|------|------|
| `P_shelf` | **720–1040** | **28** | 气氛货架台。宽 320。商品可读站位，**不是**必过台阶。 |

行走面 Z=**28**。不要再加 TopY≥32 的台阶。

### 2.3 柜台（挡弹、人可穿）

| ID | 底心 (X, Z) | 宽 | 高 |
|----|-------------|----|----|
| `CV_counter` | (400, 0) | 64 | 36 |

水平范围 X **368–432**。碰撞与现灰盒 `PassThroughPawn` 一致：

- Object Type：`WorldStatic`
- **Pawn：Ignore**（人能从柜台处走过；2D 里没有可绕的 Y 侧面）
- **Visibility：Block**
- **WorldDynamic：Block**（弹体停在这里）
- Camera：Ignore

这是柜台体块，不是战斗掩体顶。不要做成必走平台。

### 2.4 出口 `APixelRoomExit`（两扇都不锁）

类：`APixelRoomExit`（编辑器显示名 **BP_RoomExit**）。插座中心放在下表的设计点，UE 位置 `(X, 0, 0)`。

| ID | 设计 (X, Y) | `Exit Id` | `bRequireClear` |
|----|-------------|-----------|-----------------|
| `Exit_Left` | (-40, 0) | `Exit.Left` | **不勾** |
| `Exit_Right` | (1200, 0) | `Exit.Right` | **不勾** |

两扇门都回主干，**常开**。没有清房锁，没有 `RequireClear`，没有三选一。`Exit Id` 必须带 `Exit.` 前缀。不要勾右门的 `bRequireClear`。

`Volume Relative Location` 留 `(0,0,0)`。C++ 里左门触发盒会自动往房间内偏 **+50** uu，避免出生点 (48, 0) 一进来就算踩到左门。不要把左门触发盒盖住 PlayerStart。

本房没有战斗导演。门的开关不交给 `APixelRoomDirector`。

### 2.5 锚点（不生成碰撞块）

| ID | 设计 (X, Y) | 放什么 |
|----|-------------|--------|
| `PlayerStart` | **(48, 0)** | 引擎 `PlayerStart`。脚底在地面 Z=0 上；胶囊中心再抬半高（半高 + 离地 2 uu）。 |
| `ShopKeep` | (400, 0) | 店主 / 柜台交互标记。与柜台同一 X。可选形象，不要挡路。 |
| `Slot_1` | (720, 0) | 商品位 1。货由 `DA_Shop*` 驱动，不要在关卡里摆武器 uasset。 |
| `Slot_2` | (840, 0) | 商品位 2。 |
| `Slot_3` | (960, 0) | 商品位 3。 |
| `LeaveHint` | (1100, 0) | 离开 / 跳过提示，靠近右门。 |

`Room_Shop_01_spawns.csv` **只有表头**。不要放刷怪点，不要放 `APixelEnemyGrunt`。`max_alive=0`。

---

## 3. 进房宿主

1. 在房间原点 `(0, 0, 0)` 放一只 **`APixelShopRoomHost`**（显示名 **ShopRoomHost**）。全关只放一只。
2. **`RoomId` = `Room_Shop_01`**。这是 CSV 文件名。写成 `Shop_01` 时宿主读不到 `Room_Shop_01_meta.csv`。
3. **`bBuildGreyboxFromCsv` = 不勾。** 几何已在关卡里。勾上会再程序化铺一套，和手摆重叠。正式图不依赖这个开关。
4. `bAutoOpen` 保持勾上。`bPlacePlayerAtAnchor` 保持勾上（把人放到 CSV 的 PlayerStart）。
5. **不要**放 `APixelRoomDirector`。商店不跑战斗时钟，不刷两波，不弹三选一。

开店仍由宿主完成：`CollectShelfOffers(1)` 读 `Content/Data/Shop/DA_ShopInventory.json` 与 `DA_ShopPriceCurve.json`，再 `GetShopPresenter()->OpenShop(...)`。壳读 `GetRunScrap`。买 / 刷新成功才 `AddRunScrap`。不要把这条路径改成 `OpenShopDemo`。F11 仍是 Lab Demo，不经过本宿主。

---

## 4. 在编辑器里试

正式图自己当当前关卡 Play 时：

1. 控制台先执行 `PixelSide.DriveRoomShop01 1`（或在 GameMode 上勾 `bDriveRoomShop01`）。这样不会再铺 Lab 五区。关卡里已经有宿主时，GameMode **不会**再生成第二只。
2. **不要**同时开 `PixelSide.DriveRoomCombat01`。两者一起开时商店让路，这张图不会开店。
3. 不要为了这张图去改 F11、Lab 数值或 P0.6 三选一。

验收（与战斗图的差异见 [`Room_UMapBuild_AcceptDiff.md`](Room_UMapBuild_AcceptDiff.md)）：

- [ ] 进房后 `AC_ShopPresenter` 打开商店，余额是真 `RunScrap`，不是 Demo 的 128。
- [ ] 三件商品可读。买得起会扣 `RunScrap`；买不起格子变灰，不扣款。
- [ ] 可以不买。Esc、`离开` 或 `CloseShop` 关店后，左右门都能走，没有强制购买。
- [ ] 关卡里没有刷怪点，也没有 `APixelRoomDirector`。
- [ ] 关卡里看不到第二套程序化灰盒（宿主的 `bBuildGreyboxFromCsv` 是关的）。
- [ ] 没有坑。货架台 TopY 28，不是必过台。

---

## 5. 提交真图时

只提交编辑器保存出来的 `Content/Maps/Rooms/Room_Shop_01.umap`，以及 UE 确实生成、且这张图引用的配套资产。走 Git LFS。

不要提交：

- 空的或手写的 `.umap` / `.uasset`
- 为了占位而复制的别的关卡
- 商品 DataAsset、Lab 沙盒或 Presenter 代码改动

提交说明里写明：宿主已关 `bBuildGreyboxFromCsv`，`RoomId` 为 `Room_Shop_01`，没有战斗导演。并更新 `Docs/RoomPipeline.md` 登记表，把「Mac/UE 待拼」改成 umap 已入库。

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 建图清单。真 umap 仍待 Mac / UE 5.8.4。仓库不放假二进制。 |
| 2026-11-05 | 验收勾选与 Combat_01 的差异见 `Docs/Room_UMapBuild_AcceptDiff.md`。 |
