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
| `GetStat(EStatId Stat)` | `float` | 无武器查询。Base→Meta→Passive→Temp。只计 `WeaponTag` 为空的修饰 |
| `GetStatForWeapon(EStatId Stat, FName WeaponId)` | `float` | 同一套分层。空 Tag 仍计入；非空 Tag 只在与 `WeaponId` 相等时计入。`WeaponId` 为 None 时与 `GetStat` 相同 |
| `GetStatBase(EStatId Stat)` | `float` | 未加 Affix 的 Base（含武器 DA） |
| `GetAffixStacks(FName AffixId)` | `int32` | 未拥有为 0 |
| `HasTag(FName Tag)` | `bool` | Inventory 任一词条含 Tag |
| `GetOwnedAffixes()` | `TArray<FAffixRuntimeEntry>` | HUD 图标条 |
| `IsChoicePending()` | `bool` | 三选一打开中 |

**Modifier 顺序（不可改）：** Base → Meta 永久 → 局内 Passive（先 ΣAdd，再 Π(1+Mul)，最后 Override）→ 临时 Duration Buff。

**`FAffixModifier.WeaponTag`（折算按条过滤，不改上面的层序）：**

- 空（`None` / 省略）= **全局**。每个武器查询，以及不带武器的 `GetStat`，都计入。这是现网无 Tag 修饰的行为。
- 非空 = 只在 `GetStatForWeapon(Stat, WeaponId)` 且 `WeaponId` 的字符串与 Tag **相等**时计入。对不上的那一条不进该层的 Add / Mul / Override。对得上的 Override 仍替换这一层的结果。
- 比较的是 `UDA_Weapon::WeaponId`（豌豆是 `Peashooter`），不是 `DA_Weapon_Peashooter` 资产名，也不是商店合约 Id。
- `AC_WeaponController::ResolveStat` 传入 `GetWeaponId()`。角色 MaxHP / MoveSpeed 仍走不带武器的 `GetStat`，所以一条只标了枪的 Damage Mul 不会改血量。
- 词条行上的 `bRequiresWeaponTag` 是另一字段，本折算不读它。P0.6 CSV 没有 WeaponTag 列，那些行保持全局。JSON 修饰可以带可选 `WeaponTag`。

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

`UMetaProgressSubsystem::BeginRunFromMeta` 传入的上下文还带 `MetaFlags`、`MetaCoreMult`。`BeginRun` 折算修饰的 `Stat` / `Op` / `Value` / `DurationSec` / `WeaponTag` 进 Meta 层。`MetaFlags` 和 `MetaCoreMult` 仍不读。空的 `UnlockedAffixIds` 仍表示当前已加载池；Meta 成功读表时会填上 MVP 十词条（P0.6 的 8 条都在这十个里）。

PeaFamiliar 写入的是 Damage Mul，`WeaponTag=Peashooter`。档 0 不注入。购档后只有 `WeaponId=Peashooter` 的伤害查询吃到该 Mul（一档 ×1.04，二档 ×1.08，三档 ×1.12）。Scatter / Pike 以及其他 `WeaponId` 不受这条 Mul。不带武器的 `GetStat(Damage)` 也不计入它。无 Tag 的 Meta Add（例如 Vitality 的 MaxHP）仍是全局。

---

## 6. BP 暴露一览（给蓝图）

**Callable**

- `BeginRun`, `EndRun`
- `RequestAffixChoice`, `ConfirmAffixChoice`, `SkipAffixChoice`, `RollChoices`
- `GetStat`, `GetStatForWeapon`, `GetAffixStacks`, `HasTag`, `GetOwnedAffixes`, `GetAffixCardView`
- `DebugGrantAffix`

**Assign / Bind**

- `OnAffixChanged`, `OnStatsRebuilt`, `OnAffixChoiceOffered`, `OnAffixChoiceClosed`, `OnAffixTrigger`

**Pure**

- `GetStat`, `GetStatForWeapon`, `GetAffixCardView`, `IsChoicePending`, `GetAffixCardViewFromId(AffixId)`

---

