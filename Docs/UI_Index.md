# UI 线框索引（MVP · 仓库前待命）

**规范总览：** [UI.md](./UI.md)  
**画布：** 640×360 · Widget 前缀 `WBP_` · 落库 `Content/Blueprints/UI/`（Debug 见下）  
**货币：** Scrap = 仅局内；Core = 仅局外  
**状态：** 文档线 MVP 已齐；蓝图壳等仓库  

---

## 1. 文档与 Widget 一览

| Widget | 线框文档 | 正式路径 | 状态 |
|--------|----------|----------|------|
| `WBP_HUD` | [UI_Wireframes_P0.md](./UI_Wireframes_P0.md) §1 | `Content/Blueprints/UI/` | ✅ C++ `UPixelHUDWidget` + 配方 · 无 uasset。词条格 `UPixelHUDAffixIconWidget` |
| `WBP_AffixChoice` | P0 §2 | 同上 | ✅ C++ `UPixelAffixChoiceWidget` + 配方 · 无 uasset |
| `WBP_AffixCard` | P0 §2.2 | 同上 | ✅ C++ `UPixelAffixCardWidget` + 配方 · 无 uasset |
| `WBP_EventChoice` | [UI_Wireframes_Event.md](./UI_Wireframes_Event.md) | 同上 | ✅ C++ `UPixelEventChoiceWidget` + 配方 · 无 uasset。Lab `F10` |
| `WBP_Shop` | [UI_Wireframes_P1.md](./UI_Wireframes_P1.md) §1 | 同上 | ✅ C++ `UPixelShopWidget` + 配方 · 无 uasset。3 格读 `DA_ShopInventory` / `DA_ShopPriceCurve`。Lab `F11` |
| `WBP_ShopSlot` | P1 §1.2 | 同上 | ✅ C++ `UPixelShopSlotWidget` + 配方 · 无 uasset |
| `WBP_RunSummary` | P1 §2 | 同上 | ✅ 线框 |
| `WBP_Hub` | [UI_Wireframes_Hub.md](./UI_Wireframes_Hub.md) §1 | 同上 | ✅ 线框 |
| `WBP_Hub_Upgrade` | Hub §2 | 同上 | ✅ 线框 |
| `WBP_Hub_NodeRow` | Hub §2.2 | 同上 | ✅ 线框 |
| `WBP_MainMenu` | [UI_Wireframes_MainMenu.md](./UI_Wireframes_MainMenu.md) | 同上 · Entry | ✅ 线框 |
| `WBP_DebugCombatHUD` | [UI_Wireframes_DebugCombatHUD.md](./UI_Wireframes_DebugCombatHUD.md) | `Content/_Dev/Combat/UI/` | ✅ C++ 父类 `UPixelDebugCombatHUD` + 配方 · 不进正式包 · 无 uasset |
| `WBP_RunMap` | — | — | ⏸ 等廊道 |
| `WBP_Pause` | — | — | P2 未做 |

---

## 2. 依赖字段 / API（落地绑定用）

### 2.1 词条 · `UAffixSubsystem`（[AffixSubsystem_API.md](./AffixSubsystem_API.md)）

| 消费方 | 读 / 事件 | 写 |
|--------|-----------|-----|
| `WBP_AffixCard` | `GetAffixCardView` → **Name, Desc, Rarity, Icon**（可选 AffixId / Stacks / bOwned） | — |
| `WBP_AffixChoice` | 听 `OnAffixChoiceOffered`；关 `OnAffixChoiceClosed`。挂 `AC_AffixChoicePresenter`，**`bEnableNativeChoiceUI = false`** | `ConfirmAffixChoice(Index)` |
| `WBP_HUD` 词条条 | 听 `OnAffixChanged`；`GetOwnedAffixes` → **Icon, AffixId**。壳是 `UPixelHUDWidget`，挂 `AC_HUDPresenter` | — |
| `WBP_Shop` 若行上带了 AffixDef | 展示同 `GetAffixCardView` 的 Name / Desc / Rarity / Icon | 锁表三行不是词条卡。购后走流程 |

**禁止：** UI 直接扒 DA 未列字段；UI 不 `RollChoices` / `RequestAffixChoice`。

