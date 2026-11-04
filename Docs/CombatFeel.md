# CombatFeel — 枪感与战斗手感参数表

**负责人:** 弹芯（Combat）  
**引擎:** UE 5.8+ · 侧视横版 · 正交像素相机  
**单位约定:** 1 逻辑像素 ≈ 1 UU（Unreal Unit）。角色目标高度 32–48 px，以下数值按 **角色高 40 px** 标定；ArtBible 定稿后按比例缩放并回写本表。  
**真相源:** 武器/敌人数值进 `DA_` / `DT_`；本文件是手感目标与调参区间，蓝图与 C++ 只读表、不写死魔法数。

---

## 1. 手感目标

| 维度 | 目标 | 反例 |
|------|------|------|
| 移动 | 贴地、急停干脆、空中可控但不飘 | 滑冰、二段跳飘移过大 |
| 射击 | 开火瞬间有反馈；弹道可读；命中有顿挫 | 静默开火、弹丸看不见、打中无感 |
| 受击 | 玩家短暂无敌可读；敌人击退可预期 | 连环硬直、击飞出屏 |
| 节奏 | 清房 20–40s；Boss 可读招式窗 | 站桩磨血、弹幕不可躲 |

参考气质（非抄数值）：*Dead Cells* 的急停与受击顿挫 + *Enter the Gungeon* 的弹道可读性，压到 640×360 像素侧视。

---

## 2. 玩家移动（`BP_PlayerSideScroller` / 必要时 `UPixelSideMovement` C++）

| 参数 | 建议值 | 区间 | 备注 |
|------|--------|------|------|
| MaxWalkSpeed | 245 | 180–260 | 约 6.1 身/秒 |
| GroundAcceleration | 3200 | 1800–3200 | 3–5 帧到满速 |
| GroundDeceleration | 4000 | 2400–4000 | 急停 ≤4 帧 |
| JumpZVelocity | 400 | 360–480 | 按住跳的起跳速度 |
| AirControl | 0.70 | 0.55–0.80 | 空中可改线 |
| CoyoteTime | 0.08 s | 0.06–0.12 | 离台容错 |
| JumpBuffer | 0.10 s | 0.08–0.14 | 落地前提前跳 |
| GravityScale | 2.55 | 1.8–2.8 | 下落快于上升 |
| FallGravityMult | 1.55 | 1.2–1.6 | 过顶后加压 |
| MaxFallSpeed | 1000 | 700–1100 | 防穿透薄平台 |
| JumpCutRisingZScale | 0.35 | 0.25–0.50 | 松跳且 Vz>0 时 `Velocity.Z *=` 该值；短按明显矮于长按 |
| Capsule | 12×38 | — | 显示可 ±1–2 px 偏移，记入下表 |

**碰撞 vs 显示偏移（强制记录）**

| 对象 | 显示相对碰撞 | 用途 |
|------|--------------|------|
| 玩家精灵 | (0, −1) px | 脚底贴地视觉 |
| 命中框（受伤） | 相对胶囊 −10% 高 | 略仁慈 |
| 子弹命中框 | 相对视觉 +1 px 半径 | 略慷慨，像素友好 |

输入：`IA_Move`（Axis1D）、`IA_Jump`、`IA_Fire`、`IA_Aim`（可选，默认面朝水平射击）。侧视默认 **面朝左右**；自由瞄准若做，限制在面朝半球 ±70°。

---

## 3. 瞄准与开火公共规则

| 规则 | 值 |
|------|-----|
| 枪口位置 | 相对角色中心 (10, 6) px，随朝向翻转 |
| 开火方向 | 默认朝向 ±X；按住上/下可 ±35°（MVP 可先纯水平） |
| 触发 | 按下开火；全自动武器按住连发 |
| 换弹 | MVP：弹药无限或大弹匣；预留 `ReloadTime` |
| 友伤 | 关 |
| 穿透默认 | 否（词条可开） |

**全局开火反馈（所有枪共享，强度可乘武器系数）**

| 反馈 | 建议 |
|------|------|
| 屏幕震动 | 振幅 1–2 px，时长 0.05–0.08 s，XY 正交相机 |
| 开火闪白 | 枪口 1 帧亮色 / Niagara 或 flipbook 2–3 帧 |
| 后坐力（视觉） | 精灵后坐 1–2 px，0.06 s 回弹；**不**推动角色（除非词条） |
| 开火音 | 按武器；同帧最多 1 次，避免叠满 |

---

## 4. MVP 三枪（`DA_Weapon_*`）

数值为 **局内基础值**；词条乘区由词条官管，本表只定手感骨架。

### 4.1 豌豆射手 `DA_Weapon_Peashooter`（默认）

