# Room_Combat_04 — 刷怪点坐标表（相对入口）

**坐标系：** 原点 = 左侧入口内侧地面 `(0, 0)`（1 设计 px = 1 uu）。PlayerStart 在 `(48, 0)`。宽 **1760**；右出口约 `(1680, 0)`。坑 720–960；桥面 TopY=28。  
**同时存活：** ≤6  
**波次：** W1 Ground 近/坑缘 → W2 Ground 远 + 可选 Air@桥上

## 刷怪点

| ID | Tag | WaveGroup | (X, Y) | Facing | Optional | 备注 |
|----|-----|-----------|--------|--------|----------|------|
| SP_G1 | Ground | 1 | (520, 0) | Left | false | W1 近场 |
| SP_G2 | Ground | 1 | (680, 0) | Left | false | W1 坑缘左 |
| SP_G3 | Ground | 2 | (1000, 0) | Left | false | W2 坑缘右 |
| SP_G4 | Ground | 2 | (1320, 0) | Left | false | W2 远场 |
| SP_A1 | Air | 2 | (840, 72) | Left | **true** | 可选；桥上方 |

占位：`Enemy.Grunt.Early` / `Enemy.Flyer.Early`。

## 布局锚点

| 锚点 | (X, Y) | 说明 |
|------|--------|------|
| PlayerStart | (48, 0) | 安全区 |
| RecoverPoint | (680, 0) | 落坑软回收 |
| Exit_Left | (-40, 0) | 入口 |
| Exit_Right | (1680, 0) | RequireClear |
| BRIDGE | (720–960, 28) | 常驻桥 |
| P_far | (1200–1440, 28) | 远场台 |
| AffixAnchor | (880, 28) | 三选一 |

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 初版相对入口表 |
