# RoomPipeline — 房间模板与拼局规则

**Owner:** 廊道（关卡与流程）  
**对齐:** `UE_PROJECT_SPEC.md` MVP · 依赖 `GDD.md` / `RoguelikeMeta.md`（玩法数值以策划文档为准）  
**目标一局时长:** 15–20 分钟  
**模板配额:** 8–12 个可拼房间模板 + 1 个 Boss 房间 + 1 前厅

## 0. 已拍板（MVP · 主管小yan · 2026-11-04）

1. **拓扑：** 线性主干 + **最多 1 条支路**；支路只放 `Event` 或 `Shop`。  
2. **Event：** MVP **不允许战斗**；无清房门锁；交互完成或跳过即可离开。  
3. **Boss：** **要前厅**。流程：`Combat` / `Elite` →（可选支路）→ **前厅**（无怪或极少量引导）→ `Boss` 房 → 结算。  

建图顺序：`Combat_01/02` → `Boss_01` 骨架（含前厅）→ `Elite_01` → 其余。

---

## 1. 职责边界

| 模块 | 负责 | 不负责 |
|------|------|--------|
| Rooms（本文档） | 房间模板布局、刷怪点、出口/门、精英标记、拼局拓扑、Boss 节奏骨架 | 词条数值、枪感、敌人 AI 细节 |
| Roguelike / 织局 | 奖励节点、三选一触发时机、难度曲线意图 | 具体几何与刷怪点摆放 |
| Combat / 弹芯 | 可打性校验、碰撞与射程可读性 | 房间拼图规则 |
| Data / 词条官 | `DA_` / `DT_` 房间与波次数据表 | 关卡几何 |

**真相源：** 布局约定与拼局规则以本文档为准；波次/敌人表以 `Content/Data/Rooms` DataAsset 为准。

---

## 2. 资源路径与命名

```
Content/Maps/Rooms/           # 单房间模板关（umap）
Content/Blueprints/Rooms/     # BP_RoomTemplate、出口、刷怪点 Actor
Content/Data/Rooms/           # DA_Room_*、波次 / 标签表
```

| 资源 | 命名 | 例 |
|------|------|-----|
| 房间关卡 | `Room_<Type>_<Id>` | `Room_Combat_01` |
| 房间 DataAsset | `DA_Room_<Type>_<Id>` | `DA_Room_Combat_01` |
| 刷怪点 | `BP_SpawnPoint` + 标签 | Tag: `Spawn.Ground` / `Spawn.Air` / `Spawn.Elite` |
| 出口 | `BP_RoomExit` | Tag: `Exit.Left` / `Exit.Right` / `Exit.Boss` |
| Boss 房 | `Room_Boss_01` | 唯一正式 Boss 模板（MVP） |
| Boss 前厅 | `Room_BossAnte_01` | 无怪/极少引导；进 Boss 前门 |

- 单房间模板优先；禁止巨型一体关。  
- 同一张 umap 同时仅一人编辑。  
- `_Dev` 沙盒内容不得进正式拼局池。

---

## 3. 房间类型（MVP）

| Type | 数量目标 | 时长贡献（约） | 出口 | 清房条件 |
|------|----------|----------------|------|----------|
| `Combat` | 5–7 | 60–90s | 左右标准门 | 清空波次 → **三选一**（GDD） |
| `Elite` | 1–2 | 90–120s | 标准门 | 击杀精英+杂兵 → **三选一** |
| `Event` | 1–2 | 30–60s | 标准门（**不锁**） | **无战斗**；交互完成或跳过即离开；无三选一 |
| `Shop` | 0–1 | 20–40s | 标准门（不锁） | 购买或跳过离开；仅可出现在支路 |
| `BossAnte` | 1 | 15–25s | 前进门进 Boss | **无怪或极少量引导**；无三选一；不锁清房 |
| `Boss` | 1 | 150–210s | 结算出口 | Boss 死亡 → **结算** |

拼局池合计 **8–12** 个 Combat/Elite/Event 模板；另加 `BossAnte_01` + `Boss_01`。Shop 可与 Event 共用支路槽位。

### 3.1 横版几何约定

