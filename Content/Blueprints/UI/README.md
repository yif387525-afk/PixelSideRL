# Content/Blueprints/UI

Widget 落库目录。命名 `WBP_`。P0：`WBP_HUD`、`WBP_HUD_AffixIcon`、`WBP_AffixChoice`、`WBP_AffixCard`、`WBP_EventChoice`。P1：`WBP_Shop`、`WBP_ShopSlot`、`WBP_RunSummary`。线框见 Docs/UI_Wireframes_P0.md、Docs/UI_Wireframes_Event.md 与 Docs/UI_Wireframes_P1.md。

`WBP_DebugCombatHUD` 不放这里。它只属于 `Content/_Dev/Combat/UI/`。正式 HUD 不要引用它。

**不要提交 `WBP_*.uasset`。** 没有蓝图时，C++ 父类自己建出同名控件。

## 命名

| 资产名（编辑器里再建） | C++ 父类 | 类选择器显示名 | 头文件 |
|------------------------|----------|----------------|--------|
| `WBP_HUD` | `UPixelHUDWidget` | `WBP_HUD` | `Source/PixelSideRL/Public/UI/PixelHUDWidget.h` |
| `WBP_HUD_AffixIcon` | `UPixelHUDAffixIconWidget` | `WBP_HUD_AffixIcon` | `Source/PixelSideRL/Public/UI/PixelHUDAffixIconWidget.h` |
| `WBP_AffixChoice` | `UPixelAffixChoiceWidget` | `WBP_AffixChoice` | `Source/PixelSideRL/Public/UI/PixelAffixChoiceWidget.h` |
| `WBP_AffixCard` | `UPixelAffixCardWidget` | `WBP_AffixCard` | `Source/PixelSideRL/Public/UI/PixelAffixCardWidget.h` |
| `WBP_EventChoice` | `UPixelEventChoiceWidget` | `WBP_EventChoice` | `Source/PixelSideRL/Public/UI/PixelEventChoiceWidget.h` |
| `WBP_EventChoiceButton` | `UPixelEventChoiceButton` | `WBP_EventChoiceButton` | `Source/PixelSideRL/Public/UI/PixelEventChoiceButton.h` |
| `WBP_Shop` | `UPixelShopWidget` | `WBP_Shop` | `Source/PixelSideRL/Public/UI/PixelShopWidget.h` |
| `WBP_ShopSlot` | `UPixelShopSlotWidget` | `WBP_ShopSlot` | `Source/PixelSideRL/Public/UI/PixelShopSlotWidget.h` |
| `WBP_RunSummary` | `UPixelRunSummaryWidget` | `WBP_RunSummary` | `Source/PixelSideRL/Public/UI/PixelRunSummaryWidget.h` |

路径：`Content/Blueprints/UI/`。画布逻辑 640×360，安全区 8px。三张卡实例名必须是 `Card_0`、`Card_1`、`Card_2`。

### `WBP_HUD`

顶栏常驻。不暂停，不改输入。整棵控件 `HitTestInvisible`（根是 `SelfHitTestInvisible`），点击穿过到世界。没有 `Btn_DemoHurt` / `Btn_DemoHeal` / `Btn_DemoAddAffix`。

| 控件名 | 类型 | 数据 |
|--------|------|------|
| `Bar_Health` | Progress Bar，96×8 | `SetHealthRatio` 0–1。默认 1，填充 `#2EE6A6`，槽 `#3A3A3A`。≤0.25 变 `#FF3B3B` 并按 2Hz 闪填充。≤0 空条 |
| `Img_Weapon` | Image 16×16 | `SetWeaponIcon`。空则浅色块，不造 `T_UI_WeaponPlaceholder` |
| `Txt_Ammo` | Text Block | `SetAmmo`。默认 `99` |
| `Txt_Coin` | Text Block | `SetScrap`。文案 **`Scrap 000`**（控件名仍是线框的 `Txt_Coin`，不要写成「币」）。颜色 `#FFD54A` |
| `HB_AffixIcons` | Horizontal Box | 最多 8 个 `WBP_HUD_AffixIcon`，12×12，间距 2，右对齐 |

