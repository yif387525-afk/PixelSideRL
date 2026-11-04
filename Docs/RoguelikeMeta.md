# RoguelikeMeta — 词条 / 三选一 / 商店 / 永久成长（MVP）

**Owner:** 织局  
**协作:** 词条官（实现）  
**对齐:** `UE_PROJECT_SPEC.md`、`Docs/GDD.md`、`Docs/AffixBuffPipeline.md`、`Docs/MetaSaveInterface.md`

**契约声明**
- 字段名沿用 `Docs/AffixBuffPipeline.md`、`Content/Data/Affixes/SCHEMA_AffixDefinition.json`；**系统不改字段名**。
- `AffixId` 用 **PascalCase**，与 `Content/Data/Affixes/DT_Affix_MVP.json` 对齐。
- `StackRule` = `Unique` | `AdditiveStack` | `RefreshDuration`（不用 `bUnique` 布尔为主；`Unique` ⇔ `StackRule == Unique`）。
- 开局 **10 词条全进池**；禁止用 Meta `UnlockAffix` 解锁 `Piercer` / `TwinShot`（二者开局即在池内）。

---

## 1. 词条（Affix）管线

### 1.1 稀有度与堆叠

```
E_AffixRarity:     Common | Rare | Epic | Legendary   // MVP 用前三级
E_AffixStackRule:  Unique | AdditiveStack | RefreshDuration
```

| StackRule | 行为 |
|-----------|------|
| `Unique` | 池中最多持有 1；已拥有则抽卡排除（⇔ 旧文档 `bUnique`） |
| `AdditiveStack` | 可重复拿，层数线性叠至 `MaxStacks` |
| `RefreshDuration` | 再拿刷新 `DurationSec`，不叠层数值 |

### 1.2 流程

1. 触发点见 GDD §2.5（清房 / Boss / 商店券）。
2. 暂停战斗 → `WBP_AffixChoice`（3 张；UI 读 AffixCard 契约：Name←DisplayName，Desc←Description，Rarity，Icon）。
3. 选 1 → `AC_AffixInventory::AddAffix` → `UAffixSubsystem` 重算 Modifier → 恢复战斗。
4. **失败态：** UI 打开期间禁止移动开火；战斗房三选一不可跳过；商店券确认购买后必须选完。

### 1.3 `UAffixDefinition` 字段（只列契约，不改名）

`AffixId`, `DisplayName`, `Description`, `Rarity`, `Icon`, `StackRule`, `MaxStacks`, `Weight`, `Tags`, `ExclusiveTags`, `Triggers`, `Modifiers[]`（`Stat`/`Op`/`Value`/`DurationSec`）, 可选 `GrantWeapon`, `bRequiresWeaponTag`。

---

## 2. MVP 词条表（10 条 — 可回填 JSON）

字段列与 stub / SCHEMA 一一对应。Mul 语义与管线一致：`0.12` = +12%。

| AffixId | DisplayName | Rarity | StackRule | MaxStacks | Weight | Tags | ExclusiveTags | Triggers | Modifiers | Description |
|---------|-------------|--------|-----------|-----------|--------|------|---------------|----------|-----------|-------------|
| `RapidFire` | 速射 | Common | AdditiveStack | 3 | 10 | Offense, Weapon | [] | Passive | FireRate Mul **0.12** | 射速 +{Value} |
| `HollowPoint` | 空尖弹 | Common | AdditiveStack | 3 | 10 | Offense | [] | Passive | Damage Mul **0.1** | 伤害 +{Value} |
| `ThickSkin` | 厚皮 | Common | AdditiveStack | 5 | 10 | Defense | [] | Passive | MaxHP Add **10** | 最大生命 +{Value} |
| `Magnet` | 磁吸 | Common | Unique | 1 | 8 | Economy | [] | Passive | PickupRadius Add **48** | 拾取半径 +{Value} |
| `Piercer` | 穿甲 | Rare | AdditiveStack | 2 | 5 | Offense, Weapon | [] | Passive | Pierce Add **1** | 穿透 +1 |
| `TwinShot` | 双生弹 | Rare | Unique | 1 | 4 | Offense, Weapon | ProjectileShape | Passive | ProjectileCount Add **1** | 弹数 +1 |
| `CritOptics` | 暴击瞄具 | Rare | AdditiveStack | 3 | 5 | Offense | [] | Passive | CritChance Add **0.08** | 暴击率 +{Value} |
| `Adrenaline` | 肾上腺素 | Epic | RefreshDuration | 1 | 3 | Mobility, OnKill | [] | OnKill | MoveSpeed Mul **0.25**, DurationSec **2.5** | 击杀后短时移速提升 |
| `GlassCannon` | 玻璃大炮 | Epic | Unique | 1 | 2 | Offense, Defense | GlassCannon | Passive | Damage Mul **0.35**；MaxHP Mul **-0.25** | 伤害大幅提升，最大生命降低 |
| `Scavenger` | 拾荒者 | Common | AdditiveStack | 3 | 8 | Economy | [] | Passive | **ScrapGain** Mul **0.15** | Scrap 获取 +{Value} |

