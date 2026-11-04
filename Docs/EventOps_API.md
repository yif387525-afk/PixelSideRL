# Event Op 运行时

**Owner：** 词条官  
**真相源：** `Content/Data/Events/SCHEMA_EventDefinition.json`、`DA_Event_AbandonedCache.json`、`DT_Event_MVP.json`  
**实现：** `UEventSubsystem`（GameInstance）+ 主机可测的 `PixelSideEvent::FCatalog`  
**不做：** `WBP_EventChoice`、房间图、商店购买、Core / `FMetaSaveData`、CombatFeel

局内货币与商店锁表一致：只动 **Scrap**（`currencyScope=Run`）。商店标价仍在 `DA_ShopInventory` / `DA_ShopPriceCurve`，本运行时不买东西。

---

## 1. 加载

与 P0.6 词条读 JSON 相同，不要求 uasset。

| 文件 | 作用 |
|------|------|
| `Content/Data/Events/DA_Event_<EventId>.json` | 一条事件。冻名 `DA_Event_<EventId>` |
| `Content/Data/Events/DT_Event_MVP.json` | 池。`daNamingRule` 必须是 `DA_Event_<EventId>` |
| `SCHEMA_EventDefinition.json` | 字段约定。运行时不执行这份说明文件 |

`UEventSubsystem::Initialize` 读目录里全部 `DA_Event_*.json`，再对照池。`DataAssetName` 缺省或必须等于冻名；对不上就拒绝该文件并打日志。不要提交占位 `.uasset`。

`GetEventDefinition(EventId)` 给以后的界面读标题、正文、选项。本 PR 没有控件。

---

## 2. 结算

```
ResolveEventChoice(EventId, ChoiceId, OutResult) -> bool
OnEventChoiceResolved(FEventResolveView)
```

`Choices[].Effects` 在同一次调用里按数组顺序执行。`AbandonedCache` / `ForceLoot` 先 `DamageSelf 15`，再 `GrantScrap 50`。

| Op | 效果 |
|----|------|
| `GrantScrap` | `RunScrap += Value`（四舍五入成整数，结果不低于 0）。不乘 `ScrapGain`，不改 Core |
| `DamageSelf` | 活着的目标扣 `Value`。结算后 `CurrentHP` 下限 **1**，不走 `ApplyDamage`，因此没有受击闪白、击退、顿帧、无敌帧，也不会死。已经死亡（HP ≤ 0）不复活 |
| `HealSelf` | 活着的目标加 `Value`。有 MaxHP 时不超过 MaxHP。不复活 |
| `None` | 不改 Scrap / HP |
| 其他 | Output Log 警告，该步跳过，后面的效果继续，进程不崩 |

`bAffixOnResolve == false` 时不请求三选一。MVP 即使表被写成 `true` 也只打日志，不调用 `RequestAffixChoice`。`bCombatAllowed` 在 AbandonedCache 上是 `false`；写成 `true` 同样不开战。

失败（未知 EventId / ChoiceId）不改钱包和 HP。

`UPixelEventChoiceWidget` 在确认选项时调用一次 `ResolveEventChoice`，然后仍广播 `OnEventChoiceSelected`。Skip / Esc 不调用 Resolve。失败（未知 EventId / ChoiceId、子系统缺失）不改钱包和 HP，UI 仍可关，不二次应用。壳不 `SetGamePaused`。

---

## 3. 局内 Scrap 占位

没有单独的局内钱包类型时，字段挂在子系统上：

| 字段 | 类型 | 含义 |
|------|------|------|
| `UEventSubsystem::RunScrap` | `int32` | 本局 Scrap。`GetRunScrap` / `AddRunScrap` / `ResetRunScrap` |

- 不是 `FMetaSaveData.Core`，不写 `Saved/SaveGames`。
- 不是 `UAffixSubsystem` 里的 `StartingScrap`（那只是开局注入快照，不能加减）。
- 开局由玩家 `BeginAffixRun` 调用 `UMetaProgressSubsystem::BeginRunFromMeta`：先 `UAffixSubsystem::BeginRun(Ctx)`，再 `ResetRunScrap(Ctx.StartingScrap)`。StartingScrap 来自 Meta Stash 已购档，未购为 0。没有 Meta 子系统时仍用空上下文并 `ResetRunScrap(0)`。别的流程自己 `BeginRun` 时也要调一次 `ResetRunScrap`，否则钱包仍从 0 起。
- 商店 Run 路径（`UPixelShopWidget::OpenShop`）扣款走 `AddRunScrap(-Price)`，刷新走 `AddRunScrap(-RerollCost)`，扣完再 `GetRunScrap` 刷新标签。不要再造第二份整数，也不要写成 Core。Lab `F11` / `OpenShopDemo` 只改本地 DemoScrap，不调用这两个函数。Meta 除了这一次开局重置，不改 RunScrap。`AC_ShopPresenter` 只转发开/关，自己不扣。

HP 写在玩家 `AC_Damageable::ApplyEventHealth`。战斗扣血仍只走 `ApplyDamage`。

---

## 4. 非 Shipping 调试

| 入口 | 行为 |
|------|------|
| `F8` | `ResolveEventChoice("AbandonedCache", "CarefulLoot")` |
| `F9` | `ResolveEventChoice("AbandonedCache", "ForceLoot")` |
| `PixelSide.ResolveEvent [EventId] [ChoiceId]` | 同上，默认 AbandonedCache / CarefulLoot |

Output Log 类别 `LogPixelSideEvent`。每步一行 `HP a -> b` 和 `Scrap a -> b`，最后一行是 pawn 上的 HP 与 `RunScrap`。ForceLoot 在低血量时 HP 停在 1，且 `alive=1`。不改手感表。

`F10` 打开 `WBP_EventChoice`。确认「小心撬开」走与 F8 相同的 Resolve；「硬拽出来」走与 F9 相同的 Resolve。Skip / Esc 不调用。`F11` 是商店，不在本运行时。F6–F11 的键映射不改。

---

## 5. 主机校验

```bash
g++ -std=c++17 -Wall -Wextra -Werror -I Source/PixelSideRL/Public \
  -o /tmp/test_event_ops Scripts/test_event_ops.cpp \
  Source/PixelSideRL/Private/Event/EventOps.cpp
/tmp/test_event_ops .
```

覆盖：CarefulLoot +25；ForceLoot 先 −15 HP 再 +50 Scrap；HP 10 时伤害落底 1；HP 1 仍为 1；已死亡不复活；`None` 无副作用；未知 Op 留日志且不中断后续效果、不改 Core。
