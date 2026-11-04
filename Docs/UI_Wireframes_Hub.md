# 线框说明：WBP_Hub · WBP_Hub_Upgrade

**状态：** 可交互线框，不接真实存档  
**画布：** 640×360 · 最近邻 · 安全区 8px  
**货币：** 只显示 / 消费 **Core**（局外）；禁止出现 Scrap 作为 Hub 通货  
**数据：** `UMetaProgressSubsystem`（GetCore / CanPurchase / Purchase / 节点列表）  
**节点表：** `RoguelikeMeta.md` §6.1（5 节点）

---

## 1. WBP_Hub（据点主界面）

**用途：** `Hub.umap` 主 UI；开始新一局 + 进入永久成长 + 看 Core。

### 1.1 布局

```
┌──────────────────────────────────────────────────────────────┐
│  据点                                      Core  120         │
│                                                              │
│              PixelSideRL                                     │
│                                                              │
│           [ 开始新一局 ]                                     │
│           [ 永久成长   ]                                     │
│           [ 上次结算   ]  ← 可选，有 LastRunSummary 才显      │
│                                                              │
│           [ 返回标题 ]                                       │
└──────────────────────────────────────────────────────────────┘
```

| 控件 | 类型 | 说明 |
|------|------|------|
| `Txt_Title` | TextBlock | `"据点"` |
| `Txt_Core` | TextBlock | `"Core 000"` 色 `#7EC8FF`（与局内 Scrap 黄区分） |
| `Btn_StartRun` | Button | `"开始新一局"` → `OnStartRun`（流程进 GenerateRun） |
| `Btn_Upgrades` | Button | `"永久成长"` → 打开 `WBP_Hub_Upgrade` |
| `Btn_LastSummary` | Button | 可选；打开只读上次 `WBP_RunSummary` 快照 |
| `Btn_MainMenu` | Button | 回 Entry |

### 1.2 状态

| 状态 | 表现 |
|------|------|
| `Idle` | 默认可点 |
| `Starting` | Start 锁输入短时，防双开局 |
| `NoSave` | Core=0、无节点时仍可开局 |

### 1.3 事件

- `OnStartRun()`  
- `OnOpenUpgrades()`  
- `OnBackToMenu()`  

---

## 2. WBP_Hub_Upgrade（永久节点）

**用途：** 列表/简单树展示 5 个 MVP 节点；用 Core 升级；调用 Meta `Purchase`。

### 2.1 布局

```
┌──────────────────────────────────────────────────────────────┐
│  永久成长                              Core  120    [关闭]   │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ 体质        Lv 1/3     下一阶 40 Core                  │  │
│  │ 开局最大生命 +10 → +20                                 │  │
│  │                                         [ 升级 ]       │  │
│  └────────────────────────────────────────────────────────┘  │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ 囤积        Lv 0/3     下一阶 15 Core                  │  │
│  │ …                                                      │  │
│  └────────────────────────────────────────────────────────┘  │
│  （可滚动；MVP 5 条一屏可塞下则不滚）                         │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 `WBP_Hub_NodeRow` 控件

| 控件 | 绑定 |
|------|------|
| `Txt_Name` | DisplayName |
| `Txt_Level` | `"Lv {Owned}/{MaxPurchases}"` |
| `Txt_Desc` | 当前效果简述 / 下一阶预览 |
| `Txt_Cost` | `"下一阶 {CostCore} Core"`；满级 `"已满级"` |
| `Btn_Buy` | `CanPurchase`？启用：点 → `Purchase(NodeId)` |
| `Border_Locked` | 前置未满足时灰锁（MVP 节点无前置则不用） |

### 2.3 Demo 节点（对齐 RoguelikeMeta §6.1）

| NodeId | 名 | Max | CostCore by rank | 效果摘要 |
|--------|----|-----|------------------|----------|
| meta_vitality | 体质 | 3 | 20/40/70 | 开局 HP +10/20/30 |
| meta_stockpile | 囤积 | 3 | 15/35/60 | 开局 Scrap +15/30/50 |
| meta_pea_familiar | 豆射手熟悉 | 3 | 25/50/90 | 豆枪伤害 +4/8/12% |
| meta_scrap_sense | 废料感知 | 3 | 20/45/80 | 击杀 Scrap +5/10/15% |
| meta_second_gun | 第二枪缘分 | 1 | 40 | 层1 房2 后保底武器房 |

`DemoCore` 初始 **120**。

### 2.4 状态

| 状态 | 表现 |
|------|------|
| `Affordable` | 升级钮 Primary |
| `CantAfford` | 费用红字；钮 Disabled |
| `Maxed` | 无升级钮 / `"已满级"` |
| `Purchasing` | 锁输入 → Purchase → 刷新 Core 与行 |
| `Closed` | 回 `WBP_Hub` |

### 2.5 输入

| 输入 | 行为 |
|------|------|
| ↑ ↓ | 焦点行 |
| Enter | 升级焦点行 |
| Esc / 关闭 | 回 Hub 主界面 |

### 2.6 API（只调 Meta，不写存档细节）

```
Core = GetCore()
CanPurchase(NodeId) / Purchase(NodeId)
节点列表：DA_MetaTree / stub 只读展示字段
  DisplayName, Description, CostCore(下一阶), OwnedRank, MaxPurchases
```

UI **不**改 `FMetaSaveData`；Purchase 成功后听刷新或主动 GetCore。

---

## 3. 交付清单

| 项 | 状态 |
|----|------|
| `WBP_Hub` 控件 / 状态 / 事件 | ✅ |
| `WBP_Hub_Upgrade` + `WBP_Hub_NodeRow` | ✅ |
| Core 文案与 Scrap 隔离 | ✅ |
| UE 蓝图壳 | ⏳ 仓库就绪后 |