**Scavenger 运行时注：** 若 `E_StatId` 暂无 `ScrapGain`，实现可映射 `CoinGain`；回填 JSON 时用 note 标明 `ScrapGain → CoinGain`，**勿另起 AffixId**。

**DA 命名（冻）：** `DA_Affix_<AffixId>`，与 AffixId 一一对应，例如 `DA_Affix_RapidFire` … `DA_Affix_Scavenger`。禁止另起展示名作资产名。`contractVersion=3`。

### 2.1 相对 `DT_Affix_MVP.json` 差异清单

| 项 | stub | 本定稿 |
|----|------|--------|
| `CritOptics.Modifiers[0].Value` | 0.05 | **0.08** |
| `Scavenger` Stat / Desc | `CoinGain` /「金币收益」 | 建议 **`ScrapGain`** /「Scrap 获取」；运行时可映射 CoinGain |
| 其余 AffixId / 数值 / Stack / Weight / Tags | — | **全保留**（含 `Magnet`） |

### 2.2 Post-MVP 候补（不进 MVP 必做）

| AffixId | 短述 |
|---------|------|
| （已在池）`Magnet` | MVP 已保留；候补栏不再重复立项 |
| `ExtendedMag` | 弹匣容量提升（曾为 MVP 候选，现降级） |
| `ReflectShield` | 护甲/受击时小概率反弹敌弹 |
| `QuakeLanding` | 落地近距小额伤害 / 硬直 |

---

## 3. 构筑原型

三选一权重 **轻度** 向已有 Tag 倾斜（同 Tag **+10%**），不强制单构筑。

### 3.1 弹幕热管

| 项 | 内容 |
|----|------|
| 推荐武器 | Peashooter |
| 核心词条 | `RapidFire` / `TwinShot` / `Adrenaline`（可辅 `ThickSkin`） |
| 手感 | 走射清杂、击杀加速风筝 |

### 3.2 散弹破盾

| 项 | 内容 |
|----|------|
| 推荐武器 | Scattergun |
| 核心词条 | `TwinShot` / `HollowPoint` / `GlassCannon`（可辅 `ThickSkin`） |
| 手感 | 贴脸爆发克盾；玻璃大炮换输出空间 |

### 3.3 轨道刺客

| 项 | 内容 |
|----|------|
| 推荐武器 | Rail Pierce |
| 核心词条 | `Piercer` / `CritOptics` / `HollowPoint`（可辅 `GlassCannon`） |
| 手感 | 预判穿透点杀、暴击窗口补枪 |

---

## 4. 三选一生成算法

### 4.1 稀有度偏置 `AffixRarityBias`（Common / Rare / Epic）

| 房间类型 | Common | Rare | Epic |
|----------|--------|------|------|
| 普通战斗 | 70% | 25% | 5% |
| 精英 | 50% | 35% | 15% |
| Boss 后 | 30% | 45% | 25% |

可由 `DT_DifficultyCurve` / Catalog 倍率覆盖；系统只读。

### 4.2 伪代码

```
function RollAffixChoice(RoomType, PlayerInv, RunState):
  bias = AffixRarityBias[RoomType]
  pool = Catalog.All
           .Filter(UnlockFilter)          // MVP: 10 条全开
           .Filter(a => !(a.StackRule == Unique && PlayerInv.Has(a.AffixId)))
           .Filter(a => !ExclusiveTagConflict(a, PlayerInv))

  picks = []
  for i in 1..3:
    for a in pool where a.AffixId not in picks:
      tagBoost = 1.0 + 0.1 * PlayerInv.CountTagsOverlap(a.Tags)
      w = a.Weight * BaseRarityWeight(a.Rarity, bias) * tagBoost
    picks.append(WeightedSample(...))

  // 保底：连续 5 次三选一无 Rare+ → 第 6 次至少 1 Rare
  if RunState.StreakNoRarePlus >= 5:
    EnsureAtLeastOne(picks, rarity >= Rare)

  return picks  // 无重复 AffixId
```

