# Content/Blueprints/UI

Widget 落库目录。命名 `WBP_`。P0：`WBP_HUD`、`WBP_HUD_AffixIcon`、`WBP_AffixChoice`、`WBP_AffixCard`、`WBP_EventChoice`。线框见 Docs/UI_Wireframes_P0.md 与 Docs/UI_Wireframes_Event.md。

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

不 `GrantScrap`、不 `DamageSelf`、不 `HealSelf`、不 `ResolveEventChoice`、不 `SetGamePaused`、不 `RequestAffixChoice`。`bAffixOnResolve` 保持 false。

打开时 `APixelSideCharacter::SetEventChoiceBlocksControl(true)`，只丢掉移动 / 跳跃 / 开火。世界不暂停，所以门体积不会被冻住（GDD「门仍可走」）。关闭时恢复，并广播 `OnEventClosed`。选中只广播 `OnEventChoiceSelected(ChoiceId)`；跳过和 Esc 只广播 `OnEventSkipped`（无奖励）。

非 Shipping：`F10` 打开 AbandonedCache Demo。`F8` / `F9` 仍是直调 Resolve，不要占用。`F6` / `F7` 的映射不动。事件开着时这两个键先被忽略，关掉之后照旧。三选一还开着时 `F10` 不起作用。

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
9. 按 `1` 或点「小心撬开」。遮罩消失，Log 为 `OnEventChoiceSelected ChoiceId=CarefulLoot`。可以再走再开火。不加 Scrap，不扣血，不弹出三选一。再按 `F10` 后点跳过或 Esc：`OnEventSkipped`，无奖励。

要临时看 #29 文本条：清掉 `AffixChoiceWidgetClass`，把 `bEnableNativeChoiceUI` 勾回去。不要两个同时开。不要关 `AC_HUDPresenter` 来换那条文本。