词条条：构造时刷一次，之后听 `OnAffixChanged`，再 `GetOwnedAffixes()`。只用 **Icon + AffixId**。空槽 `#4A4A4A`。有词条但 Icon 为空时用 `#F2F2F2` 实心块（表里 Icon 目前是 null，`F6` 会看到格子变亮）。超过 8 个只画前 8 个。悬停提示只把 `AffixId` 留在槽上，当前不挡点击，所以不会弹出。

战斗数还没有正式只读接口。在弹芯给出之前，只走上面的设置器，HUD 自己用这些 Demo 值刷新。预留、**现在不要在 HUD 里调用**：

- `GetHealthRatio()`
- 当前武器 Icon
- Scrap 余额

不要 `ApplyDamage`、`Fire`、`RollChoices`、`RequestAffixChoice`。不要引用 `WBP_DebugCombatHUD`。

### `WBP_HUD_AffixIcon`（12×12）

| 控件名 | 类型 |
|--------|------|
| `Img_Icon` | Image，铺满 12×12 |

`SetOccupied(AffixId, Icon)` / `SetEmpty()`。无贴图不加载占位资产。

### `WBP_AffixChoice`

| 控件名 | 类型 |
|--------|------|
| `Overlay_Root` | Overlay，全屏 |
| `Img_Dim` | Image，`#000000` 60% |
| `Txt_Title` | Text Block，`选择一项强化` |
| `Txt_Hint` | Text Block，`1 / 2 / 3 选择 · Enter 确认` |
| `HB_Cards` | Horizontal Box，间距 16 |
| `Card_0` `Card_1` `Card_2` | `WBP_AffixCard` |

不要做 `Btn_Skip`。Esc 不关闭。

### `WBP_AffixCard`（约 120×168）

| 控件名 | 类型 | 数据 |
|--------|------|------|
| `Border_Rarity` | Border | Common `#B0B0B0` / Rare `#3D7EFF` / Epic `#B44DFF` / Legendary `#FFC107` |
| `Btn_Select` | Button | 点击 = 确认该下标 |
| `Txt_Hotkey` | Text Block | `1` / `2` / `3`，UI 本地，不是 DA |
| `Img_Icon` | Image 32×32 | `FAffixCardView.Icon`；空则灰块，不造占位贴图 |
| `Txt_Name` | Text Block | `Name` |
| `Txt_Desc` | Text Block | `Desc`，最多约 3 行 |
| `Txt_Rarity` | Text Block | `Rarity` |
| `Txt_Stacks` | Text Block，可选 | `bOwned` 且 `Stacks > 0` 时 `xN` |

只读这四项加可选层数。不要读 `UAffixDefinition` 上的 Modifier、Weight、Tags。

### `WBP_Shop`

`Room_Shop_01` 是 **3** 格横排（`Slot_1` `Slot_2` `Slot_3`，容器 `HB_Slots`）。线框 ASCII 的 2×2 / 4 格不落地；第四格不建控件。

货位和标价读 `Content/Data/Shop/DA_ShopInventory.json` + `DA_ShopPriceCurve.json`（JSON 失败才读同名 CSV）。不提交 uasset，不 `LoadObject` 武器 DA。层 1 用 `baseScrap`。`resolvedScrap = baseScrap * layerMultiplier`，不另起整数。

| 控件名 | 类型 |
|--------|------|
| `Overlay_Root` | Overlay，全屏 |
| `Img_Dim` | Image，`#000000` 60% |
| `Txt_Title` | Text Block，`商店` |
| `Txt_Scrap` | Text Block，`Scrap N`，`#FFD54A` |
| `HB_Slots` | Horizontal Box，间距 12。线框名 `UG_Slots` 是 4 格演示 |
| `Slot_1` `Slot_2` `Slot_3` | `WBP_ShopSlot`，128×100 |
| `Btn_Reroll` | Button，文案 `刷新 25` |
| `Btn_Leave` | Button，文案 `离开` |
| `Txt_Hint` | Text Block，`← → 选格 · Enter 买 · R 刷新 · Esc 离开` |

