# AffixSubsystem / BP 接口清单（弹芯 · 界面对接）

**Owner:** 词条官  
**状态:** 契约 v1（2026-11-04）— 字段名冻结；数值待织局按 AffixId 回填后锁表  
**实现:** GameInstance Subsystem（C++ `UAffixSubsystem` 优先；MVP 可用 BP 同名子系统）

---

## 0. AffixCard 稳定展示字段（冻结）

`WBP_AffixCard` / `WBP_AffixChoice` **只读**下列字段。改名需走契约升版。

| 契约名（UI） | DataAsset 字段 | 类型 | 说明 |
|--------------|----------------|------|------|
| **Name** | `DisplayName` | `FText` | 卡面标题 |
| **Desc** | `Description` | `FText` | 短描述；可含 `{Value}`，展示前由 `FormatDesc` 替换 |
| **Rarity** | `Rarity` | `EAffixRarity` | 描边/色；Common/Rare/Epic/Legendary |
| **Icon** | `Icon` | Soft `UTexture2D` / `UPaperSprite` | 可空 → UI 用占位图 |

辅助（可选读，不保证每卡都有）：

| 字段 | 用途 |
|------|------|
| `AffixId` | 调试、埋点、互斥 |
| `Tags` | 角标分类（Offense/Defense/…） |
| `CurrentStacks` / `MaxStacks` | 已拥有叠层时卡面角标 |

**BP 取值约定：** 传入 `UAffixDefinition*` 或 `FAffixChoiceEntry`，用下方 `GetAffixCardView` 一次取齐，避免 UI 直接扒私有字段。

```
FAffixCardView GetAffixCardView(const UAffixDefinition* Def, int32 PreviewStacks = 1)
  -> Name, Desc (已 Format), Rarity, Icon, AffixId, bOwned, Stacks
```

---

## 1. 生命周期

| 时机 | 调用方 | API |
|------|--------|-----|
| 开局进 Run | GameMode / 流程 | `BeginRun(FRunStartContext)` |
| 房间清空 / 精英 / Boss 后 | 房间/流程 | `RequestAffixChoice(EAffixOfferSource)` |
| 玩家确认三选一 | 界面 | `ConfirmAffixChoice(ChoiceIndex)` 或 `ConfirmAffix(AffixId)` |
| 跳过（若允许） | 界面 | `SkipAffixChoice()` |
| 结算离局 | 流程 | `EndRun()`（清空局内 Inventory，不碰 Meta） |

`FRunStartContext` 由 `UMetaProgressSubsystem::BuildRunStartContext()` 提供（见 MetaSaveInterface）：永久 Modifier、已解锁 Affix 池、起始武器 Id。

---

## 2. 查询（弹芯每帧/开火前可读）

均 **只读**、无副作用。挂在玩家的 `AC_StatModifiers` 与 Subsystem 结果一致；弹芯优先问组件以减耦合。

| API | 返回 | 说明 |
|-----|------|------|
| `GetStat(EStatId Stat)` | `float` | 已按 Base→Meta→Passive→Temp 汇总 |
| `GetStatBase(EStatId Stat)` | `float` | 未加 Affix 的 Base（含武器 DA） |
| `GetAffixStacks(FName AffixId)` | `int32` | 未拥有为 0 |
| `HasTag(FName Tag)` | `bool` | Inventory 任一词条含 Tag |
| `GetOwnedAffixes()` | `TArray<FAffixRuntimeEntry>` | HUD 图标条 |
| `IsChoicePending()` | `bool` | 三选一打开中 |

**Modifier 顺序（不可改）：** Base → Meta 永久 → 局内 Passive（先 ΣAdd，再 Π(1+Mul)，最后 Override）→ 临时 Duration Buff。

`EStatId` MVP：`MaxHP, MoveSpeed, FireRate, Damage, CritChance, CritMult, MagSize, ReloadSpeed, ProjectileCount, Pierce, Knockback, PickupRadius, CoinGain, ScrapGain`。

**FireRate 语义（与弹芯 CombatFeel 定稿对齐）：**
- 武器 DA 存 **`FireInterval`（秒）**；绝对 Hz **不**进 Stat 汇总、也不再使用 `BaseFireRateHz` 字段名。
- `GetStat(FireRate)` = **乘区**，无词条时 Base **1.0**；Mul 按管线 Π(1+Value)。
- **冷却 / 实际间隔** = `FireInterval / GetStat(FireRate)`。
- RapidFire：Mul **0.12** → GetStat≈1.12 → 间隔约为 `FireInterval/1.12`（约 +12% 射速）。
- 等价写法 `1/(BaseFireRateHz*GetStat)` 仅当 `BaseFireRateHz=1/FireInterval`；工程统一用 **FireInterval**。