**失败态：** 池不足 3 → 回退可重复的 `AdditiveStack` Common；打日志。

---

## 5. 商店

槽位 3：① 武器临时合约（本局）② 回复 ③ 三选一券。只花**局内 Scrap**；可跳过。

| 商品 | 价格（Scrap） |
|------|----------------|
| 回复（约 40% MaxHP） | 30 |
| 三选一券 | 60 |
| 武器合约 Peashooter / Scattergun / Rail | 45 / 60 / 80 |

层2 建议价格 ×1.15（`Content/Data/Shop/DA_ShopPriceCurve.json`，`resolvedScrap = baseScrap × 1.15`，不另起整数）。  
**失败态：** Scrap 不足灰显；券购买确认后立即弹出三选一。

数据：`Content/Data/Shop/DA_ShopInventory.json`（Slot_1=武器合约池，Slot_2=回复，Slot_3=三选一券；锚点对齐 Room_Shop_01）、`DA_ShopPriceCurve.json`。策划改 JSON，禁止占位 uasset。

### 5.1 事件房（非商店）

支路 Event 不花 Scrap、不卖券。MVP 默认 `DA_Event_AbandonedCache`（GDD §2.8）：`CarefulLoot`=+25 Scrap；`ForceLoot`=−15 HP +50 Scrap；跳过无奖。数据：`Content/Data/Events/`。

---

## 6. 经济定稿 `DA_Economy`

路径：`Content/Data/Meta/DA_Economy`（参数意图由此文档锁定；JSON stub 由词条官回填）。

| 字段 | 定稿值（contractVersion=3，已锁） |
|------|--------|
| `RunCurrencyId` | Scrap |
| `MetaCurrencyId` | Core |
| `BaseClearBonus` | 40 |
| `DeathMult` | 0.5 |
| `PerRoom` | 8 |
| `BossBonus` | 55 |
| `CoinToScrap` | 0.1 |
| `ScrapToCore` | 0.1 |
| `MetaCoreMult` | 1.0 |
| `SoftCapPerRun` | 180 |

**货币语义（主管裁定）**
- **Scrap：** 局内通货（掉落、商店）。拾取可表现为 Coin，进局内 Scrap 钱包。
- **Core：** 永久货币；结算入账、Hub 节点消费。存档字段目标为 `FMetaSaveData.Core`（若实现仍暂用 Scrap 字段，须迁到 Core，文档层一律称 Core）。

**结算公式**

```
Core = floor(
    ( BaseClearBonus * (Cleared ? 1.0 : DeathMult)
    + RoomsCleared * PerRoom
    + BossKilled * BossBonus
    + RunScrapRemain * ScrapToCore
    ) * MetaCoreMult
)
Core = min(Core, SoftCapPerRun)   // 单局入账软顶
```

> 与 `DA_Economy_MVP.json` contractVersion=2 对齐：Coin→局内 Scrap（`CoinToScrap`）；局内剩余 Scrap→Core（`ScrapToCore=1.0`）；单局 Core 软顶 `SoftCapPerRun`。

---

## 7. 永久成长（Meta）5 节点

货币：永久 **Core**。开局 10 词条**全进池**。  
**禁止：** 永久直接加 CritChance / Pierce / ProjectileCount；**禁止** `UnlockAffix` 解锁 Piercer/TwinShot。

| NodeId | 效果（按购买次数 1→Max） | CostCore / `CostCoreByRank`（递增） | MaxPurchases |
|--------|--------------------------|------------------|--------------|
| `Vitality` | 开局 MaxHP **+10 / +20 / +30** | 25 / 50 / 90 | 3 |
| `Stash` | 开局局内 Scrap **+15 / +30 / +50** | 20 / 40 / 75 | 3 |
| `PeaFamiliar` | Peashooter Damage Mul **+0.04 / +0.08 / +0.12** | 30 / 60 / 100 | 3 |
| `SalvageSense` | `MetaCoreMult` **+0.05 / +0.10 / +0.15** | 35 / 70 / 110 | 3 |
| `SidearmLuck` | 层1 房2 后保底武器房（规则开关） | 45 | 1 |

应用：仅 `BuildRunStartContext` / 结算倍率；**不**改词条池公式与 `AffixRarityBias`。

---

## 8. 数据与蓝图挂钩

