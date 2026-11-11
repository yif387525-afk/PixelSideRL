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
| `WBP_Shop` | [UI_Wireframes_P1.md](./UI_Wireframes_P1.md) §1 | 同上 | ✅ C++ `UPixelShopWidget` + 配方 · 无 uasset。3 格读 `DA_ShopInventory` / `DA_ShopPriceCurve`。Lab `F11` 仍直接开 Demo（128→98）。`OpenShop` 扣 RunScrap。流程开/关：`AC_ShopPresenter`（不改） |
| `WBP_ShopSlot` | P1 §1.2 | 同上 | ✅ C++ `UPixelShopSlotWidget` + 配方 · 无 uasset |
| `WBP_RunSummary` | P1 §2 | 同上 | ✅ C++ `UPixelRunSummaryWidget` + 配方 · 无 uasset。只展示 `FRunSummaryView`。不写 Core。无 Lab 键 |
| `WBP_Hub` | [UI_Wireframes_Hub.md](./UI_Wireframes_Hub.md) §1 | 同上 | ✅ C++ `UPixelHubWidget` + 配方 · 无 uasset。无新 Lab 键 |
| `WBP_Hub_Upgrade` | Hub §2 | 同上 | ✅ C++ `UPixelHubUpgradeWidget` + 配方 · 无 uasset |
| `WBP_Hub_NodeRow` | Hub §2.2 | 同上 | ✅ C++ `UPixelHubNodeRowWidget` + 配方 · 无 uasset |
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

两条钱包。

**Demo：** `OpenShopDemo` / Lab `F11`。本地 DemoScrap 起始 128，刷新 25 不在价格曲线里。买回复 → Sold / 98 → Leave。不调用 `GetRunScrap` / `AddRunScrap`。

**Run：** `OpenShop`。标签是 `GetRunScrap()`。买成功 `AddRunScrap(-Price)`，刷新成功 `AddRunScrap(-RerollCost)`，然后再 `GetRunScrap()`。不够 GreyOut，不扣。`RunScrap` 是整数，扣款对价格做 `llround`，不改价格曲线文件。

两条都不发武器、不回血、不写表、不 `RequestAffixChoice`。HUD `Txt_Coin` 不在这只壳里改。

券：`OnBuyItem` 之后关店（`OnShopClosed`），再 `OnRequestAffixChoiceFromVoucher`。没有商店暂停 API；`OpenShopDemo` 与 `OpenShop(..., bDemoPause=true)` 才 `SetGamePaused`。Lab 键是 `F11`（`F6`–`F10` 不占；`F10` 是事件壳）。

**房间流程开/关：** 仍是玩家 `GetShopPresenter()`（`AC_ShopPresenter`）的 `OpenShopDemo(Layer)` / `OpenShop(Offers, Scrap, bDemoPause)` / `CloseShop()`。不新绑键，不改 Presenter 开/关。`OpenShopDemo` 是 Demo。`OpenShop` 进壳后是 Run 钱包；`Scrap` 参数留在签名里，壳不拿它当余额。Presenter 自己不调用 `AddRunScrap`。`F11` 仍直接开 Lab Demo，不经过 Presenter。无 uasset。见 `Content/Maps/Rooms/Room_Shop_01.md` §7 与 `Content/Blueprints/UI/README.md`。

### 2.4 结算 · `FRunSummaryView`（P1）

| 字段 | 用途 |
|------|------|
| `bVictory` | 胜/败文案色 |
| `DurationSec` | 时长 |
| `KillCount` | 击杀 |
| `FloorReached` / `FloorMax` | 层进度 |
| `BossKills` | Boss |
| `ScrapRemain` | 局内结余展示（文案仍是 **Scrap**） |
| `CoreGained` | **Core** 获得（强调）。只显示传入的数 |
| `AffixIds` | 构筑图标条。没有短名时直接显示 Id |
| `AffixShortNames` | 可选，与 `AffixIds` 平行。Demo 用线框短名。壳不调用 `GetAffixCardView` |

事件：`OnContinueToHub`。确认是按钮、Enter、Space、Esc。Esc 与确认相同，避免卡在结算。UI 不切关卡。

C++ 壳 `UPixelRunSummaryWidget`。`Present(View, bPause=true)` / `OpenWin` / `OpenFail`。Win / Fail 数值是线框 §2.2，不是 Core 公式。不调用 `AddCore`、`PreviewCoreGain`、`Save` / `Load`，不写存档，不改 `RunScrap`。

没有结算暂停 API。打开默认 `SetGamePaused`，与商店 Demo 相同。已经暂停的世界不接管，确认时也不 `SetGamePaused(false)`。三选一还开着时拒绝打开。视口 Z = 90（HUD 10，调试条 50，商店 80，三选一 100）。不改 `WBP_HUD`。

