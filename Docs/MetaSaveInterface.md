# 局外永久成长与存档接口（系统）

**Owner:** 词条官  
**协作:** 织局（节点树与经济意图）· 界面（Hub / 结算 UI）· 弹芯（开局读 Meta 注入后的 Base）

## 货币约定（主管锁定）

| 名称 | 范围 | 用途 |
|------|------|------|
| **Scrap** | 仅局内 | 商店消费、局内掉落；开局可由 Meta（如 Stash）注入 |
| **Core** | 仅局外 | 结算入账、Hub 永久节点消费；存档字段 |

禁止再用 Scrap 指代局外钱包。

## 范围（MVP）

- 局外货币 **Core**：结算发放，Hub 消费。  
- 少量永久节点（建议 ≤12）：基础属性、开局 Scrap、武器熟悉度等；**MVP 词条开局全进池**（不做 Pool 解锁节点）。  
- 本地存档：单机槽位 1（可扩 3）；无联机。

## 数据资产

| 资产 | 路径 | 职责 |
|------|------|------|
| `DA_MetaNode_*` | `Content/Data/Meta/` | 永久节点定义 |
| `DA_MetaTree` | `Content/Data/Meta/` | 节点图、前置、价格（**CostCore**） |
| `DA_Economy` | `Content/Data/Meta/` | 结算 **Core** 公式参数 |

### `UMetaNodeDefinition`

| 字段 | 类型 | 说明 |
|------|------|------|
| NodeId | `FName` | |
| DisplayName / Description | `FText` | |
| CostCore | int32 | 单级价；多级用 `CostCoreByRank` |
| Prerequisites | `TArray<FName>` | |
| UnlockAffixIds | `TArray<FName>` | MVP 通常为空（全池已开） |
| PermanentModifiers | `TArray<FAffixModifier>` | 开局注入 Base 之后一层 |
| StartingScrap | int32 | 可选：开局注入局内 Scrap |
| UnlockWeapon | Soft `DA_Weapon_*` | 可选 |
| MaxPurchases | int32 | 默认 1；可重复买的用 >1 |

## 运行时

| 对象 | 职责 |
|------|------|
| `UMetaProgressSubsystem` | 持有已购节点、**Core**；读写存档 |
| `FMetaSaveData` | 可序列化结构 |
| `BP_Hub` / `Hub.umap` | 只调子系统 API，不写存档细节 |

### `FMetaSaveData`（版本化）

```
SaveVersion: int32 = 1
Core: int32
PurchasedNodes: TMap<FName, int32>   # NodeId -> 购买次数/等级
UnlockedWeapons: TArray<FName>
Stats: RunsStarted, RunsCleared, TotalKills, BestDepth
LastRunSummary: optional（上次结算展示用）
```

路径建议：`Saved/SaveGames/PixelSideRL_Meta.sav`（`UGameplayStatics::SaveGameToSlot`）。

## API

```
GetCore() / AddCore(Delta, Reason)
CanPurchase(NodeId) / Purchase(NodeId) -> bool
GetUnlockedAffixPool() -> TArray<UAffixDefinition*>
GetPermanentModifiers() -> TArray<FAffixModifier>
BuildRunStartContext() -> FRunStartContext   # 含 StartingScrap、PermanentModifiers
Save() / Load() / ResetSlot()
```

### 结算发放（系统侧公式槽，参数来自 `DA_Economy`）

```
CoreGained = floor(
    BaseClearBonus * (Cleared ? 1 : DeathMult)
  + RoomsCleared * PerRoom
  + BossKilled * BossBonus
  + RunScrapCollected * ScrapToCore   // 定稿 ScrapToCore=0.1
) * MetaCoreMult
```

夹取：`SoftCapPerRun`（180）。局内 `Coin` 拾取 → 先按 `CoinToScrap` 进局内 Scrap；**不**直接改 Core。

## 开局注入顺序

1. Load Meta（读 **Core** 与节点）  
2. `BuildRunStartContext` → PermanentModifiers + StartingScrap + Affix 池  
3. 进 Run；局内 Inventory 空；局内钱包 Scrap 从 StartingScrap 起  
4. 结算 → `AddCore` → Save → 回 Hub

## 验收

- [ ] 杀进程重开，**Core** 与已购节点保留  
- [ ] Hub 消费只扣 Core；局内商店只扣 Scrap  
- [ ] `ResetSlot` 后与新档一致  
- [ ] SaveVersion 升级时可迁移（`Scrap` 存档字段 → `Core`）