- 逻辑参考分辨率 **640×360**；房间设计宽度约 **2–4 屏**（Combat），Boss **3–5 屏**。  
- 主路径水平可读；垂直差控制在可跳/可平台链范围内（具体像素以 ArtBible / 碰撞为准）。  
- 出生点：默认入口内侧 **1 屏内安全区**，无即时伤害体积。  
- 掩体/高低差至少提供 **一条近战压迫线 + 一条风筝线**（Combat/Elite）。
- 掩体 Peek（横版）：**穿过掩体后站对侧（沿 X）**；禁止依赖跳上掩体顶或绕 Y 侧面。级差必过台 ≤ 现行净跳窗（约 28，净跳 ~32）。

---

## 4. 刷怪点（Spawn）

### 4.1 组件约定

每个模板关放置 `BP_SpawnPoint` 实例，最少字段：

- `SpawnTag`：`Ground` / `Air` / `Elite` / `Boss`  
- `Facing`：左/右/朝向玩家入口  
- `WaveGroup`：整数，同一波同组激活  
- `Optional`：布尔；难度低时可跳过  

### 4.2 波次节奏（Combat 默认）

| 波 | 时机 | 意图 |
|----|------|------|
| W1 | 进房后 0.5–1.0s | 试探，低密度 |
| W2 | 清 W1 或超时 | 主压力，引入组合 |
| W3（可选） | 清 W2 | 收束或精英预告 |

- Elite 房：W1 杂兵铺场 → W2 精英登场（精英点 `Spawn.Elite`）。  
- 禁止进房瞬间满屏刷怪；首波出生点须在**屏幕外或半遮挡**，避免贴脸。  
- 同时存活上限建议：普通房 **6**，精英房 **8**（含精英），最终以 CombatFeel / AI 预算校准。

### 4.3 与数据表挂钩

`DA_Room_*` 引用：

- 敌人池（敌 ID 列表，不写具体数值）  
- 波次条目：`(WaveGroup → EnemyId × Count)`  
- 房间标签：`Early` / `Mid` / `Late`（供拼局权重）

---

## 5. 出口与门

### 5.0 清房奖励挂钩（与 GDD）

- Combat / Elite：`OnRoomCleared` → 门锁打开前或开门同时弹出 **三选一**（`WBP_AffixChoice`）；选完再允许进门。  
- Event：MVP **禁止刷怪/战斗**；门不因清房上锁。  
- Shop：无强制三选一；商品由 Roguelike 数据驱动；门不锁。  
- BossAnte：无三选一；引导结束后开进 Boss 的门。  
- Boss：无三选一；走 Run 结算。

### 5.1 `BP_RoomExit`

- 默认左右各一；单向推进局可只开「前进」侧。  
- 上锁条件：`RequireClear`（Combat/Elite/Boss = true；Event/Shop/BossAnte = false）。  
- 解锁表现：门锁 → 开门（表现归 UI/FX，逻辑只发 `OnRoomCleared`）。  
- Boss 入口：仅从 **前厅** 前进门进入；Boss 房出口进结算，不回拼局图。

### 5.2 连接规则

- 模板只声明 **插座**（左/右出口），不写死邻居房间。  
- 拼局器按拓扑边连接同高程出口；禁止十字无缝拼接依赖（MVP 不做 2D 网格分支也可跑通 **线性 + 1 条支路**）。

### 5.3 P0.7 落地（仅 Room_Combat_01）

`APixelRoomDirector` 读 `Content/Data/Rooms/Room_Combat_01_{meta,spawns,geometry}.csv`。进房 0.5–1.0s 刷 W1，清 W1 再刷 W2，同时存活不超过 `max_alive`。`clear_rule=WavesCleared` 且 `affix_on_clear` 时调用 `RequestAffixChoice(NormalRoom)`（P0.6 同一池）。确认一张后打开 `require_clear_exit`（`Exit.Right`）。`Exit.Left` 回退不重置已清、不重刷。精英房 / Boss / Hub 不在本步。正式 umap 未拼，空关驱动见 `Docs/Combat_P0_LabSetup.md` §7。

验收：进房 → 两波 → 三选一 → 右门开。

---

## 6. 拼局规则（Run Graph）

### 6.1 MVP 拓扑（已拍板）

```
[Hub]
  → Combat×N
  → Elite
  → Combat×M
  → (可选：最多 1 条支路 → Event 或 Shop → 回到主干)
  → BossAnte（前厅）
  → Boss
  → Settlement
```

