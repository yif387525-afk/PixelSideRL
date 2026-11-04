# Room_Event_01 — 正式事件房模板规格

| 字段 | 内容 |
|------|------|
| Type / Tag | Event / Branch |
| Owner | 廊道 |
| 状态 | **规格定稿候选** · 待 UE umap；交互已挂 `DA_Event_AbandonedCache` |
| 尺寸 | 设计宽 **1280**（2 屏）· **安全区** |
| 意图 | 支路非战斗事件；交互完成或跳过即可离开 |
| 入口安全区 | 平层 X≈0–240；无威胁 |
| Spawn | **无**（MVP 禁止战斗；spawns.csv 仅表头） |
| 同时存活上限 | 0 |
| Exit | Left / Right · **不锁** · **无** `RequireClear` |
| 清房 | 交互完成或跳过即可离开；**无三选一** |
| 数据 | `Content/Data/Rooms/Room_Event_01_*.csv` |
| 上场 | **仅支路**；与 Shop 同 Run 最多共占 1 条支路（二选一上场） |
| 禁止 | 任何刷怪点、伤害体积、清房门锁、强制交互、fake uasset、改 Lab |

**坐标：** 原点 = 左侧入口内侧地面；`+X` 右；`+Y` 上；1 px = 1 uu。  
**挂点默认：** Elite 后、BossAnte 前（RoomPipeline 开放问题默认，未另批则按此）。

---

## 1. ASCII 侧视（约 1 字符 ≈ 40 px）

```
Y
28|              [=气氛台=]
 0|安====掩====[交互物]====跳过提示====出口=
   +------------------------------------→ X
     0   320   640        960       1280
```

地面连续无坑；交互物居中可读；跳过提示靠近右出口。

---

## 2. 几何尺寸表

### 2.1 地面 / 边界

| ID | 类型 | X0 | X1 | TopY | 备注 |
|----|------|----|----|------|------|
| G_main | 地板 | 0 | 1280 | 0 | 安全主路；**无坑** |
| WALL_L | 边界墙 | -40 | 0 | — | 高 280 |
| WALL_R | 边界墙 | 1280 | 1320 | — | 高 280 |

### 2.2 平台（气氛；可选；TopY=28）

| ID | X0 | X1 | TopY | 角色 |
|----|----|----|------|------|
| P_atmos | 880 | 1080 | **28** | 气氛矮台；非必过 |

### 2.3 短掩体（气氛；挡弹可选；Pawn 可穿）

| ID | 底心 (X,Y) | 高 | 宽 | 作用 |
|----|------------|----|----|------|
| CV_A | (360, 0) | 40 | 24 | 近段气氛柱 |

无战斗需求；掩体仅打破空白盒感。

---

## 3. 出口

| ID | Tag | (X,Y) | 规则 |
|----|-----|-------|------|
| Exit_Left | Exit.Left | (-40, 0) | 回主干；**不锁** |
| Exit_Right | Exit.Right | (1200, 0) | 回主干；**不锁**；无 RequireClear |

---

## 4. 交互 / 刷怪

**刷怪：无。** 完整约定见 [`Room_Event_01_Spawns.md`](./Room_Event_01_Spawns.md)（仅锚点）。

| ID | Kind | (X,Y) | 备注 |
|----|------|-------|------|
| Interact_Main | Interactable | (640, 0) | 主事件交互；挂 `DA_Event_AbandonedCache` |
| SkipHint | UIHint | (1040, 0) | 文案：「走开（无奖励）」；靠近右出口 |
| EventResolve | Anchor | (640, 0) | 结算/Resolve 参考（与交互同点可） |

MVP：玩家可 **交互** 或 **直接走门跳过**；两种都立刻可离。

### 4.1 交互 DA（织局定稿）

| 项 | 值 |
|----|----|
| DA | `Content/Data/Events/DA_Event_AbandonedCache.json` |
| EventId | `AbandonedCache` |
| 标题 / 正文 | 见 DA；GDD §2.8 |
| CarefulLoot | `GrantScrap 25` |
| ForceLoot | `DamageSelf 15` → `GrantScrap 50`（HP 下限 1） |
| 跳过 | 走门 / SkipHint；无奖励；不弹三选一 |
| 运行时 | `UEventSubsystem::ResolveEventChoice`（`Docs/EventOps_API.md`）。确认选项由 `UPixelEventChoiceWidget` 调用一次。跳过 / 走门不调用。无 uasset |

UI 壳：`Docs/UI_Wireframes_Event.md`（`UPixelEventChoiceWidget`）。只显示本 DA 的标题、正文、两个按钮和 SkipHint；确认时把 EventId / ChoiceId 交给 Resolve，不自己加减 Scrap / HP，不 `SetGamePaused`。


---

## 5. 布局锚点

| 锚点 | (X,Y) | 说明 |
|------|-------|------|
| PlayerStart | (48, 0) | 入口安全区 |

---

## 6. 数据表

| 文件 | 用途 |
|------|------|
| `Content/Data/Rooms/Room_Event_01_meta.csv` | 元数据 |
| `Content/Data/Rooms/Room_Event_01_spawns.csv` | **仅表头**（无刷怪行） |
| `Content/Data/Rooms/Room_Event_01_geometry.csv` | 几何/出口/交互锚点 |
| `Content/Data/Events/DA_Event_AbandonedCache.json` | 交互文案与效果 |
| `Content/Data/Events/DT_Event_MVP.json` | 事件池（MVP 仅本条） |

`clear_rule=None`；`affix_on_clear=false`；`require_clear_exit` 空；`max_alive=0`。

---

## 7. 入库验收

- [ ] 无刷怪 / 无伤害体积 / 门不因清房上锁  
- [ ] 交互物居中可读；跳过靠近出口；可跳过离开  
- [ ] 无三选一；CSV 齐；无 Lab 改动  
- [ ] 登记为 Branch；与 Shop 二选一支路上场  
- [x] Interact 挂 `DA_Event_AbandonedCache`；SkipHint 文案齐  

---

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-04 | 细表占位 |
| 2026-11-05 | 正式规格：安全区几何/交互锚点/CSV；COMMENT 交审；文案待织局 DA |
| 2026-11-05 | 挂 `DA_Event_AbandonedCache`（CarefulLoot / ForceLoot / 跳过） |