`Txt_Reroll` / `Txt_Leave` 只是按钮里的文案，可选绑定。

默认货架（层 1，表里的字段，不是线框那 4 个假名）：

| 控件 | 表 | 展示 | 价 |
|------|----|------|----|
| `Slot_1` | `WeaponContract`，`PickOneFromPool` | 池中第一把（豆射手 / `WeaponContract_Peashooter`） | `Weapon_Peashooter` → 45 |
| `Slot_2` | `Heal` / `HealMaxHp` | 回复，`Heal · 40% MaxHP` | `Heal` → 30 |
| `Slot_3` | `AffixVoucher` / `AffixChoiceVoucher` | 三选一券 | `AffixVoucher` → 60 |

Scrap 不够：格子灰显（表字段 `insufficientScrap = GreyOut`），不扣款。点了会抖 2px。Esc / `Btn_Leave` 直接关，不强制买。

购买、刷新、离开只改本控件上的 **DemoScrap**（开店 128）和 Sold。不发武器、不回血、不写 JSON、不调用 `RequestAffixChoice`，也不调用 `UEventSubsystem::AddRunScrap`。日后房间流程扣真钱包用 `AddRunScrap(-Price)`。`RunScrap` 与 HUD 的 `Txt_Coin` 都不要在这只壳里改。

刷新价 **25** 来自线框，不在价格曲线里。Demo 刷新只把未售出的武器格换成池中下一把，价格仍查曲线。

券：`OnBuyItem` → 关店（`OnShopClosed`，并解开这次 Demo 暂停）→ `OnRequestAffixChoiceFromVoucher`。监听方再开三选一。

没有商店暂停 API。`OpenShopDemo` / `OpenShop(..., bDemoPause=true)` 才 `SetGamePaused`。流程自己暂停时传 false。视口 Z = 80（HUD 10，三选一 100）。壳自己不是 `AC_AffixChoicePresenter`。

### 从流程开店（`AC_ShopPresenter`）

`APixelSideCharacter` 构造时创建 `AC_ShopPresenter`（`UPixelShopPresenterComponent`）。它不在 `BeginPlay` 自动打开，也不新绑键。`F6`–`F11` 不动。`F11` 仍直接对 Lab 那只调试控件调 `OpenShopDemo`，不经过本组件。

`Room_Shop_01` 由流程对玩家调用：

```cpp
UPixelShopPresenterComponent* Presenter = Player->GetShopPresenter();
Presenter->OpenShopDemo(/*Layer=*/1);
// 或 Presenter->OpenShop(Offers, Scrap, /*bDemoPause=*/true);
Presenter->CloseShop();
```

`OpenShop` / `OpenShopDemo` / `CloseShop` 转发到 `UPixelShopWidget` 的同名函数。控件自己 `AddToViewport(80)`，Presenter 不再挂一次、也不另选 Z。传入的 `Scrap` 只种进壳内 DemoScrap，不读、不写 `RunScrap`，不调用 `AddRunScrap`。买和刷新仍是壳里的 `TryBuy` / `TryReroll`，本组件不改这两处。

关掉：流程 `CloseShop()`，或玩家 Esc / `离开`。`OnBuyItem` 等仍绑在返回的控件上。本组件不订阅、不扣款。

`ShopWidgetClass` 留空则用 `UPixelShopWidget`。不要提交 uasset，不要为这个入口新建 umap。

### `WBP_ShopSlot`（128×100）

