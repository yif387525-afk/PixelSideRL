# Meta 进度运行时

**Owner：** 词条官  
**真相源：** `Content/Data/Meta/DA_Economy.json`（contractVersion=3，已锁）、`Content/Data/Meta/DT_MetaNodes_MVP.json`（五节点）、`Content/Data/Affixes/DT_Affix_MVP.json` 的 `affixes[]`（十词条）  
**实现：** `UMetaProgressSubsystem`（GameInstance）+ 主机可测的 `PixelSideMeta::FProgress`  
**对齐：** `Docs/MetaSaveInterface.md`、`Docs/RoguelikeMeta.md` §6–§7

## 不做

- 不写 `Saved/SaveGames/PixelSideRL_Meta.sav`（`Save` / `Load` 返回 false，钱包留在内存）
- 不造 `DA_MetaNode_*.uasset`，也不拆单独的 `DA_MetaNode_*.json`。冻名写在节点的 `DataAssetName` 上
- 不做商店购买、Hub WBP、房间清房、完整两层 Run
- 不改 `Content/Data/Shop/*`，不改 `Docs/CombatFeel.md`
- 不改 F6–F11。F8 / F9 仍是 CarefulLoot / ForceLoot，F10 仍是事件壳，F11 仍是商店壳
- 不做商店购买 runtime，不改商店壳，不做 EventChoice 界面

局外货币是 **Core**。局内货币是 **Scrap**（`UEventSubsystem::RunScrap`）。两者不要混。

---

## 1. 读表

与词条 / Event 一样，运行时读 JSON，不要求 uasset。

| 文件 | 作用 |
|------|------|
| `Content/Data/Meta/DA_Economy.json` | 结算系数。`MetaCoreMult` 基值 1.0，`SoftCapPerRun` 180，`ScrapToCore` 0.1 |
| `Content/Data/Meta/DT_MetaNodes_MVP.json` | Vitality / Stash / PeaFamiliar / SalvageSense / SidearmLuck |
| `Content/Data/Affixes/DT_Affix_MVP.json` | 只读 `affixes[]` 的十个 AffixId。`postMvpCandidates` 不进池 |

`daNamingRule` 必须是 `DA_MetaNode_<NodeId>`，`mvpAffixPoolPolicy` 必须是 `AllTenUnlockedAtRunStart`，`contractVersion` 必须是 3。`DataAssetName` 对不上冻名就拒绝整张表。

永久修饰若写 `CritChance` / `Pierce` / `ProjectileCount`，读表失败。这三条不能从 Meta 直接加。

节点上的 `UnlockAffixIds` 会解析，但**不**用来开关词条池。池子始终是那十个（含 Piercer、TwinShot）。非空的 Unlock 列表只打日志，出现在 `IgnoredUnlock`。

---

## 2. 内存档

新档：`Core = 0`，五个节点档位都是 0。`ResetSlot()` 回到这个状态。杀进程不会保留；本 PR 没有存档文件。

`GetPurchasedRank(NodeId)` 未知或未购都返回 0。`GetPurchasedNodes()` 把已加载节点都列出来，含 0 档。

`AddCore(Delta, Reason)` 只改 Core。结果不低于 0，也不超过 `INT_MAX`。Reason 进日志，不进局内 Scrap。

---

## 3. 购买

```
CanPurchase(NodeId) -> bool
Purchase(NodeId) -> bool
```

下一档费用是 `CostCoreByRank[当前档]`。例如 Stash 0→1 花 20，1→2 花 40，2→3 花 75。`当前档 >= MaxPurchases` 则失败，Core 不变。Core 不够失败，不扣。前置节点（`Prerequisites`）至少 1 档才放行；MVP 锁表前置都是空的。

效果取**该档**那一行，不把低档叠上去：

| 节点 | 档 1 / 2 / 3 | 进开局上下文的位置 |
|------|----------------|---------------------|
| Vitality | MaxHP Add +10 / +20 / +30 | `PermanentModifiers` |
| Stash | StartingScrap 15 / 30 / 50 | `StartingScrap`。未购是 0 |
| PeaFamiliar | Damage Mul +0.04 / +0.08 / +0.12，`WeaponTag=Peashooter` | `PermanentModifiers`。不是 `StartWeaponId` |
| SalvageSense | `MetaCoreMult` +0.05 / +0.10 / +0.15 | `GetMetaCoreMult()` = 表基值 + 该档 delta |
| SidearmLuck | 旗标 `GuaranteeWeaponRoomAfterRoom2_Act1`（只买 1 次，45 Core） | `MetaFlags`。本 PR 不生成武器房 |

---

## 4. 开局顺序

玩家 `BeginPlay` → `BeginAffixRun`。还没进 Run 时：

```
Ctx = UMetaProgressSubsystem::BuildRunStartContext()
UAffixSubsystemDefault::BeginRun(Ctx)          // 若子系统在
UEventSubsystem::ResetRunScrap(Ctx.StartingScrap)
```