**货币：** 局内 Scrap / 局外 Core（见 MetaSaveInterface）。`ScrapGain` 只影响局内。事件 Op 的加减写在 `UEventSubsystem::RunScrap`（见 `EventOps_API.md`），不写本子系统，也不写 Core / `FMetaSaveData`。

---

## 3. 事件（弹芯 / 界面绑定）

| 事件 | Payload | 谁听 |
|------|---------|------|
| `OnAffixChanged` | `FAffixChangeEvent`（Added/Stacked/Removed, AffixId, Stacks） | 弹芯重读 Stat；HUD 刷图标 |
| `OnStatsRebuilt` | 无 / 脏标记 | 弹芯可选，避免每枪扫全表 |
| `OnAffixChoiceOffered` | `TArray<FAffixChoiceEntry>`（恰好 3） | 界面弹 `WBP_AffixChoice` |
| `OnAffixChoiceClosed` | `bConfirmed` | 界面关卡；流程解暂停 |
| `OnAffixTrigger` | `EAffixTrigger, AffixId, optional Context` | 弹芯做 OnKill 等表现钩子 |

```
FAffixChoiceEntry:
  Def: UAffixDefinition*   // 含 Name/Desc/Rarity/Icon
  WeightUsed: float        // 调试
  bWouldBeNew: bool        // 是否首次获得
```

---

## 4. 抽卡

| API | 说明 |
|-----|------|
| `RollChoices(int32 Count = 3, int32 Seed = -1) -> TArray<FAffixChoiceEntry>` | 过滤 Unique 已满、Requires、权重；稀有度乘区来自进度表 |
| `RequestAffixChoice(Source)` | 内部 Roll + 广播 `OnAffixChoiceOffered` + 暂停请求 |
| `ConfirmAffixChoice(int32 Index)` | 合法性检查 → `AddAffix` → `OnAffixChanged` → `OnAffixChoiceClosed(true)` |
| `DebugGrantAffix(FName AffixId)` | 仅 `!UE_BUILD_SHIPPING` |

**不做：** 弹芯不要自己 Roll；房间只调 `RequestAffixChoice`。

---

## 5. 开局注入

```
BeginRun(FRunStartContext Ctx):
  1. ClearInventory()
  2. ApplyMetaModifiers(Ctx.PermanentModifiers)   // 进 Meta 层，不进 Inventory
  3. SetAffixPool(Ctx.UnlockedAffixIds)           // Catalog 过滤
  4. 若 Ctx.StartWeaponId：通知武器系统（弹芯接口，本子系统只传 Id）
  5. RebuildStats() → OnStatsRebuilt / OnAffixChanged(Reset)
```

---

## 6. BP 暴露一览（给蓝图）

**Callable**

- `BeginRun`, `EndRun`
- `RequestAffixChoice`, `ConfirmAffixChoice`, `SkipAffixChoice`, `RollChoices`
- `GetStat`, `GetAffixStacks`, `HasTag`, `GetOwnedAffixes`, `GetAffixCardView`
- `DebugGrantAffix`

**Assign / Bind**

- `OnAffixChanged`, `OnStatsRebuilt`, `OnAffixChoiceOffered`, `OnAffixChoiceClosed`, `OnAffixTrigger`

**Pure**

- `GetStat`, `GetAffixCardView`, `IsChoicePending`, `GetAffixCardViewFromId(AffixId)`

---

## 7. 弹芯最小接入清单

1. 玩家生成后：拿 `AC_StatModifiers`（或 Subsystem），开火/移速/伤害读 `GetStat`。  
2. 绑 `OnAffixChanged` → 刷新缓存的 FireRate/Damage 等。战斗枪只订玩家 `AC_StatModifiers` 上的同名委托，开火冷却 live `GetStat(FireRate)`。  
3. 击杀/受伤时若有 GameplayEvent，转发给 Subsystem 以触发 `OnKill`/`OnTakeDamage` 类词条（系统算 Modifier，表现仍归弹芯）。  
4. **禁止** 在武器 BP 里写死词条 Id 分支（除调试）。