| 控件名 | 类型 |
|--------|------|
| `Border_Frame` | Border。焦点 1px 白边；买不起或 Sold 时灰边 |
| `Btn_Buy` | Button，铺满 |
| `Img_Icon` | Image 24×24。表里没有图标就用灰块，不造贴图 |
| `Txt_Name` | 武器用池内 `displayName`；回复 / 券用槽 `displayName` |
| `Txt_Kind` | `WeaponContract` / `Heal · N% MaxHP` / `AffixVoucher` |
| `Txt_Price` | `45 Scrap` 这种。买不起变灰，不变红 |
| `Img_SoldOut` | Sold 时的斜杠 |

若调用方在行上放了 `AffixDef`，只读 `GetAffixCardView` 的 Name / Desc / Rarity / Icon。锁表三行没有 Def。Desc 放在购买按钮的 tooltip 上，因为槽位没有 `Txt_Desc`。

## `WBP_RunSummary`

全屏胜负结算。逻辑画布 640×360，安全区 8px。线框：`Docs/UI_Wireframes_P1.md` §2。

只展示 `FRunSummaryView`。`CoreGained` 原样写成 `获得 Core +N`。`ScrapRemain` 写成 `Scrap 结余 N`（局内结余，不是 Core）。不调用 `AddCore`、`PreviewCoreGain`、`Save`、`Load`，不写存档，不改 `RunScrap`，不读战斗只读接口，不改 `WBP_HUD`。构筑短名用视图里的 `AffixShortNames`；空了才显示 `AffixIds`。不调用 `GetAffixCardView`。Demo 的 Id（`Demo_PeaVolley` 等）不是表里的词条，不会授予。

没有单独的芯片 WBP。`WB_Build` 里的图标块和短名是运行时填的，无贴图（12×12 浅色块）。空列表显示 `（无词条）`。超过 8 个时后面加 `…`，悬停保留全名。第一次填入时清掉 `WB_Build` 里原有的子控件，之后复用这 8 个芯片。不要在图表里手工摆芯片，不要提交 uasset。

| 控件名 | 类型 | 数据 |
|--------|------|------|
| `Overlay_Root` | Overlay，全屏 | |
| `Img_Dim` | Image，`#000000` 94% | 挡住画面，点击不穿透。不 Hide HUD |
| `Txt_Outcome` | Text Block，字号 22 | `胜利` `#2EE6A6` / `失败` `#FF3B3B` |
| `Txt_Time` | Text Block | `16:20` 这种 `MM:SS` |
| `Txt_Kills` | Text Block | `击杀 86` |
| `WB_Build` | Wrap Box | 图标 + 短名，最多两行的量；空则 `（无词条）` |
| `Txt_Floor` | Text Block | `层进度 2/2` |
| `Txt_BossKills` | Text Block | `Boss 2` |
| `Txt_ScrapLeft` | Text Block，`#FFD54A` | `Scrap 结余 40` |
| `Txt_CoreTip` | Text Block，小字 | 仅失败：`局内 Scrap / 词条 / 临时枪已清空` |
| `Txt_CoreGain` | Text Block，字号 28，白色 | `获得 Core +42`。视觉上大于标题 |
| `Btn_Continue` | Button | `回据点` |
| `Txt_Continue` | Text Block，放在按钮里 | 同上 |
| `Txt_Hint` | Text Block | `Enter / Space / 点击回据点 · Esc 同确认` |

`Btn_DemoWin` / `Btn_DemoFail` 只在非 Shipping 的回退壳上，文案 `样例·胜` / `样例·败`。Shipping 里若蓝图仍绑了这两个按钮，会被折叠。

| 调用 | 结果 |
|------|------|
| `OpenWin()` | §2.2 胜：16:20，击杀 86，层 2/2，Boss 2，Scrap 结余 40，Core +42，三词条。无失败小字 |
| `OpenFail()` | §2.2 败：09:05，击杀 31，层 1/2，Boss 0，Scrap 结余 12，Core +8，只有豌豆连射。显示清空小字 |
| `Present(View)` | 用调用方给的数。`bPause` 默认 true |
| 确认 | `OnContinueToHub` 一次。不打开 Hub 关卡 |

