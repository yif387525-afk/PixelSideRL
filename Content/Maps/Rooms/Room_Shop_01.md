# Room_Shop_01 — 正式商店房模板规格

| 字段 | 内容 |
|------|------|
| Type / Tag | Shop / Branch |
| Owner | 廊道 |
| 状态 | **规格定稿候选** · Presenter/#47 已接 `OpenShop` · **尚无 umap** · 建图清单 `Docs/Room_Shop_01_UMapBuild.md` |
| 尺寸 | 设计宽 **1280**（2 屏）· **安全区** |
| 意图 | 支路商店；购买或跳过离开 |
| 入口安全区 | 平层 X≈0–240；无威胁 |
| Spawn | **无**（spawns.csv 仅表头） |
| 同时存活上限 | 0 |
| Exit | Left / Right · **不锁** · **无** `RequireClear` |
| 清房 | 玩家离开即可；**无三选一**；无强制购买 |
| 数据 | `Content/Data/Rooms/Room_Shop_01_*.csv`；货架 `Content/Data/Shop/DA_ShopInventory.json` 与 `DA_ShopPriceCurve.json` |
| 上场 | **仅支路**；与 Event 二选一上场（最多 1 支路） |
| 禁止 | 战斗、强制购买、清房门锁、fake uasset、改 Lab |

**坐标：** 原点 = 左侧入口内侧地面；`+X` 右；`+Y` 上；1 px = 1 uu。  
**挂点默认：** Elite 后、BossAnte 前（同 Event）。

---

## 1. ASCII 侧视（约 1 字符 ≈ 40 px）

```
Y
28|           [=货架台=]
 0|安====[柜台]====货1 货2 货3====离开=
   +--------------------------------→ X
     0    400   640  800  960   1280
```

地面连续无坑；柜台/商品点位清晰；离开门常开。

---

## 2. 几何尺寸表

### 2.1 地面 / 边界

| ID | 类型 | X0 | X1 | TopY | 备注 |
|----|------|----|----|------|------|
| G_main | 地板 | 0 | 1280 | 0 | 安全主路；**无坑** |
| WALL_L | 边界墙 | -40 | 0 | — | 高 280 |
| WALL_R | 边界墙 | 1280 | 1320 | — | 高 280 |

### 2.2 平台（货架气氛；TopY=28）

| ID | X0 | X1 | TopY | 角色 |
|----|----|----|------|------|
| P_shelf | 720 | 1040 | **28** | 货架气氛台；商品可读站位 |

### 2.3 短掩体 / 柜台体块

| ID | 底心 (X,Y) | 高 | 宽 | 作用 |
|----|------------|----|----|------|
| CV_counter | (400, 0) | 36 | 64 | **柜台**；挡弹可选；Pawn 可绕 |

---

## 3. 出口

| ID | Tag | (X,Y) | 规则 |
|----|-----|-------|------|
| Exit_Left | Exit.Left | (-40, 0) | 回主干；**不锁** |
| Exit_Right | Exit.Right | (1200, 0) | 回主干；**不锁**；无 RequireClear |

---

## 4. 刷怪 / 商店锚点

**刷怪：无。** 见 [`Room_Shop_01_Spawns.md`](./Room_Shop_01_Spawns.md)。

| ID | Kind | (X,Y) | 备注 |
|----|------|-------|------|
| ShopKeep | NPCAnchor | (400, 0) | 店主/柜台交互（可选形象） |
| Slot_1 | ShopSlot | (720, 0) | 商品位 1 · DA 驱动 |
| Slot_2 | ShopSlot | (840, 0) | 商品位 2 |
| Slot_3 | ShopSlot | (960, 0) | 商品位 3 |
| LeaveHint | UIHint | (1100, 0) | 离开/跳过提示 |

MVP：可购买任意槽或 **直接走门跳过**；门常开。

---

## 5. 布局锚点

| 锚点 | (X,Y) | 说明 |
|------|-------|------|
| PlayerStart | (48, 0) | 入口安全区 |

---

## 6. 数据表

