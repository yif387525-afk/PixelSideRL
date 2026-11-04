# 事件线框：WBP_EventChoice

**状态：** C++ 壳 `UPixelEventChoiceWidget`（显示名 `WBP_EventChoice`）。无 uasset。  
**画布：** 640×360 · 最近邻 · 安全区 8px  
**临时色：** 与 [UI_Wireframes_P0.md](./UI_Wireframes_P0.md) 文末一致  
**落库：** `Content/Blueprints/UI/`（可选子蓝图，勿提交 uasset）  
**配方：** [Content/Blueprints/UI/README.md](../Content/Blueprints/UI/README.md)  
**文案源：** `Content/Data/Events/DA_Event_AbandonedCache.json`（锁表）。GDD §2.8。房间 `Room_Event_01`。

确认选项（按钮 / `1`·`2`·`3` / 焦点在选项上按 Enter）调用一次 `UEventSubsystem::ResolveEventChoice(EventId, ChoiceId)`，再广播 `OnEventChoiceSelected`，关 UI，然后 `OnEventClosed`。壳不自己加减 Scrap / HP，也不 `SetGamePaused`。效果由子系统按 DA 执行（`DamageSelf` 后 CurrentHP 下限 1）。Skip / Esc（以及焦点在 Skip 上的 Enter）不调用 Resolve，只广播 `OnEventSkipped`，然后 `OnEventClosed`。未知 EventId / ChoiceId 或子系统缺失时打日志，不崩，UI 仍关闭，不二次应用效果。**不**打开 `WBP_AffixChoice`（`bAffixOnResolve=false`）。

---

## 1. 布局（640×360）

```
┌──────────────────────────────────────────────────────────────┐
│                         （半透明遮罩 60%）                      │
│                                                              │
│                         废弃补给箱                             │
│   封条已经裂开。小心撬开能摸到 Scrap；硬拽更快，但箱缘会割手。    │
│                       也可以直接走开。                         │
│                                                              │
│                    ┌────────────────────────┐                │
│                    │ 1   小心撬开            │                │
│                    └────────────────────────┘                │
│                    ┌────────────────────────┐                │
│                    │ 2   硬拽出来            │                │
│                    └────────────────────────┘                │
│                                                              │
│                     [ 走开（无奖励） ]                         │
└──────────────────────────────────────────────────────────────┘
```

面板宽约 420，垂直居中。选项 1–3 纵向排列，间距 8。MVP 只有两条，第三条折叠。

---

## 2. 控件

| 控件名 | 类型 | 数据 |
|--------|------|------|
| `Overlay_Root` | Overlay | 全屏 |
| `Img_Dim` | Image | `#000000` @ 60% |
| `Txt_Title` | TextBlock | `DisplayName` |
| `Txt_Body` | TextBlock | `PromptBody`，约 400px 宽换行 |
| `Btn_Choice_1` `Btn_Choice_2` `Btn_Choice_3` | `WBP_EventChoiceButton` | `Choices[i].Label`；没有的槽 Collapsed |
| `Btn_Skip` | Button | 点击 = 跳过 |
| `Txt_Skip` | TextBlock | `SkipHint`，放在 `Btn_Skip` 里 |

`WBP_EventChoiceButton`（`UPixelEventChoiceButton`）内部：`Btn_Select`、`Txt_Hotkey`（`1`/`2`/`3`，UI 本地）、`Txt_Label`（DA 原文）。

`BindWidgetOptional`。缺控件且没有设计师树时，C++ 按上表自己建。

---

## 3. Demo 文案（AbandonedCache）

与 `DA_Event_AbandonedCache.json` 逐字相同。Lab 用这段硬编码，**不**建 DataAsset uasset。

| 字段 | 文案 |
|------|------|
| EventId | `AbandonedCache` |
| DisplayName | 废弃补给箱 |
| PromptBody | 封条已经裂开。小心撬开能摸到 Scrap；硬拽更快，但箱缘会割手。也可以直接走开。 |
| SkipHint | 走开（无奖励） |
| `CarefulLoot` | 小心撬开 |
| `ForceLoot` | 硬拽出来 |
| `bAllowDoorSkip` | true |
| `bCombatAllowed` | false |
| `bAffixOnResolve` | false |

效果由 `UEventSubsystem::ResolveEventChoice` 按 DA 执行。壳不自己加减 Scrap / HP：

| ChoiceId | Op 顺序 |
|----------|---------|
| `CarefulLoot` | `GrantScrap 25` |
| `ForceLoot` | `DamageSelf 15` → `GrantScrap 50` |

跳过：无奖励。不新增 EventId。

---

## 4. 打开 / 关闭（暂停）

GDD §2.8：**打开期间可暂停移动开火；门仍可走。**

