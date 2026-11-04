# P0 线框说明：WBP_HUD · WBP_AffixChoice

**状态：** 可交互线框（占位像素），不接真实战斗数据  
**画布：** 640×360 逻辑像素 · 最近邻 · 四周安全区 8px  
**临时色（点阵锁色板前）：** 见文末「临时色板」  
**落库路径（仓库建好后）：** `Content/Blueprints/UI/` · 贴图 `Content/Art/UI/`

**词条对接：** 见 `Docs/AffixSubsystem_API.md` §0 / §8；卡面仅 Name/Desc/Rarity/Icon。

---

## 1. WBP_HUD

**用途：** 局内常驻；不暂停时间。  
**数据：** 本期用占位绑定（可调 Slider / 按钮模拟）；正式版只读 Combat / Roguelike 子系统。

### 1.1 布局（640×360）

```
┌──────────────────────────────────────────────────────────────┐  y=8
│ [HP####....]  [WPN] ammo 99    币 000          [a][b][c][d]… │  顶栏 h≈28
│                                                              │
│                         （游戏画面）                           │
│                                                              │
│                                                              │
└──────────────────────────────────────────────────────────────┘
  x=8                                                    x=632
```

| 区域 | 锚点 | 尺寸(px) | 说明 |
|------|------|----------|------|
| 血条 `Bar_Health` | 左上 (8,8) | 96×8 | 底槽 + 填充；低血闪烁 |
| 武器图标 `Img_Weapon` | 左上 (112,8) | 16×16 | 当前武器占位方块 |
| 弹药/能量 `Txt_Ammo` | 武器右侧 | 自适应 | `"99"` 或能量条二选一，本期数字 |
| 局内货币 `Txt_Coin` | 顶栏偏右 | 自适应 | `"币 000"` |
| 词条图标条 `HB_AffixIcons` | 右上 (632 右对齐,8) | 每格 12×12，间距 2，最多 8 | GDD：已选关键词条一排 |

### 1.2 控件列表

| 控件名 | 类型 | 绑定（占位） | 正式接口（预留） |
|--------|------|--------------|------------------|
| `Bar_Health` | ProgressBar | `DemoHealth` 0–1 | `GetHealthRatio()` |
| `Img_Weapon` | Image | 色块 / `T_UI_WeaponPlaceholder` | 当前武器 Icon |
| `Txt_Ammo` | TextBlock | `DemoAmmo` int | 弹药或能量 |
| `Txt_Coin` | TextBlock | `DemoCoin` int | 局内货币 |
| `HB_AffixIcons` | HorizontalBox | 0–8 个 `Img_AffixSlot` | 听 `OnAffixChanged` → `GetOwnedAffixes()` 只取 **Icon + AffixId** |
| `Img_AffixSlot_*` | Image | 空槽灰 / 有词条 | `GetOwnedAffixes` → Icon；空 → 占位 |
| `Btn_DemoHurt`（仅 _Dev） | Button | −10% 血 | 删除于正式 |
| `Btn_DemoHeal`（仅 _Dev） | Button | +10% 血 | 删除于正式 |
| `Btn_DemoAddAffix`（仅 _Dev） | Button | 追加占位图标 | 删除于正式 |

### 1.3 状态

| 状态 | 表现 | 触发（占位） |
|------|------|--------------|
| `Normal` | 血条绿/青填充，顶栏全显 | 默认 |
| `LowHealth` | 血量 ≤25%：血条红 + 2Hz 闪烁填充 | `DemoHealth≤0.25` |
| `Dead` | 血条空；可选全屏暗角（本期不做） | `DemoHealth≤0` |
| `AffixFull` | 已 8 个图标时 `_Dev` 追加无效 | 图标数≥8 |
| `Hidden` | 整 Widget Collapse（过场/主菜单） | 外部 SetVisibility |

### 1.4 交互

- 无玩法点击；鼠标穿透到世界（`Visibility` 对装饰设为 SelfHitTestInvisible）。  
- `_Dev` 按钮仅调试层，正式包剥离。