## 7. 弹芯最小接入清单

1. 玩家生成后：拿 `AC_StatModifiers`（或 Subsystem）。移速 / 生命读 `GetStat`。开火伤害、击退、射速读 `GetStatForWeapon(Stat, WeaponId)`。  
2. 绑 `OnAffixChanged` → 刷新缓存的 FireRate/Damage 等。战斗枪只订玩家 `AC_StatModifiers` 上的同名委托，开火冷却 live `GetStatForWeapon(FireRate, WeaponId)`。  
3. 击杀/受伤时若有 GameplayEvent，转发给 Subsystem 以触发 `OnKill`/`OnTakeDamage` 类词条（系统算 Modifier，表现仍归弹芯）。  
4. **禁止** 在武器 BP 里写死词条 Id 分支（除调试）。

## 8. 界面最小接入清单

1. 听 `OnAffixChoiceOffered` → 对每条 `GetAffixCardView` → 填 `WBP_AffixCard` 的 Name/Desc/Rarity/Icon。  
2. 确认键 → `ConfirmAffixChoice(Index)`。  
3. HUD：`GetOwnedAffixes` 只取 Icon + AffixId。正式壳是 `UPixelHUDWidget`（显示名 `WBP_HUD`），听 `OnAffixChanged`，最多 8 格。不 Roll、不写战斗。挂载见 `Content/Blueprints/UI/README.md`。

---

## 验收

- [ ] 弹芯改枪伤只经 `GetStatForWeapon(Damage, WeaponId)`，改 DA 数值即生效。带 `WeaponTag` 的修饰只打在匹配的 `WeaponId` 上  
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
- `HollowPoint` ×1：`GetStat(Damage) = 8 * (1+0.1) = 8.8`（无 Tag，Peashooter 与其他枪同一乘区）
- PeaFamiliar 一档：`GetStatForWeapon(Damage, "Peashooter") = 8 * 1.04 = 8.32`；`GetStat(Damage)` 与 `GetStatForWeapon(Damage, "Scatter")` 仍是 8。档 0 不注入

`AC_StatModifiers::GetStat` 在子系统存在时**原样转发**子系统结果，并转发 `OnAffixChanged` / `OnStatsRebuilt`。`AC_AffixInventory` 在确认或 Grant 时写入同一份拥有列表。

**暂停：** `RequestAffixChoice` 调 `ApplyChoicePause(true)`：先广播 `OnAffixPauseRequested(true)`，默认再 `UGameplayStatics::SetGamePaused`。`ConfirmAffixChoice` 反向解开。流程若要自己暂停，调用 `SetAutoPauseDuringChoice(false)` 并听 `OnAffixPauseRequested`（GameMode / PlayerController `SetPause`）。UI 不 `SetGamePaused`、不 Roll。正式壳是 `UPixelAffixChoiceWidget` / `UPixelAffixCardWidget`（显示名 `WBP_AffixChoice` / `WBP_AffixCard`），只绑 `OnAffixChoiceOffered` / `OnAffixChoiceClosed` 与 `GetAffixCardView`，确认 `ConfirmAffixChoice(Index)`。`APixelSideCharacter` 上 `AC_AffixChoicePresenter.bEnableNativeChoiceUI = false`，且 `AffixChoiceWidgetClass` 指向该壳，这样不会和文本占位 `UAffixChoiceDebugWidget` 叠两层。要回文本占位：清空 WidgetClass 并把开关打开。

**Lab（非 Shipping）：** `F6` = `DebugGrantAffix("RapidFire")`（立即约 1.12 射速）；`F7` = 开三选一并暂停。GDD 不允许跳过，所以没有跳过键。`F8` / `F9` 是事件 Op 直调 `ResolveEventChoice`，不是本界面。`F10` 才打开 `WBP_EventChoice`（AbandonedCache），不调用 `RequestAffixChoice`，也不 `SetGamePaused`。确认选项由壳调用一次 `ResolveEventChoice`；Skip 不调用。`F11` 是商店。事件界面开着时 `F6` / `F7` 先被忽略，关掉之后两个键与原来一样。
