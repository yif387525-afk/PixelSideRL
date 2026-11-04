# Content/Blueprints/UI

Widget 落库目录。命名 `WBP_`。P0：`WBP_HUD`、`WBP_AffixChoice`、`WBP_AffixCard`。线框见 Docs/UI_Wireframes_P0.md。

`WBP_DebugCombatHUD` 不放这里。它只属于 `Content/_Dev/Combat/UI/`。正式 HUD 不要引用它。

**不要提交 `WBP_*.uasset`。** 没有蓝图时，C++ 父类自己建出同名控件。

## 命名

| 资产名（编辑器里再建） | C++ 父类 | 类选择器显示名 | 头文件 |
|------------------------|----------|----------------|--------|
| `WBP_AffixChoice` | `UPixelAffixChoiceWidget` | `WBP_AffixChoice` | `Source/PixelSideRL/Public/UI/PixelAffixChoiceWidget.h` |
| `WBP_AffixCard` | `UPixelAffixCardWidget` | `WBP_AffixCard` | `Source/PixelSideRL/Public/UI/PixelAffixCardWidget.h` |

路径：`Content/Blueprints/UI/`。画布逻辑 640×360。三张卡实例名必须是 `Card_0`、`Card_1`、`Card_2`。

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

## 挂载（只有一套 Presenter）

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

## 编辑器里做子蓝图（可选）

1. 编译 `PixelSideRL`（UMG / Slate / SlateCore 已在 Public）。
2. `Content/Blueprints/UI/` → User Interface → Widget Blueprint。
3. 父类选显示名 `WBP_AffixChoice` 或 `WBP_AffixCard`，不要选裸的 User Widget。
4. 控件名与上表逐字一致，并勾 Is Variable（`BindWidgetOptional`）。
5. 图表留空。不要 `RequestAffixChoice`、`RollChoices`、`ApplyDamage`、`SetGamePaused`。
6. 玩家组件上 `AffixChoiceWidgetClass` 改指这个 WBP。**开关保持 false。**

## Lab 里 Play

非 Shipping，`APixelSideGameMode`，`/Engine/Maps/Entry` 即可。

1. 走动，确认没有遮罩。
2. `F7`：子系统暂停并给出 3 张真实卡（`GetAffixCardView`，不是占位文案）。
3. 按 `1` 或点左边那张。遮罩消失，可以再走再开火。
4. 选中带射速的词条后，冷却应短于暂停前。`F6` 仍是直接 `DebugGrantAffix("RapidFire")`，不经过三选一。

要临时看 #29 文本条：清掉 `AffixChoiceWidgetClass`，把 `bEnableNativeChoiceUI` 勾回去。不要两个同时开。