### 1.5 子 Widget（可选拆分）

- `WBP_HUD_AffixIcon`：单格 12×12（空 / 有图标 / 悬停提示预留）。

---

## 2. WBP_AffixChoice

**用途：** 房间清空后弹出三选一；**打开时暂停局内时间**。  
**目标：** 一屏看懂稀有度、名字、简述，1/2/3 或点击选定。

### 2.1 布局（640×360）

```
┌──────────────────────────────────────────────────────────────┐
│                         （半透明遮罩）                         │
│                                                              │
│              选择一项强化                    [Esc 跳过?]        │  标题区
│                                                              │
│     ┌─────────┐     ┌─────────┐     ┌─────────┐            │
│     │  [1]    │     │  [2]    │     │  [3]    │            │  三卡
│     │  ICON   │     │  ICON   │     │  ICON   │            │  每卡 ~120×168
│     │  名称   │     │  名称   │     │  名称   │            │
│     │  简述…  │     │  简述…  │     │  简述…  │            │
│     │ Common  │     │  Rare   │     │ Epic    │            │
│     └─────────┘     └─────────┘     └─────────┘            │
│         ↑焦点                                                    │
│                                                              │
│              ← → 切换 · Enter 确认 · 1 2 3 直选               │
└──────────────────────────────────────────────────────────────┘
```

三卡水平居中，卡间距 16px；垂直约居中偏上（顶边 y≈56）。

### 2.2 控件列表

| 控件名 | 类型 | 说明 |
|--------|------|------|
| `Overlay_Root` | Overlay | 全屏 |
| `Img_Dim` | Image | 全屏遮罩，临时色 `#000000` @ 60%（允许半透明仅遮罩层） |
| `Txt_Title` | TextBlock | `"选择一项强化"` |
| `Txt_Hint` | TextBlock | `"1 / 2 / 3 选择 · Enter 确认"` |
| `HB_Cards` | HorizontalBox | 三张卡容器 |
| `WBP_AffixCard` ×3 | 用户控件 | 见下表；实例名 `Card_0` `Card_1` `Card_2` |
| `Btn_Skip`（可选 MVP） | Button | GDD 未强制跳过；默认 **隐藏**，商店才可跳过 |

#### `WBP_AffixCard` 内部

| 控件名 | 类型 | 尺寸 | 绑定字段（占位 struct） |
|--------|------|------|-------------------------|
| `Border_Rarity` | Border | 120×168 | 稀有度边框色 |
| `Btn_Select` | Button | 铺满 | 点击 = 选此卡 |
| `Txt_Hotkey` | TextBlock | 角标 | `"1"` / `"2"` / `"3"`（UI 本地，非 DA） |
| `Img_Icon` | Image | 32×32 | **`FAffixCardView.Icon`**；空 → `T_UI_AffixPlaceholder` |
| `Txt_Name` | TextBlock | 一行 | **`FAffixCardView.Name`**（DA `DisplayName`） |
| `Txt_Desc` | TextBlock | 最多 3 行 wrap | **`FAffixCardView.Desc`**（已 Format） |
| `Txt_Rarity` | TextBlock | 底部 | **`FAffixCardView.Rarity`** → 文案；边框色同步 |
| `Txt_Stacks`（可选） | TextBlock | 角标 | `Stacks` / `bOwned` 时显示；契约辅助字段 |

### 2.3 占位数据（Demo）

| Slot | Name | Rarity | Desc |
|------|------|--------|------|
| 0 | 豌豆连射 | Common | 射速 +15% |
| 1 | 爆裂弹芯 | Rare | 击中附加小范围伤害 |
| 2 | 吸血涂层 | Epic | 击杀回复 1 点生命 |

### 2.4 状态

