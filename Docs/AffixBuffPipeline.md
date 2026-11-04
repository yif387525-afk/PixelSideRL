# 词条 / Buff 管线（系统）

**Owner:** 词条官  
**协作:** 织局（内容与数值意图）· 弹芯（读取 Modifier 应用到战斗）· 界面（三选一读表展示）  
**原则:** DataAsset 为唯一真相源；蓝图 / 战斗只读 Modifier，不写死词条逻辑。  
**货币:** 局内 Scrap / 局外 Core。`FireRate` = 乘区默认 1.0；间隔=FireInterval/GetStat(FireRate)（见 AffixSubsystem_API）。

## 目标（MVP）

- 局内获得 8–12 个词条中的若干，可叠加（同 Id 按规则叠层）。
- 房间结算 / 精英掉落 / Boss 后触发 **三选一**（`WBP_AffixChoice`）。
- 所有数值改动经 **Modifier 栈** 汇总后交给战斗侧查询。

## 运行时组件

| 资产 | 路径建议 | 职责 |
|------|----------|------|
| `UAffixDefinition` (DataAsset) | `Content/Data/Affixes/DA_Affix_*` | 词条静态定义 |
| `UAffixCatalog` (DataAsset) | `Content/Data/Affixes/DA_AffixCatalog` | MVP 池、权重、互斥 |
| `AC_AffixInventory` | `Content/Blueprints/Roguelike/` | 局内已选词条与叠层 |
| `UAffixSubsystem` (GameInstance) | `Source/.../Affix` 或 BP 子系统 | 抽卡、应用、查询 Modifier |
| `AC_StatModifiers` | 挂玩家 | 聚合后的战斗可读数值 |

## 核心类型

```
E_AffixRarity: Common | Rare | Epic | Legendary
E_AffixStackRule: Unique | AdditiveStack | RefreshDuration
E_ModifierOp: Add | Mul | Override
E_StatId: MaxHP | MoveSpeed | FireRate | Damage | CritChance | CritMult
         | MagSize | ReloadSpeed | ProjectileCount | Pierce | Knockback
         | PickupRadius | CoinGain | ...
E_AffixTrigger: OnAcquire | OnHit | OnKill | OnRoomClear | OnTakeDamage | Passive
```

### `FAffixModifier`

| 字段 | 类型 | 说明 |
|------|------|------|
| Stat | `E_StatId` | 目标属性 |
| Op | `E_ModifierOp` | Add / Mul / Override |
| Value | float | 数值（Mul 用 0.1 = +10%） |
| DurationSec | float | ≤0 表示永久（局内） |

### `UAffixDefinition` (`DA_Affix_*`)

| 字段 | 类型 | 说明 |
|------|------|------|
| AffixId | `FName` | 稳定 Id，存档与互斥用 |
| DisplayName | `FText` | UI |
| Description | `FText` | 短描述，可含 `{Value}` |
| Rarity | `E_AffixRarity` | |
| Icon | SoftObjectPath | UI |
| StackRule | `E_AffixStackRule` | |
| MaxStacks | int32 | AdditiveStack 上限 |
| Weight | float | 抽卡权重（可被 Catalog 覆盖） |
| Tags | `TArray<FName>` | 如 `Weapon`, `Defense`, `Economy` |
| ExclusiveTags | `TArray<FName>` | 互斥组 |
| Triggers | `TArray<E_AffixTrigger>` | |
| Modifiers | `TArray<FAffixModifier>` | Passive 主路径 |
| GrantWeapon | Soft `DA_Weapon_*` | 可选：解锁/替换枪 |
| bRequiresWeaponTag | `FName` | 可选：仅某枪生效 |

### AffixCard 展示契约（冻结 v1）

界面 `WBP_AffixCard` 稳定字段：**Name**←`DisplayName`，**Desc**←`Description`，**Rarity**，**Icon**。  
详见 [AffixSubsystem_API.md](./AffixSubsystem_API.md) §0。完整 BP 接口同文档。

## 应用顺序（固定）

1. **Base**（角色 / 当前武器 DataAsset）  
2. **Meta 永久加成**（开局一次性注入，见 MetaSaveInterface）  
3. **局内 Affix Passive Modifiers**（按获得顺序；同 Stat：先 Add 求和，再乘积 Mul，最后 Override）  
4. **临时 Buff**（Duration > 0，独立栈，过期移除）

查询 API（弹芯只调这些）：

- `GetStat(E_StatId) -> float`  
- `GetAffixStacks(FName AffixId) -> int32`  
- `HasTag(FName) -> bool`

## 三选一抽卡

1. `UAffixSubsystem::RollChoices(Count=3, Seed)`  
2. 从 `DA_AffixCatalog` 过滤：未 Unique 占满、满足 Requires、权重 > 0  
3. 稀有度：房间进度表（织局填曲线；系统只读权重倍率）  
4. 玩家确认 → `AC_AffixInventory::AddAffix(DA)` → 重算 Modifier → 广播 `OnAffixChanged`

## 与战斗边界

- **不做：** 后坐力曲线、弹道散射表现、受击闪白（弹芯 / CombatFeel）。  
- **做：** 把 `FireRate`/`Damage`/`ProjectileCount` 等数交给 `AC_WeaponController` 读取。  
- 特殊触发（OnKill 掉弹等）用 `E_AffixTrigger` + GameplayEvent，具体表现由弹芯订接口，系统只发事件与参数。

## 验收（第 3–4 周）

- [ ] 编辑器内改 `DA_Affix_*` 数值，进 `Run_Test` 无需改蓝图逻辑即可生效  
- [ ] 三选一可重复开，Unique 词条不会二次出现  
- [ ] `GetStat` 与手算表一致（`Scripts/test_affix_p05.cpp`；RapidFire → FireRate 1.12，HollowPoint → Damage `Base*(1+0.1)`）
- P0.6：`UAffixSubsystemDefault` + `AC_AffixInventory` + `AC_StatModifiers::GetStat` 转发；见 [AffixSubsystem_API.md](./AffixSubsystem_API.md) §10