Enter、Space、点击 `回据点`、Esc 都是确认。Esc 与确认相同，避免卡死。

没有结算暂停 API。`Present` / `OpenWin` / `OpenFail` 在世界还没暂停时 `SetGamePaused(true)`，和 `OpenShopDemo` 一样。世界已经暂停（例如商店 Demo 或三选一）时不抢所有权，确认后也不 `SetGamePaused(false)`。三选一还开着时这只壳拒绝打开，免得把词条暂停解开。确认时先收起、解开**自己**的暂停，再广播。流程要自己暂停时传 `bPause=false`。

视口 Z = 90。HUD 是 10，调试战斗条 50，商店 80，三选一 100。深底盖住 HUD，本壳不调用 `AC_HUDPresenter`。

无 Lab 键。不要去改 F6–F11，也不要加 F12。Play 之后在控制台执行：

```
PixelSide.RunSummary.Demo Win
PixelSide.RunSummary.Demo Fail
```

也可以对玩家控制器 `CreateWidget` 出 `UPixelRunSummaryWidget`，再调 `OpenWin` / `OpenFail` / `Present`。

可选子蓝图：父类选显示名 `WBP_RunSummary`。控件名逐字一致并勾 Is Variable。图表留空。不要 `AddCore`、`PreviewCoreGain`、`SetScrap`、`AddRunScrap`。不要提交 uasset。

## 挂载 HUD

`AC_HUDPresenter`（`UPixelHUDPresenterComponent`）只负责把正式 HUD 放进本地玩家视口。它不暂停、不改输入模式，也不动 `AC_AffixChoicePresenter`。

`APixelSideCharacter` 构造时已经：

- 创建 `AC_HUDPresenter`
- `HUDWidgetClass = UPixelHUDWidget`

`BeginPlay`、`PossessedBy`、`PawnClientRestart` 都会 `EnsureMounted()`。组件自己的 `BeginPlay` 也会试一次。已经在视口里就不会再加一个。

Run / Lab：

1. 玩家身上留 `AC_HUDPresenter`（角色 C++ 已经有）。
2. `HUDWidgetClass` 指到 `UPixelHUDWidget`，或以后的 `WBP_HUD` 子类。留空则用 `UPixelHUDWidget`。
3. `bEnableHUD` 默认开（Shipping 也开）。要整条藏起来用控件 `SetVisibility(Collapsed)`，或把 `bEnableHUD` 关掉再 `EnsureMounted`。
4. 视口 Z 为 10。三选一是 100，选卡盖在 HUD 上面。调试战斗条若开着在 Z 50、(8, 40)，和顶栏错开，两边互不引用。
5. GameMode 或 PlayerController 若要自己挂：对玩家上的这个组件调 `EnsureMounted()`。不要再写一套监听去 `RequestAffixChoice`。

## 挂载三选一（只有一套 AffixChoice Presenter）

`AC_AffixChoicePresenter`（`UAffixChoicePresenterComponent`）已经在听 `OnAffixChoiceOffered` / `OnAffixChoiceClosed`。不要再写第二个监听器去 `RequestAffixChoice`。

`APixelSideCharacter` 构造时已经：

- `bEnableNativeChoiceUI = false`
- `AffixChoiceWidgetClass = UPixelAffixChoiceWidget`

因此 Lab 弹出的是正式三卡，不是 `UAffixChoiceDebugWidget` 文本条。

流程 / 弹芯：