| 文件 | 用途 |
|------|------|
| `Content/Data/Rooms/Room_Shop_01_meta.csv` | 元数据 |
| `Content/Data/Rooms/Room_Shop_01_spawns.csv` | **仅表头** |
| `Content/Data/Rooms/Room_Shop_01_geometry.csv` | 几何/出口/商店锚点 |

`clear_rule=None`；`affix_on_clear=false`；`require_clear_exit` 空；`max_alive=0`。  
商品池/价格曲线：`Content/Data/Shop/DA_ShopInventory.json`、`DA_ShopPriceCurve.json`（冻名；Slot_1 武器合约池 / Slot_2 回复 / Slot_3 三选一券；无 uasset）。本文件只定几何与锚点。

---

## 7. 流程开店

玩家 `APixelSideCharacter` 上已有 `AC_ShopPresenter`（`UPixelShopPresenterComponent`）。进房由 `APixelShopRoomHost` 调用（`PixelSide.DriveRoomShop01 1`，或 GameMode `bDriveRoomShop01`）。它不走 `APixelRoomDirector`，不新绑键，不建 umap，不提交 uasset。Combat_01 导演开关优先：两者同时开时商店让路。

```cpp
UPixelShopPresenterComponent* Presenter = Player->GetShopPresenter();
TArray<FPixelShopSlotOffer> Offers;
TArray<FPixelShopSlotOffer> Pool;
UPixelShopWidget::CollectShelfOffers(/*Layer=*/1, Offers, Pool); // 只读 DA_Shop*，不开店
Presenter->OpenShop(Offers, /*Scrap=*/0.0, /*bDemoPause=*/true); // 壳忽略 Scrap，读 GetRunScrap
Presenter->CloseShop(); // 与 Leave / Esc 相同
```

`OpenShop` 是进房主路径。传入的 `Scrap` 不是余额。买成功 `AddRunScrap(-Price)`，刷新成功 `AddRunScrap(-RerollCost)`，然后再 `GetRunScrap`。Presenter 自己不调用 `AddRunScrap`。`OpenShopDemo` 不是本房验收路径。`F11` 仍直接开 Lab Demo（128→买回复→98），不经过本组件。`F6`–`F10` 不动。

门仍不锁。可以不买，直接走门；关店用 `CloseShop`、Esc 或 `离开`。正式图按 `Docs/Room_Shop_01_UMapBuild.md` 在 Mac / UE 5.8.4 里手摆，宿主关掉 `bBuildGreyboxFromCsv`。**仓库里仍然没有 umap**，也不放假 uasset。不要放 `APixelRoomDirector`。

---

## 8. 入库验收

- [ ] 无刷怪 / 无强制购买 / 门不锁  
- [ ] 柜台与 3 商品位可读；可跳过离开  
- [ ] 无三选一；CSV 齐；无 Lab 改动  
- [ ] Branch-only；与 Event 二选一支路上场  
- [x] 流程经 `AC_ShopPresenter` 开/关。不改 F11，Presenter 自己不扣，无 uasset  
- [x] 壳上 `OpenShop` 买/刷成功扣 `RunScrap`。`OpenShopDemo` / F11 仍是 DemoScrap 128→98

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-04 | 细表占位 |
| 2026-11-05 | 正式规格：柜台/货位/出口/CSV；COMMENT 交审；商品 DA 归词条官 |
| 2026-11-05 | 货架改指 `Content/Data/Shop/DA_Shop*`；几何未改 |
| 2026-11-05 | 流程开店：`AC_ShopPresenter` 调 `OpenShop` / `OpenShopDemo` / `CloseShop`。不改扣款，不改 F11 |
| 2026-11-05 | 壳上两路钱包。`OpenShop` 读 `GetRunScrap` 并在买/刷成功 `AddRunScrap`。Presenter 开/关不改。F11 仍是 Demo |
| 2026-11-05 | 进房 `APixelShopRoomHost` 调 `OpenShop`（真 RunScrap）。不走 OpenShopDemo，不改 F11 |
| 2026-11-05 | 正式 umap 建图清单 `Docs/Room_Shop_01_UMapBuild.md`。真图待 Mac；本仓库仍无 umap |