- **主干线性**；全 Run **最多 1 条支路**，支路节点只能是 Event 或 Shop。  
- 总战斗房（Combat+Elite）约 **6–9** 间，使一局落在 15–20 分钟。  
- 第 1–2 房：仅 `Early` 标签模板。  
- Elite：**恰好 1**（可进池 2 个模板轮换）。  
- 进 Boss 前 **必须** 经 `BossAnte`；禁止 Combat/Elite 直连 Boss。

### 6.2 权重与去重

- 同一次 Run 内同一 `Room_*` umap **默认不重复**；池耗尽才允许 Combat 复用。  
- `Late` 标签不得出现在前 2 房。  
- Event/Shop 与 Elite 不相邻超过 1 次（避免奖励扎堆）。

### 6.3 难度锚点（流程侧）

| 进度 | 房间标签 | 波次 |
|------|----------|------|
| 0–25% | Early | W1–W2，无 W3 |
| 25–70% | Mid | 标准 W2，偶发 W3 |
| 70–100% | Late / Elite / Boss | 满配，精英与 Boss 机制完整 |

具体敌人数值由 RoguelikeMeta / 敌人表驱动；本文档只约束 **何时加压**。

---

## 7. Boss 前厅与 Boss 战节奏

### 7.1 前厅 `Room_BossAnte_01`

- 宽度约 **1–2 屏**；安全区为主。  
- **无战斗波次**；允许 0 怪，或 ≤2 个无伤害/弱引导单位（教学走位，可关）。  
- 门：入口不锁；前进门进 `Room_Boss_01`（可播短演出后再开）。  
- 禁止三选一、禁止商店。

### 7.2 Boss 房 `Room_Boss_01`

**总时长目标:** 2.5–3.5 分钟（含 Intro；不含前厅）

| 阶段 | 时长（约） | 关卡侧要求 |
|------|------------|------------|
| Intro | 3–5s | 锁门、镜头可读、无伤害 |
| P1 | 40–60s | 场地半开放；刷怪点仅辅助杂兵（可选） |
| 转场 | 5–8s | 平台/危害启用或侧通路关闭；明确可读 telegraph |
| P2 | 50–80s | 全场地危害或夹击点激活；禁止无法躲避的死区 |
| Outro | — | Boss 死亡 → 解锁结算出口 |

- Boss 本体出生点唯一：`Spawn.Boss`。  
- 杂兵若存在：限频、限上限（建议同时 ≤3），不得盖过 Boss 可读性。  
- 失败回收：玩家死亡回 Hub / 结算，不中途改房间几何。  
- **骨架与前厅同批建设**（建图顺序已拍板）。

---

## 8. 验收清单（模板入库）

每个新房间 PR 必须满足：

1. [ ] 命名与路径符合第 2 节  
2. [ ] 至少 1 个入口安全区 + 左右（或文档声明的）出口  
3. [ ] 刷怪点带 Tag 与 WaveGroup；DA 已挂波次  
4. [ ] `Run_Test` 可单独加载并清房开门  
5. [ ] Docs：本文件「模板登记表」增一行  
6. [ ] 无 `_Dev` 依赖、无未命名临时 Actor

---

## 9. 模板登记表（MVP）

| ID | Type | 标签 | 状态 | 备注 |
|----|------|------|------|------|
| Room_Combat_01 | Combat | Early | P0.7 导演已接 CSV | 灰盒导演已接；正式 umap 建图清单见 `Docs/Room_Combat_01_UMapBuild.md`；Mac/UE 待拼 |
| Room_Combat_02 | Combat | Early | 规格定稿候选 | 一层平台 TopY=28 + Air@W2；CSV 已挂 |
| Room_Combat_03 | Combat | Mid | 规格定稿候选 | 双高台主次不对称 28/56；CSV 已挂 |
| Room_Combat_04 | Combat | Mid | 规格定稿候选 | 坑+桥；CSV 已挂 |
| Room_Combat_05 | Combat | Mid/Late | 规格定稿候选 | 夹击走廊；CSV 已挂 |
| Room_Combat_06 | Combat | Late | 规格定稿候选 | 开阔风筝；CSV 已挂 |
| Room_Elite_01 | Elite | Mid | 规格定稿候选 | 中央竞技场；CSV 已挂 |
| Room_Elite_02 | Elite | Mid | 规格定稿候选 | 高台压制；CSV 已挂；与 Elite_01 池轮换 |
| Room_Event_01 | Event | Branch | 规格定稿候选 | 无战斗；仅支路；CSV 已挂；**DA_Event_AbandonedCache** |
| Room_Shop_01 | Shop | Branch | 规格定稿候选 | Presenter/#47 已接 `OpenShop`；正式 umap 建图清单见 `Docs/Room_Shop_01_UMapBuild.md`；Mac/UE 待拼 |
| Room_BossAnte_01 | BossAnte | BossGate | 规格定稿候选 | CSV 已挂；BossGate 进 Boss_01 |
| Room_Boss_01 | Boss | Boss | 规格定稿候选 | 双阶段；CSV 已挂；经前厅进入 |