1. 玩家身上留 `AC_AffixChoicePresenter`。
2. **`bEnableNativeChoiceUI` 必须为 false**，否则文本占位和正式卡会叠两层。
3. `AffixChoiceWidgetClass` 指到 `UPixelAffixChoiceWidget`（或以后的 `WBP_AffixChoice` 子类）。留空则回到 #29 文本占位（并把开关重新打开）。
4. 房间清空只调 `RequestAffixChoice`。子系统 `ApplyChoicePause(true)` 后广播 Offered。UI 不暂停、不 Roll。
5. Presenter 把控件加进视口并 `PresentOfferedChoice`。控件用 `GetAffixCardView` 填 Name / Desc / Rarity / Icon。
6. 点击或 1 / 2 / 3 / Enter 只调 `ConfirmAffixChoice(Index)`。
7. 子系统解暂停并广播 `OnAffixChoiceClosed`。控件收起。

## 编辑器里做 HUD 子蓝图（可选）

1. 编译 `PixelSideRL`（UMG / Slate / SlateCore 已在 Public）。
2. `Content/Blueprints/UI/` → User Interface → Widget Blueprint。
3. 父类选显示名 `WBP_HUD` 或 `WBP_HUD_AffixIcon`，不要选裸的 User Widget。
4. 控件名与上表逐字一致，并勾 Is Variable（`BindWidgetOptional`）。词条格父类用 `WBP_HUD_AffixIcon`，放进 `HB_AffixIcons`。不满 8 个且没有别的子控件时，C++ 会补到 8 个。
5. 图表留空。不要 `ApplyDamage`、`Fire`、`GetHealthRatio`、`RollChoices`。血和 Scrap 只调 `SetHealthRatio` / `SetAmmo` / `SetScrap` / `SetWeaponIcon`。
6. 玩家组件上 `HUDWidgetClass` 改指这个 WBP。

## 编辑器里做三选一子蓝图（可选）

1. 编译 `PixelSideRL`（UMG / Slate / SlateCore 已在 Public）。
2. `Content/Blueprints/UI/` → User Interface → Widget Blueprint。
3. 父类选显示名 `WBP_AffixChoice` 或 `WBP_AffixCard`，不要选裸的 User Widget。
4. 控件名与上表逐字一致，并勾 Is Variable（`BindWidgetOptional`）。
5. 图表留空。不要 `RequestAffixChoice`、`RollChoices`、`ApplyDamage`、`SetGamePaused`。
6. 玩家组件上 `AffixChoiceWidgetClass` 改指这个 WBP。**开关保持 false。**

## 事件选择 `WBP_EventChoice`

画布逻辑 640×360。文案对齐 `Content/Data/Events/DA_Event_AbandonedCache.json`，不建 DataAsset uasset。线框：`Docs/UI_Wireframes_Event.md`。

| 控件名 | 类型 | 数据 |
|--------|------|------|
| `Overlay_Root` | Overlay，全屏 | |
| `Img_Dim` | Image，`#000000` 60% | |
| `Txt_Title` | Text Block | `DisplayName`（废弃补给箱） |
| `Txt_Body` | Text Block | `PromptBody` |
| `Btn_Choice_1` `Btn_Choice_2` `Btn_Choice_3` | `WBP_EventChoiceButton` | `Choices[].Label`。MVP 只显示前两个，第三个折叠 |
| `Btn_Skip` | Button | 跳过 |
| `Txt_Skip` | Text Block，放在 `Btn_Skip` 里 | `SkipHint`：走开（无奖励） |

按钮内部：`Btn_Select`、`Txt_Hotkey`（1 / 2 / 3）、`Txt_Label`。

壳不自己 `GrantScrap`、不 `DamageSelf`、不 `HealSelf`、不 `SetGamePaused`、不 `RequestAffixChoice`。确认选项调用一次 `ResolveEventChoice`。Skip / Esc 不调用。`bAffixOnResolve` 保持 false。

打开时 `APixelSideCharacter::SetEventChoiceBlocksControl(true)`，只丢掉移动 / 跳跃 / 开火。世界不暂停，所以门体积不会被冻住（GDD「门仍可走」）。关闭时恢复，并广播 `OnEventClosed`。选中先 Resolve，再广播 `OnEventChoiceSelected(ChoiceId)`；跳过和 Esc 只广播 `OnEventSkipped`（无奖励，不 Resolve）。