| 时机 | 行为 |
|------|------|
| Open | `APixelSideCharacter::SetEventChoiceBlocksControl(true)`：停掉当前速度和按住开火，之后忽略移动 / 跳跃 / 开火。UI 模态（光标，点按钮）。**不** `SetGamePaused`。 |
| Close | 收起遮罩，输入回到 GameOnly，`SetEventChoiceBlocksControl(false)`，再广播 `OnEventClosed`。 |
| 门 | `bAllowDoorSkip` 留在视图上。世界时钟照走，门体积不会被整局暂停冻住。本壳不锁出口。真 `Room_Event_01` 接入时出门不依赖「先解暂停」。Lab 没有门，优先把移动 / 开火输入关掉。 |
| 跳过 | `Btn_Skip` 文案就是 SkipHint。Esc 或焦点在跳过上按 Enter，同样 `OnEventSkipped`。 |

三选一仍走自己的 `ApplyChoicePause`（会 `SetGamePaused`）。事件壳自己不 `SetGamePaused`。事件打开时忽略 `F6` / `F7`，避免再冻住世界。事件关上之后 `F6` / `F7` 与以前相同。`F10` 在三选一还开着时不起作用。`F8` / `F9` 仍是不打开界面的直调 Resolve。`F11` 仍是商店。本 PR 不改 F6–F11。

---

## 5. 输入与事件

| 输入 | 行为 |
|------|------|
| `1` / `2` / `3` | 确认对应选项（没有该槽则忽略），并调用一次 Resolve |
| ↑ / ↓ · W / S | 焦点在选项和跳过之间移动 |
| Enter / Space | 确认焦点并 Resolve；焦点在跳过上则跳过，不 Resolve |
| 点击选项 | `ResolveEventChoice` 一次，然后 `OnEventChoiceSelected(ChoiceId)` |
| 点击 `Btn_Skip` / Esc | `OnEventSkipped`。不调用 Resolve |
| A / D | 不由本界面占用。Lab 里角色会丢掉移动，因为控制被关掉 |

| 事件 | 何时 | 载荷 |
|------|------|------|
| `OnEventChoiceSelected` | 点选或热键确认。Resolve 已经调用过一次 | `ChoiceId`（`CarefulLoot` / `ForceLoot`） |
| `OnEventSkipped` | 跳过。不调用 Resolve | 无。无奖励 |
| `OnEventClosed` | 上面二者之后，界面已收起、移动开火已恢复 | 无 |

壳不写 Scrap、不扣血、不治疗、不 `SetGamePaused`、不 `RequestAffixChoice`。监听 `OnEventChoiceSelected` 的一方不要再调 `ResolveEventChoice`，否则效果会加两次。角色上现有的监听只打日志。

---

## 6. Lab

非 Shipping，`APixelSideCharacter`，`F10` 开壳。`F8` / `F9` 仍是直调 Resolve。`F11` 仍是商店。键映射不改。

1. 走动、开火正常；没有遮罩。`F6` 仍是 RapidFire，`F7` 仍是三选一。
2. `F10`：标题「废弃补给箱」，正文为上表，两个按钮「小心撬开」「硬拽出来」，跳过「走开（无奖励）」。移动和开火停住。游戏时间不暂停（本壳不 `SetGamePaused`）。
3. 按 `1` 或点「小心撬开」：遮罩消失。Output Log 仍有 `OnEventChoiceSelected ChoiceId=CarefulLoot`，屏幕短提示 `OnEventChoiceSelected CarefulLoot`。同时 `ResolveEventChoice(AbandonedCache, CarefulLoot)`，Scrap 增加与 `F8` 相同（+25）。可以再走再开火。不弹出三选一。
4. 再按 `F10`，点「硬拽出来」或按 `2`：`ResolveEventChoice(AbandonedCache, ForceLoot)`，与 `F9` 相同（先 −15 HP，下限 1，再 +50 Scrap）。Log 仍有 `OnEventChoiceSelected ChoiceId=ForceLoot`，然后 `OnEventClosed`。
5. 再按 `F10`，点跳过或 Esc（焦点在跳过上按 Enter 同样）：只有 `OnEventSkipped`，然后 `OnEventClosed`。不调用 Resolve。钱包和 HP 不变。
6. `F8` / `F9` 行为不变：不打开界面，直接 Resolve。`F6`–`F11` 的键不变。`F11` 仍开商店。

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | C++ 壳：AbandonedCache Demo，`F10`，只关移动/开火，不 `SetGamePaused`，不调用 Resolve |
| 2026-11-05 | 确认选项调用一次 `ResolveEventChoice`。Skip / Esc 不调用。不 `SetGamePaused`。F6–F11 不动 |
