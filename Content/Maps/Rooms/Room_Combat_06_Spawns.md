# Room_Combat_06 — 刷怪点坐标表（相对入口）

**坐标系：** 原点 = 左侧入口内侧地面 `(0, 0)`（1 设计 px = 1 uu）。PlayerStart 在 `(48, 0)`。宽 **2560**；右出口约 `(2480, 0)`。  
**同时存活：** ≤6  
**波次：** W1 近压迫 → W2 中场+Air → W3 远收束+Air

## 刷怪点

| ID | Tag | WaveGroup | (X, Y) | Facing | Optional | 备注 |
|----|-----|-----------|--------|--------|----------|------|
| SP_G1 | Ground | 1 | (720, 0) | Left | false | W1 近压迫 |
| SP_G2 | Ground | 2 | (1440, 0) | Left | false | W2 中场 |
| SP_G3 | Ground | 3 | (2000, 0) | Left | false | W3 远收束 |
| SP_A1 | Air | 2 | (1120, 88) | Left | false | W2 中场上空 |
| SP_A2 | Air | 3 | (1760, 88) | Left | false | W3 远上空 |

占位：`Enemy.Grunt.Early` / `Enemy.Flyer.Early`。

## 布局锚点

| 锚点 | (X, Y) | 说明 |
|------|--------|------|
| PlayerStart | (48, 0) | 安全区 |
| Exit_Left | (-40, 0) | 入口 |
| Exit_Right | (2480, 0) | RequireClear |
| P_near | (560–800, 28) | 近台 |
| P_far | (1840–2160, 28) | 远台 |
| AffixAnchor | (1280, 0) | 三选一 |

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 初版相对入口表 |