非 Shipping：`F10` 打开 AbandonedCache Demo。`F8` / `F9` 仍是直调 Resolve，不要占用。`F6` / `F7` 的映射不动。`F11` 仍是商店。事件开着时 `F6` / `F7` 先被忽略，关掉之后照旧。三选一还开着时 `F10` 不起作用。

可选子蓝图：父类选显示名 `WBP_EventChoice` 或 `WBP_EventChoiceButton`。控件名逐字一致并勾 Is Variable。图表留空。不要提交 uasset。

## Lab 里 Play

非 Shipping，`APixelSideGameMode`，`/Engine/Maps/Entry` 即可。正式 HUD 和三选一都会出来。

1. Play 后顶栏就在：满血绿条、武器浅色块、`99`、`Scrap 000`、右边 8 个灰词条格。点击打得到世界，HUD 不挡。
2. `F6`：`DebugGrantAffix("RapidFire")`。第一格从灰变成已拥有（当前没有 Icon，所以是浅色块）。调试战斗条若开着仍在 (8, 40)，不是这条顶栏。
3. 走动，确认没有三选一遮罩。
4. `F7`：子系统暂停并给出 3 张真实卡（`GetAffixCardView`，不是占位文案）。HUD 还在底下。
5. 按 `1` 或点左边那张。遮罩消失，可以再走再开火。词条格多一格已拥有。
6. 选中带射速的词条后，冷却应短于暂停前。
7. 血和 Scrap 还没有战斗只读接口。对视口里的 `UPixelHUDWidget` 调 `SetHealthRatio` / `SetScrap`（默认满血、`Scrap 000`）。≤0.25 血条变红并闪，≤0 血条空。
8. `F10`：事件遮罩。标题「废弃补给箱」，两个按钮「小心撬开」「硬拽出来」，跳过「走开（无奖励）」。正式 HUD 仍挂在底下（Z 10），不被拆掉。移动和开火停住，游戏时间不暂停。`F8` / `F9` 仍是不打开界面的直调 Resolve。
9. 按 `1` 或点「小心撬开」。遮罩消失，Log 仍有 `OnEventChoiceSelected ChoiceId=CarefulLoot`。同时 Resolve，Scrap +25，与 `F8` 相同。可以再走再开火。不弹出三选一。再按 `F10` 选「硬拽出来」：与 `F9` 相同，先 −15 HP 再 +50 Scrap。再按 `F10` 后点跳过或 Esc：只有 `OnEventSkipped`，钱包和 HP 不变。
10. `F11` 开商店。遮罩下仍能看见 HUD。三格：豆射手 45、回复 30、三选一券 60。右上 `Scrap 128`。`F6`–`F10` 不改。这一键不经过 `AC_ShopPresenter`。房间流程用上一节的 `OpenShop` / `OpenShopDemo` / `CloseShop`。
11. 点回复或把焦点移过去按 Enter。该格变 Sold，`Scrap 98`。
12. `离开` 或 Esc。遮罩消失，人能走能开火。再按 `F11` 会把 Demo 货架和 128 重置。买券只发 `OnRequestAffixChoiceFromVoucher`，不自己开三选一，也不 `AddRunScrap`。
13. 结算没有键。F6–F11 不要改，也不要按 F12（没有这个映射）。控制台 `PixelSide.RunSummary.Demo Win`：胜利，`16:20`，击杀 86，`获得 Core +42`，`Scrap 结余 40`。`PixelSide.RunSummary.Demo Fail`：失败小字出现，Core 是 `+8`。Enter / Space / Esc / `回据点` 都只打 `OnContinueToHub`，不写 Core。关掉之后 F6–F11 仍是原来的键。

要临时看 #29 文本条：清掉 `AffixChoiceWidgetClass`，把 `bEnableNativeChoiceUI` 勾回去。不要两个同时开。不要关 `AC_HUDPresenter` 来换那条文本。