无 Lab 键。非 Shipping 控制台：`PixelSide.RunSummary.Demo Win` 或 `Fail`。**不占、不改 F6–F11，不加 F12。** F10 仍是事件，F11 仍是商店。

### 2.5 据点 · `UMetaProgressSubsystem`（[MetaSaveInterface.md](./MetaSaveInterface.md)）

| API / 字段 | 用途 |
|------------|------|
| `GetCore` | Hub / Upgrade 顶栏。文案 `Core N`，色 `#7EC8FF` |
| `GetNodeViews` | 只读行：DisplayName、Description、NextCostCore、OwnedRank、MaxPurchases、bMaxed、bLocked、bCanPurchase |
| `CanPurchase` / `Purchase` | 节点升级。不调用 `Save` / `AddCore` |

C++ 壳 `UPixelHubWidget` / `UPixelHubUpgradeWidget` / `UPixelHubNodeRowWidget`。事件 `OnStartRun` / `OnOpenUpgrades` / `OnBackToMenu`。`OnStartRun` 不调用 `BeginRunFromMeta`。Hub 地图上由 `UPixelHubRunFlowSubsystem` 在短锁之后调用一次 `BeginRunFromMeta`，再旅行到 `Room_Combat_01`。直接 Play 该战斗房不打开 Hub。确认结算后回 Hub 并 `OpenHub`。`GameDefaultMap` 仍是 `/Engine/Maps/Entry`。步骤见 [UI_Wireframes_Hub.md](./UI_Wireframes_Hub.md) §5。

两条钱包。

**Demo：** `OpenHubDemo` 或没有 `UMetaProgressSubsystem` 的 `OpenHub`。本地 **DemoCore 120**，节点是线框 §2.3（`meta_vitality` 等）。不调用 `GetCore` / `Purchase` / `AddCore` / `Save`，不写存档。Hub 顶栏与升级页是同一个本地数。

**Meta：** `OpenHub` 且子系统在。顶栏 `GetCore()`。升级读 `GetNodeViews()`，点升级 `Purchase(NodeId)`，然后再 `GetCore()`。满级显示「已满级」并收起升级钮。Core 不够时费用红字、钮 Disabled。前置未满足时 `Border_Locked`。

**注意：** 节点效果里的「开局 Scrap」是局内注入，费用货币仍是 **Core**。顶栏禁止把局外钱包写成 Scrap。

无 Lab 键。非 Shipping 控制台：`PixelSide.Hub.Demo`（本地 120）或 `PixelSide.Hub.Open`（走 Meta）。**不占、不改 F6–F11，不加 F12。** 视口 Z = 85（HUD 10，调试条 50，商店 80，结算 90，三选一 100）。`上次结算` 默认折叠；`SetLastRunSummary` 之后才显示，点开只 `Present` 已有的 `WBP_RunSummary`，不写 Core。Esc 只在升级页回到 Hub。

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
| `Hub` | `WBP_Hub`（+ Upgrade 叠层）。C++ 壳，无 uasset。控制台 `PixelSide.Hub.Demo` / `PixelSide.Hub.Open` |
| Run / `Run_Test` | `WBP_HUD`（玩家 `AC_HUDPresenter.EnsureMounted`）；弹窗 AffixChoice / Shop / EventChoice；结束时由流程 `Present` `WBP_RunSummary` |
| `Lab_CombatP0` / `_Dev` / Entry | 同上正式 `WBP_HUD`；另有 `WBP_DebugCombatHUD`（Z 50，y=40）。`F6` 刷 HUD 词条格。`F7` 弹 `UPixelAffixChoiceWidget`（native 文本占位关）。`F8` / `F9` 直调 `ResolveEventChoice`。`F10` 弹 `UPixelEventChoiceWidget`（AbandonedCache，不暂停世界；确认选项才 Resolve，Skip 不 Resolve）。`F11` 仍直接弹 `UPixelShopWidget` Demo（DemoScrap 128，不写 RunScrap），不经过 Presenter。房间流程另调玩家 `AC_ShopPresenter` 开/关（Z 80）。结算没有键：`PixelSide.RunSummary.Demo Win` 或 `Fail`。据点没有键：`PixelSide.Hub.Demo` 或 `PixelSide.Hub.Open`。不加 F12 |

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

1. `WBP_MainMenu` → `WBP_Hub` / Upgrade（C++ 壳已落地，无 uasset）  
2. `WBP_HUD` + `WBP_AffixChoice` / Card（C++ 父类已在，无 uasset）  
3. `WBP_Shop` / Slot（C++ 壳已读 #35 表）→ `WBP_RunSummary`（C++ 壳已落地，无 uasset）→ `WBP_Hub`（C++ 壳已落地，无 uasset）  
4. `WBP_DebugCombatHUD`（可并行 _Dev）  

完成后本索引保持为单页入口；细节仍以各 Wireframes 文件为准。