| 参数 | 值 | 手感说明 |
|------|-----|----------|
| FireInterval | 0.12 s | ~8.3 rps，稳定主武器；实际冷却 = FireInterval/GetStat(FireRate) |
| Spread | 1.0° | 近乎直 |
| ProjectileSpeed | 560 | 中速可读 |
| ProjectileRadius | 3 px | |
| Damage | 8 | |
| KnockbackToEnemy | 70 | 明显推开 |
| HitStop_Player | 0.03 s | 短顿 |
| HitStop_Enemy | 0.08 s | 命中顿挫 |
| CamShakeScale | 1.1 | 命中反馈更明显 |
| MaxRange / Life | 0.9 s | 约 504 px 飞行 |

### 4.2 散弹 `DA_Weapon_Scatter`

| 参数 | 值 | 手感说明 |
|------|-----|----------|
| FireInterval | 0.45 s | 慢、狠；实际冷却 = FireInterval/GetStat(FireRate) |
| PelletCount | 5 | |
| Spread | 18° | 扇形 |
| ProjectileSpeed | 480 | |
| ProjectileRadius | 2.5 px | |
| Damage/弹 | 5 | 近战爆发 |
| KnockbackToEnemy | 70 | 明显推开 |
| HitStop_Enemy | 0.07 s | |
| CamShakeScale | 1.4 | |
| Falloff | 距离 >180 px 伤害 ×0.6 | 鼓励贴脸 |

### 4.3 穿甲矛枪 `DA_Weapon_Pike`（预留给构筑）

| 参数 | 值 | 手感说明 |
|------|-----|----------|
| FireInterval | 0.55 s | 蓄力感用间隔代替真蓄力（MVP）；实际冷却 = FireInterval/GetStat(FireRate) |
| Spread | 0° | |
| ProjectileSpeed | 640 | 快、直 |
| ProjectileRadius | 4 px | |
| Damage | 18 | |
| PierceCount | 2 | 穿 2 个单位后消失 |
| KnockbackToEnemy | 30 | |
| HitStop_Enemy | 0.06 s | |
| CamShakeScale | 1.0 | |

弹体类型：MVP 全部用 **Projectile Movement + Sphere**（像素圆），禁止 Hitscan（除未来狙击词条）。

---

## 5. 弹道 / 命中管线

```
AC_WeaponController.Fire
  → Spawn BP_Projectile_*（或轻量 Projectile Actor 池）
  → OnHit / Overlap → BI_Damageable.ApplyDamage(FDamageSpec)
  → 受击方：扣血、击退、HitFlash、HitStop、死亡
```

| 规则 | 约定 |
|------|------|
| 对象池 | 弹体 ≥64；禁止每发 NewObject 不回收 |
| 碰撞通道 | `Projectile` vs `Pawn` / `WorldStatic`；忽略发射者 0.05 s |
| 命中判据 | Overlap 优先（像素友好）；阻塞静态则销毁并播落地 FX |
| 多段伤害 | 同一 `(Projectile, Actor)` 0.1 s 内只结算一次 |
| 暴击/元素 | 预留 `FDamageSpec` 字段；MVP 恒 false / None |

`FDamageSpec`（Struct，放 Data 或 Combat 模块）：`Damage, Knockback, HitStopVictim, HitStopInstigator, DamageType, SourceActor, AffixTags`。

---

## 6. 受击与击杀反馈

### 玩家受击

| 参数 | 值 |
|------|-----|
| I-Frame | 0.60 s（闪烁 8–10 Hz） |
| HitKnockback | 145 水平，离开伤害源 |
| HitStop | 0.07 s |
| KnockbackSlide | 0.11 s ease-out；空中仍 ×0.6 |
| HitFlash | 白闪 2 帧 |
| 伤害数字 | HUD 可选；MVP 可只闪血条 |

### 敌人受击

| 参数 | 值 |
|------|-----|
| HitFlash | 白/受击色 2–3 帧 |
| Knockback 衰减 | 0.11 s ease-out 走完位移；空中保留 60% |
| HitStun | 小怪 0.08–0.12 s；精英 0.05 s；Boss **无硬直**（仅闪白） |
| 死亡 | 击退 ×1.5 + 4–6 帧碎裂/粒子 + 掉落由 Roguelike 模块生成 |

### HitStop 实现

- 优先：**自定义时间膨胀**仅作用于受击 Actor 与弹体（避免全局慢动作卡房间逻辑）。  
- MVP 可先用短 `CustomTimeDilation`；若与 AI Tick 冲突再改 C++ 局部 pause。

---

## 7. 敌人基础 AI（`BP_AI_*` / BehaviorTree 轻量）

MVP 三原型（数值进 `DT_EnemyStats`）：

| ID | 行为 | 移动 | 攻击 |
|----|------|------|------|
| `Grunt` | 进入警戒距后走向玩家，水平射击 | Speed 140 | 豌豆慢速 0.8 s，Damage 6 |
| `Hopper` | 跳跃接近，落地短停再跳 | Jump 为主 | 接触伤害 10 或落地溅射 |
| `TurretBug` | 固定或慢移，朝玩家方向间歇射击 | Speed 40 | 三连发，间隔 1.2 s |

