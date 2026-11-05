# Combat_01 / Shop_01 — umap 建图验收勾选差异

**日期：** 2026-11-05  
**用途：** Mac 上线前，对照两张已写清单的验收勾选。不新开房间规格，不提交 `.umap` / `.uasset`。  
**清单：** [`Room_Combat_01_UMapBuild.md`](Room_Combat_01_UMapBuild.md) · [`Room_Shop_01_UMapBuild.md`](Room_Shop_01_UMapBuild.md)

真图都还没进仓库。灰盒可以先在空关验：Combat 用 `PixelSide.DriveRoomCombat01 1`，Shop 用 `PixelSide.DriveRoomShop01 1`。两个开关不要同时开。

---

## 相同项

两张图手拼时，这些勾选是一样的。

| 项 | 两边都要 |
|----|----------|
| 引擎 | UE 5.8.4。世界设置仍用现有 `APixelSideGameMode`，不改 C++。 |
| 路径 | `Content/Maps/Rooms/Room_<名>`。真 umap 走 Git LFS。不提交空文件或假 uasset。 |
| 原点 | 左侧入口内侧地面。PlayerStart 不是原点。 |
| 轴 | 设计 X → UE X；设计向上的 Y → UE **Z**；深度 Y=0 为板中心，板深 40。 |
| 单位 | 1 设计 px = 1 uu。宽 **1280**。地面连续，**无坑**。行走面用 TopY，板厚只往下长（地面 24，平台 16）。不改 JumpZ / Lab 级差。 |
| 墙 | `WALL_L` X -40–0，`WALL_R` X 1280–1320，墙顶 Z=280。地板和墙挡人和弹。 |
| 出口类 | `APixelRoomExit`（显示名 BP_RoomExit）。`Exit Id` 必须是 `Exit.Left` / `Exit.Right`。左门插座 X=-40，右门插座 X=1200。`Volume Relative Location` 留 0，左门触发盒会自己往房内偏 +50。 |
| 灰盒开关 | 正式图上的宿主 **`bBuildGreyboxFromCsv` 不勾**。勾上会再铺一套，和手摆重叠。 |
| 人放哪 | `bPlacePlayerAtAnchor` 保持勾上。脚底在 Z=0，胶囊中心再抬半高 + 2 uu。 |
| 空关先验 | 对应的 Drive 开关打开后不铺 Lab 五区。关卡里已经有一只宿主时，GameMode 不再生成第二只。 |
| 不要动 | F11、Lab 手感、P0.6 三选一代码。本对照文也不改这些。 |

---

## 差异项

勾验收时按房间分开，不要把战斗项勾到商店上。

| 项 | `Room_Combat_01` | `Room_Shop_01` |
|----|------------------|----------------|
| 宿主 | **`APixelRoomDirector`**（RoomDirector），放在原点，全关一只 | **`APixelShopRoomHost`**（ShopRoomHost），放在原点，全关一只。**不要**放战斗导演 |
| `RoomId` | `Room_Combat_01`。写成 `Combat_01` 导演拒绝启动 | `Room_Shop_01`。写成 `Shop_01` 读不到 meta |
| 自动开 | `bAutoStart` 勾上。`FirstWaveDelay` 0.75（夹在 0.5–1.0s）。`GruntEarlyClass` 留空 | `bAutoOpen` 勾上。不跑波次时钟 |
| Play 开关 | `PixelSide.DriveRoomCombat01 1`（或 `bDriveRoomCombat01`） | `PixelSide.DriveRoomShop01 1`（或 `bDriveRoomShop01`） |
| 互斥 | 两者同时开时 **商店让路**，Combat 导演优先。验商店时不要开 Combat 开关 | 同上。验战斗时可以不开商店开关 |
| 右门 | **勾** `bRequireClear`。清房并确认三选一之后才开 | **不勾**。左右门常开，无清房锁 |
| 左门 | 始终可走。清房后踩左门 **不** 重置已清、不重刷 | 始终可走。没有「已清」状态 |
| 三选一 | 两波清空后 `RequestAffixChoice(NormalRoom)`。没选中就关掉，右门仍挡 | **无**三选一，无强制购买 |
| 刷怪 | 导演读 spawns.csv。W1：SP_G1 (420,0)、SP_G2 (560,0)。W2：SP_G3 (720,0)、SP_G4 (900,0)。全是 `Enemy.Grunt.Early`，朝左。同时 ≤4。不要手摆怪 | spawns.csv **只有表头**。关卡里不放刷怪点、不放 Grunt。`max_alive=0` |
| 进房看见什么 | 约 0.75s 后 W1，两只都死后 W2 | `AC_ShopPresenter` → `OpenShop`。余额是真 **RunScrap**，不是 Demo 128。买/刷新成功才 `AddRunScrap`。不要走 `OpenShopDemo` |
| 离开 | 确认三选一后右门打开才能前进 | Esc、`离开` 或 `CloseShop` 之后，不买也能从左右门走 |
| PlayerStart | **(40, 0)** | **(48, 0)** |
| 特有高台 | `P_kite` 中心 X=700，宽 160，TopY=**28**（X 620–780）。单跳可达，非必过 | `P_shelf` X **720–1040**，TopY=**28**。气氛货架，非必过 |
| 特有挡体 | `CV_A` (640,0) 宽 24 高 44；`CV_B` (880,0) 宽 24 高 40。Peek = 穿过后沿 X 站对侧 | `CV_counter` (400,0) 宽 64 高 36。柜台，不是战斗掩体顶 |
| 挡体碰撞 | 两边相同：WorldStatic；Pawn Ignore；Visibility 与 WorldDynamic Block；Camera Ignore | 同左 |
| 特有锚点 | `AffixAnchor` (600, 0)，三选一参考，不必放 Actor | `ShopKeep` (400,0)、`Slot_1/2/3` (720 / 840 / 960, 0)、`LeaveHint` (1100, 0)。不摆武器 uasset |
| 提交说明 | 导演已关灰盒，`RoomId=Room_Combat_01` | 宿主已关灰盒，`RoomId=Room_Shop_01`，关卡里没有战斗导演 |

---

## 勾选时先看哪一列

1. 这张图的宿主类对不对，`RoomId` 是不是带 `Room_` 的全名。
2. `bBuildGreyboxFromCsv` 是不是关的。正式图不靠它铺几何；空关先验才靠它。
3. 只开对应的那一个 Drive 开关。
4. 战斗图勾两波、三选一、右门锁；商店图勾真 RunScrap、可跳过、两门常开、没有导演。

Mac 手拼完成后，各自清单 §5 再把登记表里的「Mac/UE 待拼」改成 umap 已入库。在那之前仓库保持没有假二进制。

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 整理 Combat_01 / Shop_01 建图验收勾选差异。不新开房间，不提交 umap。 |