## 8. 界面最小接入清单

1. 听 `OnAffixChoiceOffered` → 对每条 `GetAffixCardView` → 填 `WBP_AffixCard` 的 Name/Desc/Rarity/Icon。  
2. 确认键 → `ConfirmAffixChoice(Index)`。  
3. HUD：`GetOwnedAffixes` 只取 Icon + AffixId。正式壳是 `UPixelHUDWidget`（显示名 `WBP_HUD`），听 `OnAffixChanged`，最多 8 格。不 Roll、不写战斗。挂载见 `Content/Blueprints/UI/README.md`。

---

## 验收

- [ ] 弹芯改枪伤只经 `GetStat(Damage)`，改 DA 数值即生效  
- [ ] 界面三选一仅依赖 Name/Desc/Rarity/Icon 四字段即可出卡  
- [ ] `BeginRun` 后 Meta 加成进 Stat，Inventory 为空  
- [ ] Unique 词条第二次不会进 `RollChoices`

## 9. 正式表锁定（contractVersion **3**）

- **AffixId（MVP 10）：** RapidFire, HollowPoint, ThickSkin, Magnet, Piercer, TwinShot, CritOptics, Adrenaline, GlassCannon, Scavenger  
- CritOptics=0.08；Scavenger=`ScrapGain`；无 ExtendedMag（Post-MVP）  
- **FireRate：** 乘区默认 1.0；间隔=`FireInterval / GetStat(FireRate)`；武器 DA 字段=`FireInterval`（秒）  
- **货币：** Scrap=局内；Core=永久。Economy：ScrapToCore**0.1** / SoftCapPerRun**180** / CoinToScrap0.1；冻名 `DA_Affix_<AffixId>`  
- 正式 JSON：`Content/Data/Affixes/DT_Affix_MVP.json`、`DA_Affix_NameMap.json`、`Content/Data/Meta/DA_Economy.json`、`DT_MetaNodes_MVP.json`

## 10. P0.6 三选一最小闭环（数据 / 系统）

不造 uasset。池子读 `Content/Data/Affixes/DT_Affix_P05.json`（≤8，数值等于 `DT_Affix_MVP.json` contractVersion=3），失败则 CSV，再失败用 C++ `BuiltinP05Rows`。`UAffixSubsystemDefault` 是具体子系统。

**手算（无 Meta、Peashooter Base）：**

- `DebugGrantAffix("RapidFire")` 后 `GetStat(FireRate) = 1 * (1+0.12) = 1.12`
- 冷却 = `FireInterval / 1.12`（Peashooter `0.12/1.12`）
- `HollowPoint` ×1：`GetStat(Damage) = 8 * (1+0.1) = 8.8`

`AC_StatModifiers::GetStat` 在子系统存在时**原样转发**子系统结果，并转发 `OnAffixChanged` / `OnStatsRebuilt`。`AC_AffixInventory` 在确认或 Grant 时写入同一份拥有列表。

**暂停：** `RequestAffixChoice` 调 `ApplyChoicePause(true)`：先广播 `OnAffixPauseRequested(true)`，默认再 `UGameplayStatics::SetGamePaused`。`ConfirmAffixChoice` 反向解开。流程若要自己暂停，调用 `SetAutoPauseDuringChoice(false)` 并听 `OnAffixPauseRequested`（GameMode / PlayerController `SetPause`）。UI 不 `SetGamePaused`、不 Roll。正式壳是 `UPixelAffixChoiceWidget` / `UPixelAffixCardWidget`（显示名 `WBP_AffixChoice` / `WBP_AffixCard`），只绑 `OnAffixChoiceOffered` / `OnAffixChoiceClosed` 与 `GetAffixCardView`，确认 `ConfirmAffixChoice(Index)`。`APixelSideCharacter` 上 `AC_AffixChoicePresenter.bEnableNativeChoiceUI = false`，且 `AffixChoiceWidgetClass` 指向该壳，这样不会和文本占位 `UAffixChoiceDebugWidget` 叠两层。要回文本占位：清空 WidgetClass 并把开关打开。

**Lab（非 Shipping）：** `F6` = `DebugGrantAffix("RapidFire")`（立即约 1.12 射速）；`F7` = 开三选一并暂停。GDD 不允许跳过，所以没有跳过键。