### 2.2 局内战斗只读（正式 HUD）

| 字段（预留） | 用途 | 现在 |
|--------------|------|------|
| `GetHealthRatio()` | `Bar_Health` | `SetHealthRatio`（Demo 默认 1）。≤0.25 红 + 2Hz，≤0 空条 |
| 当前武器 Icon / 弹药或能量 | `Img_Weapon` / `Txt_Ammo` | `SetWeaponIcon` / `SetAmmo`（Demo 默认无图、`99`） |
| Scrap 余额 | `Txt_Coin` → 文案 **Scrap** | `SetScrap`（Demo 默认 `Scrap 000`） |

正式只读仍由弹芯提供。HUD 不调用这些预留函数，也不写伤害。配方：`Content/Blueprints/UI/README.md`。

### 2.3 商店 · 流程 / Roguelike（P1）

| 字段 | 用途 |
|------|------|
| Scrap 余额 | `Txt_Scrap` |
| Slot: Kind, Name, Price, Rarity?, Icon?, Sold | `WBP_ShopSlot` |
| RerollCost | `Btn_Reroll` |

事件：`OnBuyItem` / `OnReroll` / `OnShopClosed` / `OnRequestAffixChoiceFromVoucher`。

C++ 壳 `UPixelShopWidget` / `UPixelShopSlotWidget`。格数按 `Room_Shop_01` 为 3（`Slot_1..3`）。线框 4 格不落地。

`OpenShopDemo` 读 `DA_ShopInventory.json` 与 `DA_ShopPriceCurve.json`：`Slot_1` 武器合约池展示一把（默认豆射手，价 `Weapon_Peashooter`），`Slot_2` Heal 30 / 约 40% MaxHP，`Slot_3` AffixVoucher 60。Scrap 不足按表 **GreyOut**。可 Leave 跳过。

本壳只改本地 DemoScrap（起始 128，刷新 25 不在价格曲线里）。不发武器、不回血、不写表、不 `RequestAffixChoice`，也不调用 `UEventSubsystem::AddRunScrap`。日后流程扣真钱包：`AddRunScrap(-Price)`。HUD `Txt_Coin` 不在这只壳里改。

券：`OnBuyItem` 之后关店（`OnShopClosed`），再 `OnRequestAffixChoiceFromVoucher`。没有商店暂停 API；`OpenShopDemo` 才 `SetGamePaused`。Lab 键是 `F11`（`F6`–`F10` 不占；`F10` 是事件壳）。

### 2.4 结算 · `FRunSummaryView`（P1）

| 字段 | 用途 |
|------|------|
| `bVictory` | 胜/败文案色 |
| `DurationSec` | 时长 |
| `KillCount` | 击杀 |
| `FloorReached` / `FloorMax` | 层进度 |
| `BossKills` | Boss |
| `ScrapRemain` | 局内结余展示 |
| `CoreGained` | **Core** 获得（强调） |
| `AffixIds` | 构筑图标条 |

事件：`OnContinueToHub`。

### 2.5 据点 · `UMetaProgressSubsystem`（[MetaSaveInterface.md](./MetaSaveInterface.md)）

| API / 字段 | 用途 |
|------------|------|
| `GetCore` | Hub / Upgrade 顶栏 |
| `CanPurchase` / `Purchase` | 节点升级 |
| 节点：DisplayName, Description, CostCore, OwnedRank, MaxPurchases | `WBP_Hub_NodeRow` |

**注意：** 节点效果里的「开局 Scrap」是局内注入，费用货币仍是 **Core**。

### 2.6 主菜单

| 事件 | 流向 |
|------|------|
| `OnStartGame` | → Hub（推荐） |
| `OnOpenSettings` | 占位面板 |
| `OnQuitGame` | Quit |

无货币字段。

### 2.7 Debug · `BI_CombatDebugRead`（[Combat_P0_DebugHUD.md](./Combat_P0_DebugHUD.md)）

| 字段 | 控件 |
|------|------|
| Health, MaxHealth | HP 行 / 条 |
| IFrameRemaining | IFR |
| WeaponName | WPN |
| FireCooldownRemaining, ResolvedFireCooldown | CD（剩余 / `GetResolvedFireCooldown()`） |
| FireRate（`GetFireRate` = `GetStat(FireRate)`） | `FR`；无词条 1.000，RapidFire ≈ 1.120 |
| LastDamageTaken, LastDamageTime | DMG |

