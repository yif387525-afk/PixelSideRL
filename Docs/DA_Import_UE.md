# JSON → UE DataAsset 导入说明（一页）

**状态：** contractVersion=3 正式表已验收；等待 Git/UE 仓库后执行。  
**Owner：** 词条官  
**真相源：** `Content/Data/Affixes/DT_Affix_MVP.json`、`Content/Data/Meta/DA_Economy.json`、`DT_MetaNodes_MVP.json`

---

## 1. 冻名（禁止另起）

| JSON | UE 资产名 |
|------|-----------|
| AffixId `X` | `DA_Affix_X`（见 `DA_Affix_NameMap.json`） |
| NodeId `Y` | `DA_MetaNode_Y` |
| 经济 | `DA_Economy` |
| 稀有度权重 | `DA_AffixRarityBias` 或 `DT_AffixRarityBias` |

路径：`Content/Data/Affixes/`、`Content/Data/Meta/`。

---

## 2. 建议 C++ / BP 类型

1. `UAffixDefinition : UPrimaryDataAsset`（字段对齐 `SCHEMA_AffixDefinition.json` + AffixCard：DisplayName/Description/Rarity/Icon）  
2. `UMetaNodeDefinition : UPrimaryDataAsset`（含 `CostCoreByRank`）  
3. `UEconomySettings : UDataAsset`（或 `UPrimaryDataAsset`）  
4. 可选：`UAffixCatalog` 引用 10 个 `DA_Affix_*`（开局全进池）

头文件契约：`Source/PixelSideRL/Public/Affix/AffixTypes.h`、`AffixSubsystem.h`。

---

## 3. 导入步骤（仓库就绪后）

1. 引擎建好 `UAffixDefinition` 等类并编译。  
2. **Editor Utility / Python：** 读 `DT_Affix_MVP.json` → 对每个 affix `NewObject` / Factory 创建 `DA_Affix_<Id>`，写字段；`Icon` 可空。  
3. 同样导入 5 个 `DA_MetaNode_*` 与 `DA_Economy`。  
4. 跑 `Scripts/validate_affix_stub.py` 的正式版校验（或扩成读 `DT_Affix_MVP.json`）。  
5. 提交：JSON（已有）+ 生成的 `uasset`（LFS）+ Docs 一行。

**不要**手改 uasset 数值而不回写 JSON；MVP 以 JSON 为策划编辑入口，导入脚本可重复跑（幂等按 AffixId upsert）。

---

## 4. 字段映射速查

| JSON | UE |
|------|-----|
| `DisplayName` / `Description` | `FText`（AffixCard Name/Desc） |
| `Rarity` | `EAffixRarity` |
| `Modifiers[].Op` Mul `0.12` | 管线按 Π(1+Value)；FireRate 乘区默认 1.0 |
| `CostCoreByRank` | `TArray<int32>` |
| `ScrapToCore` / `SoftCapPerRun` | `DA_Economy` float/int |

冷却（弹芯）：`FireInterval / GetStat(FireRate)`。

---

## 5. Deprecated

`*_stub.json`、`DA_Economy_MVP.json` 已标 `deprecated` → redirect 正式文件。新工作勿再读 stub。