`BeginRunFromMeta()` 把这两步包在一起，并打一行：

```
BeginRunFromMeta StartingScrap=<n> RunScrap=<n> ...
```

`ResetRunScrap` 先把局内钱包清 0，再写入 StartingScrap（负数当 0）。这和主机侧 `PixelSideMeta::SeedRunScrap` 一致。

没有 Meta 子系统时，`BeginAffixRun` 仍用空的 `FRunStartContext` 并 `ResetRunScrap(0)`，和以前的空开局一样。

`UnlockedAffixIds` 在读表成功时是十个 MVP Id。P0.6 实际加载的是其中 8 条（不含 Adrenaline / GlassCannon）；这十个把那 8 条都留在池里，不另开 Unlock 门。读表失败时列表留空，Affix 把空列表当成「当前已加载池」。

`BeginRun` 只把修饰的 `Stat` / `Op` / `Value` / `DurationSec` 放进 Meta 层。`WeaponTag` 留在 `FAffixModifier` 上，折算层还不会按枪过滤。因此**若调试买了 PeaFamiliar，Damage Mul 会进所有武器的 Meta 伤害**，直到折算认识 `WeaponTag`。默认档 0 不注入，Lab 不开局买节点时手感不变。`MetaFlags` 和 `MetaCoreMult` 同样不被 Affix `BeginRun` 读取。

调试再注入走控制台 `PixelSide.Meta.BeginRun`。它会再调 `BeginRunFromMeta`，`BeginRun` 会清掉本局已拿的词条。不新绑键。

---

## 5. MetaCoreMult 与结算预览

`GetMetaCoreMult()`：经济表基值，加上 SalvageSense **当前档**的 delta。未购 = 1.0。表没载入 = 0。

`PreviewCoreGain(bCleared, RoomsCleared, BossKilled, RunScrapRemain)` 按 `Docs/RoguelikeMeta.md` §6 算一笔整数，**不调用 `AddCore`，不结束 Run**：

```
Core = floor(
    ( BaseClearBonus * (Cleared ? 1 : DeathMult)
    + RoomsCleared * PerRoom
    + BossKilled * BossBonus
    + RunScrapRemain * ScrapToCore
    ) * MetaCoreMult
    + 1e-9
)
Core = min(Core, SoftCapPerRun)
```

`1e-9` 只用来躲开 `0.1` 这类二进制小数把整数边界收小。`MetaSaveInterface.md` 里仍写着 `RunScrapCollected`，和 §6 的 `RunScrapRemain` 不是同一字段；本预览跟 §6。局内 Coin→Scrap（`CoinToScrap`）不在这里做。

---

## 6. 和 RunScrap、商店

| 动作 | 谁改 |
|------|------|
| 开局把 Stash 注入局内钱包 | `ResetRunScrap(StartingScrap)` |
| 事件发 Scrap、商店扣款 | `AddRunScrap(Delta)`，扣款是 `AddRunScrap(-Price)` |
| Hub 买永久节点 | `Purchase` 扣 **Core** |

本子系统不读 `DA_ShopInventory`，也不卖回复 / 三选一券 / 武器合约。

---

## 7. 调试（非 Shipping）

| 入口 | 行为 |
|------|------|
| `PixelSide.Meta.AddCore [Delta=100]` | `AddCore`，Reason=`Debug`。不改 RunScrap |
| `PixelSide.Meta.Purchase [NodeId=Stash]` | 买一档 |
| `PixelSide.Meta.Context` | 打印 `DescribeRunStartContext`（Core、档位、StartingScrap、修饰、十词条） |
| `PixelSide.Meta.BeginRun` | `BeginRunFromMeta`，日志里有 StartingScrap 和 RunScrap |

F6 仍是 `DebugGrantAffix("RapidFire")`，F7 仍是三选一，F8 / F9 仍是 AbandonedCache，F10 仍是事件壳，F11 仍是商店壳。本子系统不新绑这些键。Shipping 不注册上面的命令。

---

## 8. 主机校验

无编辑器时跑这个，不 PIE：

```bash
g++ -std=c++17 -Wall -Wextra -Werror -I Source/PixelSideRL/Public \
  -o /tmp/test_meta_progress Scripts/test_meta_progress.cpp \
  Source/PixelSideRL/Private/Meta/MetaProgress.cpp
/tmp/test_meta_progress .
```

覆盖：五节点费用与冻名、新档全 0、Stash 15/30/50 写入 `SeedRunScrap`、Vitality 只取该档 MaxHP、PeaFamiliar 的 WeaponTag、SalvageSense 倍率、SidearmLuck 旗标、不超过 MaxPurchases、Core 不足不扣、UnlockAffix 的 Piercer/TwinShot 不改十词条池、Pierce 修饰被拒绝、结算预览不入账、`Content/Data/Meta` 下没有 `.uasset` 或 `DA_MetaNode_*` 文件。
