# JSON → UE DataAsset 导入说明（一页）

**状态：** contractVersion=3 正式表已验收；等待 Git/UE 仓库后执行。  
**Owner：** 词条官  
**真相源：** `Content/Data/Affixes/DT_Affix_MVP.json`、`Content/Data/Meta/DA_Economy.json`、`DT_MetaNodes_MVP.json`、`Content/Data/Shop/DA_ShopInventory.json`、`DA_ShopPriceCurve.json`

---

## 1. 冻名（禁止另起）

| JSON | UE 资产名 |
|------|-----------|
| AffixId `X` | `DA_Affix_X`（见 `DA_Affix_NameMap.json`） |
| NodeId `Y` | `DA_MetaNode_Y` |
| 经济 | `DA_Economy` |
| 稀有度权重 | `DA_AffixRarityBias` 或 `DT_AffixRarityBias` |
| `Content/Data/Shop/DA_ShopInventory.json` | `DA_ShopInventory` |
| `Content/Data/Shop/DA_ShopPriceCurve.json` | `DA_ShopPriceCurve` |

路径：`Content/Data/Affixes/`、`Content/Data/Meta/`、`Content/Data/Shop/`。

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

## 6. P0.6 子集（无 uasset）

`DT_Affix_P05.json` / `DT_Affix_P05.csv` 是正式锁表的 8 条子集，冻名仍是 `DA_Affix_<AffixId>`。编辑器没导入 uasset 时，`UAffixSubsystemDefault` 读 JSON，否则 CSV，否则 `UAffixDefinition::ApplyBuiltinP05` / `BuiltinP05Rows`。不要提交占位 `.uasset`。

## 7. 商店冻名（contractVersion=3）

策划入口是 JSON，同名 `.csv` 只做表审镜像。`contractVersion=3` 与 Affix / `DA_Economy` 正式锁表同一代，不另设 `shopContractVersion`。货币字段只有局内 `Scrap`（`currencyScope=Run`），禁止写成 Core。

| JSON | UE 资产名 | 契约 |
|------|-----------|------|
| `Content/Data/Shop/DA_ShopInventory.json` | `DA_ShopInventory` | `SCHEMA_ShopInventory.json` |
| `Content/Data/Shop/DA_ShopPriceCurve.json` | `DA_ShopPriceCurve` | `SCHEMA_ShopPriceCurve.json` |

默认货位（可改字段，本锁表固定）：`Slot_1` 武器合约池（`Peashooter` / `Scattergun` / `RailPierce`）、`Slot_2` 回复（约 40% MaxHP）、`Slot_3` 三选一券。锚点对齐 `Room_Shop_01`，不在本表改几何。层 1 标价见 RoguelikeMeta §5；层 2 `resolvedScrap = baseScrap * 1.15`，`rounding=None`。

校验：`python3 Scripts/validate_shop_da.py`。

**不要**提交占位 `.uasset`。本表不新增 C++：`DA_Economy` / `DT_MetaNodes_MVP` 同样先以 JSON 为入口。`UAffixDefinition` 已存在，是因为 P0.6 子系统在读表。商店购买、`WBP_Shop`、刷怪留到 runtime PR。导入时再按 schema 建 `UShopInventoryDefinition` / `UShopPriceCurveDefinition`（`UPrimaryDataAsset`），资产名仍用上表冻名。