| 状态 | 作用对象 | 表现 |
|------|----------|------|
| `Closed` | 根 | Collapsed；恢复时间 |
| `Open` | 根 | Visible；暂停时间；默认焦点 `Card_0` |
| `Card_Idle` | 卡 | 稀有度色边 2px；无额外亮边 |
| `Card_Focused` | 卡 | 外再加 1px 亮边（临时白）；微上移 2px（整数） |
| `Card_Pressed` | 卡 | 亮边保持；缩放 98%（可选） |
| `Card_Disabled` | 卡 | 灰化（无候选时）；本期三卡始终可用 |
| `Confirming` | 根 | 短锁输入 0.15s → 广播选择 → Closed |

**稀有度边框临时色：**

| Rarity | 边框 |
|--------|------|
| Common | `#B0B0B0` |
| Rare | `#3D7EFF` |
| Epic | `#B44DFF` |
| Legendary | `#FFC107` |

### 2.5 输入

| 输入 | 行为 |
|------|------|
| `1` / `2` / `3` | 焦点对应卡并确认（或仅确认，本期：**直选并确认**） |
| ← / → · A/D | 焦点循环 |
| Enter / Space | 确认焦点卡 |
| 鼠标点击卡 | 确认该卡 |
| Esc | 本期无跳过 → 忽略（或仅关闭 Debug） |

### 2.6 AffixSubsystem 对接（契约 v1，词条官冻结）

**只读展示四字段：** `Name` / `Desc` / `Rarity` / `Icon`（经 `GetAffixCardView`；勿直接扒 DA 未列字段）。

| 步骤 | API / 事件 |
|------|------------|
| 打开 | 听 `OnAffixChoiceOffered(TArray<FAffixChoiceEntry>)`（恰好 3）→ 对每条 `GetAffixCardView(Def)` → 填卡 |
| 确认 | `ConfirmAffixChoice(ChoiceIndex)`（1/2/3 或点击对应 Index 0..2） |
| 关闭 | 听 `OnAffixChoiceClosed(bConfirmed)` → Collapse；流程侧解暂停 |
| 禁止 | UI 不调用 `RollChoices` / `RequestAffixChoice`；不写词条数值 |

`SkipAffixChoice`：GDD 禁止跳过三选一 → UI **不暴露** Esc 跳过（与契约「若允许」分支一致：本期不允许）。

### 2.7 「一屏看懂」检查

- [x] 三卡同屏，无翻页  
- [x] 稀有度色边 + 文字双重标识  
- [x] 名称 + ≤3 行简述  
- [x] 热键角标与底部提示一致  

---

## 3. 临时色板（UI Debug）

| 用途 | Hex | 备注 |
|------|-----|------|
| 画布底（线框预览） | `#1A1A2E` | 仅编辑器预览 |
| HUD 文字 | `#F2F2F2` | 高对比 |
| 血条底 | `#3A3A3A` | |
| 血条填充 | `#2EE6A6` | LowHealth→`#FF3B3B` |
| 货币 | `#FFD54A` | |
| 空词条槽 | `#4A4A4A` | |
| 遮罩 | `#000000` 60% | 仅 AffixChoice |
| 焦点亮边 | `#FFFFFF` | 1px |

正式色板由点阵写入 ArtBible 后替换本节。

---

## 4. 本周交付清单

| 项 | 状态 |
|----|------|
| `WBP_HUD` 控件列表 + 状态 | ✅ 本文 §1 |
| `WBP_AffixChoice` / `WBP_AffixCard` 控件列表 + 状态 | ✅ 本文 §2 |
| 布局线框（ASCII） | ✅ |
| 占位数据与输入表 | ✅ |
| UE 内可点击蓝图壳 | ⏳ 等仓库 + 引擎环境；设计已可直接落地 |
| 真实战斗/词条 DA 绑定 | ❌ 明确不做（本期） |

---

## 5. 下一步（不阻塞本周）

1. 点阵：UI 切片尺寸按本文锁定（血条 96×8、词条图标 12、卡面 120×168、卡图标 32）。  
2. 词条官：确认 Affix 展示字段 `Name` / `Desc` / `Rarity` / `Icon`。  
3. 仓库就绪后：在 `Content/Blueprints/UI/` 建 `WBP_HUD`、`WBP_AffixChoice`、`WBP_AffixCard`。