| 路径 | 用途 |
|------|------|
| `Docs/AffixBuffPipeline.md` | Modifier 栈、StackRule、查询 API |
| `Docs/MetaSaveInterface.md` | `FMetaSaveData.Core`、结算槽、Hub API |
| `Content/Data/Affixes/SCHEMA_AffixDefinition.json` | 字段契约 |
| `Content/Data/Affixes/DT_Affix_MVP.json` | AffixId 对齐源；定稿后回填数值 |
| `Content/Data/Affixes/DA_Affix_*` | 运行时真相源 |
| `Content/Data/Meta/DA_Economy.json` | 结算参数（已锁 v3） |
| `Content/Data/Meta/DA_MetaNode_*` / `DA_MetaTree` | 永久节点 |
| `Content/Data/Shop/DA_ShopInventory.json` / `DA_ShopPriceCurve.json` | 商店三槽与层价格（§5）；无占位 uasset |
| `Content/Blueprints/Roguelike/` | `AC_AffixInventory`、三选一、商店 |
| `Content/Blueprints/Meta/` | Hub 消费与存档 |
| `Content/Blueprints/UI/` | `WBP_AffixChoice`、`WBP_Shop`、`WBP_RunSummary` |

---


---

## 9.5 近端：三选一最小闭环（P0.6）

**前置：** P0.5 Grunt 可击杀（或 Debug「假清房」事件）。  
**范围：** 不做商店券、不做 Boss 高稀有偏置 UI；稀有度可先固定普通房 bias。

| 步骤 | 要求 |
|------|------|
| 触发 | `UAffixSubsystem::RollChoices(3)`；数据源 `DT_Affix_MVP` / `DA_Affix_*` |
| UI | `WBP_AffixChoice`：3 卡、稀有度色边、选 1 必选 |
| 应用 | `AC_AffixInventory::AddAffix` → 重算 Modifier → 广播 |
| 可感知验收 | 选 `RapidFire` 后 Peashooter 射速 ↑（对照 `CombatFeel` FireInterval/GetStat） |
| 失败态 | 未选不可关；池空回退 Common 可重复条并打日志 |

正式里程碑顺序见 `GDD.md` §6.5（P0.5→P0.6→P0.7→P0.8）。

## 9. 验收

### 近端（P0.6）优先勾

- [x] Debug 或清房可弹出三选一（F7 / RequestAffixChoice；主机测过）
- [x] **P0.6 运行池 8 Id**（`DT_Affix_P05`）：无 Adrenaline/GlassCannon；正式锁表仍 10
- [x] `RapidFire` 选中后射速乘区生效（GetStat FireRate≈1.12）
- [x] 池内 Unique：`Magnet` / `TwinShot` 不重复（`GlassCannon` 留正式 MVP）

- [x] 正式 10 个 PascalCase AffixId 已锁表（`DT_Affix_MVP` cv3）；P0.6 抽卡读八条子集
- [ ] `Magnet` 保留在 MVP 池；无另起 Id
- [ ] `CritOptics` CritChance = **0.08**；`Scavenger` 文档为 ScrapGain（或 note 映射 CoinGain）
- [ ] `Unique`（Magnet / TwinShot / GlassCannon）不会第二次出现
- [ ] ExclusiveTags（`ProjectileShape` / `GlassCannon`）冲突正确排除
- [ ] 保底：连续 5 次无 Rare+ → 第 6 次至少 1 Rare
- [ ] 三种构筑在自定义种子下 1 局内可成型
- [ ] Meta 5 节点只影响开局 / `MetaCoreMult`；不改词条池；无 UnlockAffix→Piercer/TwinShot
- [ ] `DA_Economy` 定稿系数与公式入账 `FMetaSaveData.Core`，受 SoftCapPerRun
- [ ] 商店三槽价格可配；局内 Scrap 不足不可买；Hub 花 Core

---

## 变更记录

| 日期 | 作者 | 说明 |
|------|------|------|
| 2026-11-04 | 织局 | 首版：10 AffixId、构筑、经济 Core、Meta 五节点 |
| 2026-11-04 | 织局 / 词条官 | 对齐 contractVersion=3；冻 `DA_Affix_<AffixId>` |
| 2026-11-05 | 织局 | 签收手感本版；补 P0.6 三选一最小闭环定义与近端验收；顺序服从 GDD §6.5 |
| 2026-11-05 | 织局 | P0.6 策划签收勾近端项；Event 指向 `DA_Event_AbandonedCache` |
| 2026-11-05 | 词条官 | 商店锁表：`Content/Data/Shop/DA_ShopInventory.json`、`DA_ShopPriceCurve.json`（无 uasset） |