实现：`UCombatDebugRead`（显示名 `BI_CombatDebugRead`）由 `UCombatDebugReadComponent` 转到 `UDamageableComponent` / `UWeaponControllerComponent`，挂在 `APixelSideCharacter`（`GetDamageable` / `GetWeaponController`）。HUD 父类 `UPixelDebugCombatHUD`，0.05s 只读刷新。CD 分母用 `GetResolvedFireCooldown`（`FireInterval / GetStat(FireRate)`，缺省 `1/FireRateBase`）。配方：`Content/_Dev/Combat/UI/README.md`。不调用 `ApplyDamage` / `Fire`。正式 `WBP_HUD` 不引用。

### 2.8 事件 · `UPixelEventChoiceWidget`（[UI_Wireframes_Event.md](./UI_Wireframes_Event.md)）

| 消费方 | 读 | 写 |
|--------|----|----|
| `WBP_EventChoice` | Demo 视图：`DisplayName`、`PromptBody`、`Choices[].Label`、`SkipHint`。Lab 文案对齐 `DA_Event_AbandonedCache.json`，不建 DA uasset | `OnEventChoiceSelected(ChoiceId)`、`OnEventSkipped()`、`OnEventClosed()` |

**打开：** 忽略移动 / 跳跃 / 开火（`SetEventChoiceBlocksControl`）。**不** `SetGamePaused`。`bAllowDoorSkip` 保持 true，门体积照走（GDD §2.8「门仍可走」）。  
**确认：** 选项调用一次 `ResolveEventChoice`。壳不自己 `GrantScrap` / `DamageSelf` / `HealSelf`，不 `SetGamePaused`。Skip / Esc 不调用 Resolve。不因结算打开 `WBP_AffixChoice`（`bAffixOnResolve=false`）。

---

## 3. 地图挂载

| Map | Widget |
|-----|--------|
| `Entry` | `WBP_MainMenu` |
| `Hub` | `WBP_Hub`（+ Upgrade 叠层） |
| Run / `Run_Test` | `WBP_HUD`（玩家 `AC_HUDPresenter.EnsureMounted`）；弹窗 AffixChoice / Shop / EventChoice；结束 RunSummary |
| `Lab_CombatP0` / `_Dev` / Entry | 同上正式 `WBP_HUD`；另有 `WBP_DebugCombatHUD`（Z 50，y=40）。`F6` 刷 HUD 词条格。`F7` 弹 `UPixelAffixChoiceWidget`（native 文本占位关）。`F8` / `F9` 直调 `ResolveEventChoice`。`F10` 弹 `UPixelEventChoiceWidget`（AbandonedCache，不暂停世界；确认选项才 Resolve，Skip 不 Resolve）。`F11` 弹 `UPixelShopWidget`（DemoScrap 128，不写 RunScrap） |

---

## 4. 协作依赖

| 角色 | UI 等待 |
|------|---------|
| 词条官 | Affix 契约 ✅；Meta CostCore ✅ |
| 弹芯 | 正式 HUD 壳已接 Demo 设置器；`GetHealthRatio` / 武器 Icon / Scrap 余额仍待。Debug 读面已由 `AC_CombatDebugRead` 转发，正式 HUD 不读它 |
| 点阵 | 切片尺寸见各线框；色板替换临时色 |
| 织局 / 廊道 | RunMap 需求；商店价表 |
| 主管 | 仓库 → 蓝图壳合入 |

---

## 5. 仓库就绪后落地顺序（建议）

1. `WBP_MainMenu` → `WBP_Hub` / Upgrade  
2. `WBP_HUD` + `WBP_AffixChoice` / Card（C++ 父类已在，无 uasset）  
3. `WBP_Shop` / Slot（C++ 壳已读 #35 表）→ `WBP_RunSummary`  
4. `WBP_DebugCombatHUD`（可并行 _Dev）  

完成后本索引保持为单页入口；细节仍以各 Wireframes 文件为准。