> 建图顺序（已拍板）：`Combat_01/02` → `Boss_01` 骨架（**含前厅**）→ `Elite_01` → 其余 Combat → Event/Shop。

---

## 10. 开放问题

- （原 3 条已拍板，见 §0。）  
- 支路挂点偏好：Elite 前 / Elite 后 / BossAnte 前？（默认：**Elite 后、前厅前**，未另批则按此实现。）  
- ~~Event 具体交互内容：待 GDD / 织局补文案后挂 DA。~~ → **已挂** `DA_Event_AbandonedCache`（GDD §2.8）。

---



## 附录 · Lab 沙盒（非拼局）

P0 手感关 `Lab_CombatP0` 布局见 `Docs/Lab_CombatP0_Layout.md`（**已拍板**）。属 `Content/_Dev/Combat/`，**不进**正式房间池与登记表。落地：`bSpawnLabSandbox` 程序化 + 落坑软回收；不先手摆 umap。

## 变更记录

| 日期 | 变更 |
|------|------|
| 2026-11-04 | 初稿：类型、刷怪点、出口、拼局、Boss 节奏、登记表 |
| 2026-11-04 | 写入已拍板：线性+1支路、Event 无战斗、Boss 前厅；更新拓扑/登记表 |
| 2026-11-04 | MVP 全模板规格细表落盘（11 间）；登记表状态→细表完成 |
| 2026-11-04 | 附录：Lab_CombatP0 立体灰盒方案（待主管确认） |
| 2026-11-04 | Lab 拍板落地：岛跳 / 软回收 / SpawnLabSandbox 程序化 |
| 2026-11-05 | Combat_01 正式几何/出口/CSV 规格；Peek=2D 站位；COMMENT 交审 |
| 2026-11-05 | Combat_02 正式规格：一层平台/刷怪/出口/CSV；COMMENT 交审 |
| 2026-11-05 | Combat_03 正式规格：双高台/刷怪/出口/CSV；COMMENT 交审 |
| 2026-11-05 | BossAnte_01 骨架正式规格 COMMENT 交审 |
| 2026-11-05 | Boss_01 骨架正式规格 COMMENT 交审 |
| 2026-11-05 | Elite_01 骨架正式规格 COMMENT 交审 |
| 2026-11-05 | Combat_04 正式规格 COMMENT 交审 |
| 2026-11-05 | Combat_05 正式规格 COMMENT 交审 |
| 2026-11-05 | Combat_06 正式规格 COMMENT 交审 |
| 2026-11-05 | Event_01 正式规格 COMMENT 交审 |
| 2026-11-05 | Shop_01 正式规格 COMMENT 交审 |
| 2026-11-05 | Event_01 挂 `DA_Event_AbandonedCache`；开放问题关闭 |
| 2026-11-04 | P0.7：Room_Combat_01 导演读 CSV（两波 → 三选一 → 右门）。umap 待拼 |
| 2026-11-05 | Elite_02 骨架正式规格 COMMENT 交审 |
| 2026-11-05 | Combat_01 正式 umap 建图清单（真图待 Mac；仓库无 umap） |
| 2026-11-05 | Shop_01 正式 umap 建图清单（真图待 Mac；仓库无 umap） |
| 2026-11-05 | Combat_01 / Shop_01 umap 验收勾选差异见 `Docs/Room_UMapBuild_AcceptDiff.md` |