公共 AI 参数：

| 参数 | 值 |
|------|-----|
| AggroRange | 280 px |
| LoseAggroRange | 360 px |
| AttackRange | 按武器 |
| 面朝 | 仅 ±X，与玩家同一套翻转 |
| 子弹 vs 玩家 | 同通道；敌人弹可复用弹体池，换伤害与颜色 |

Boss：本表只占位——**有线电报招 ≥0.4 s、无 HitStun、阶段血量 70%/30%**；具体招式等 GDD / RoomPipeline 定稿后补一节。

---

## 8. Blueprint vs C++ 分工

| 放 Blueprint | 放 C++（`Source/PixelSideRL`） |
|--------------|--------------------------------|
| `BP_PlayerSideScroller` 装配、动画通知 | 精确 Coyote/Buffer、移动急停（若 BP 抖） |
| `AC_WeaponController` 读 DA 开火 | 弹体对象池、`FDamageSpec` 应用 |
| `BP_Projectile_*` 外观与 FX | HitStop 局部时间（若需要） |
| 敌人 BT / 简易状态 | — |
| 调参、DA 默认值 | 网络预留接口（空壳即可） |

原则：**能 BP 调的手感不进 C++**；只有帧精度、池化、性能热点才下沉。

---

## 9. 验收清单（每日 / PR）

- [ ] `Run_Test`：走、跳、急停手感符合第 2 节观感  
- [ ] 三枪均可开火；弹道可见；命中有闪白 + 轻击退  
- [ ] 至少 1 种小怪会追击并受击死亡  
- [ ] 玩家受击有 I-Frame 闪烁，不会同帧连环秒杀  
- [ ] 本文件与 `DA_Weapon_*` / `DT_EnemyStats` 数值一致（改表必改文档一行）

---

## 10. 实现顺序（建议）

1. **P0** 玩家移动 + 相机跟随 + `BI_Damageable`  
2. **P0** `AC_WeaponController` + 豌豆弹体池 + 命中反馈  
3. **P1** 散弹 / 矛枪 DA 切换  
4. **P1** Grunt AI + 受击/死亡  
5. **P2** HitStop 打磨、碰撞显示偏移、Hopper/TurretBug  
6. **P2** Boss 占位与招式窗（依赖策划案）

---


## 11. 与 AffixSubsystem 的数值关系

契约：`Docs/AffixSubsystem_API.md`（主管裁定；**FireRate = 乘区**）。

- 武器 `DA_Weapon_*` 存绝对间隔 **`FireInterval`（秒）**，以及 Damage / Knockback 等基础值。
- `GetStat(FireRate)` = **射速乘区**，默认 Base `1.0`；Affix Mul 走 Π(1+Value)（如 RapidFire 0.12 → 乘区 ×1.12）。
- **实际开火冷却** = `FireInterval / GetStat(FireRate)`（等价于曾写的 `1/(BaseFireRateHz×Mult)`，DA 字段统一为 `FireInterval`）。
- 弹体：`Spec.Damage` / `Spec.Knockback` 经 `GetStat(Damage)` / `GetStat(Knockback)`（语义见 API）。
- 移速：`MaxWalkSpeed` ← `GetStat(MoveSpeed)`（§2 为手感目标 Base）。

禁止武器蓝图写死 AffixId 分支；禁止战斗侧自行 Roll 词条。


## 变更记录

| 日期 | 作者 | 说明 |
|------|------|------|
| 2026-11-04 | 弹芯 | 首版：单位约定、移动、三枪、弹道、受击、AI 骨架、BP/C++ 边界 |
| 2026-11-04 | 弹芯 | §11：对齐 AffixSubsystem_API v1（GetStat / FireRate=Hz） |
| 2026-11-04 | 弹芯 | FireRate=Hz 定稿；Mul=Π(1+Value) |
| 2026-11-04 | 弹芯 | **主管拍板**：DA=`BaseFireRateHz`，`GetStat(FireRate)`=乘区；间隔=1/(Base*Mult) |
| 2026-11-04 | 弹芯 | 与 API 统一：DA=`FireInterval`；冷却=`FireInterval/GetStat(FireRate)`；FireRate 乘区默认 1.0 |
| 2026-11-04 | 弹芯 | P0 默认值按 §2/§4.1/§6 进 C++。Knockback 按 uu 位移做 0.14s ease-out（急停摩擦 3200 会把 40/120 收成亚像素）；空中仍 ×0.6。Jump 动画按 v3 **5** 帧刮帧。 |
| 2026-11-04 | 弹芯 | **主管拍板（死亡细胞向）**：§2 急停/空中改线/Jump Cut 0.35；§4.1 豌豆 0.12s、散布 1°、弹速 560、击退 70、敌顿 0.08；§6 玩家击退 145、HitStop 0.07、滑行 0.11s。Coyote 0.08、Jump Buffer 0.10、FireRate 乘区、Jump **5** 帧不动。 |
