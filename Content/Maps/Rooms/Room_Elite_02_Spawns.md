# Room_Elite_02 — 刷怪点坐标表（相对入口）

**坐标系：** 原点 = 左侧入口内侧地面 `(0, 0)`（1 设计 px = 1 uu）。PlayerStart 在 `(48, 0)`。宽 **1920**；右出口约 `(1840, 0)`。  
**同时存活：** ≤8（含精英）  
**波次：** W1 Ground 侧廊铺场 → W2 Elite 登台

## 刷怪点

| ID | Tag | WaveGroup | (X, Y) | Facing | Optional | 备注 |
|----|-----|-----------|--------|--------|----------|------|
| SP_G1 | Ground | 1 | (400, 0) | Left | false | W1 左廊 |
| SP_G2 | Ground | 1 | (720, 0) | Left | false | W1 左近台 |
| SP_G3 | Ground | 1 | (1520, 0) | Left | false | W1 右廊 |
| SP_E1 | Elite | 2 | (1080, 28) | Left | false | 唯一精英；精英高台中心 |

占位：`Enemy.Grunt.Early` / `Enemy.Elite.Mid`。

## 布局锚点

| 锚点 | (X, Y) | 说明 |
|------|--------|------|
| PlayerStart | (48, 0) | 安全区 |
| Exit_Left | (-40, 0) | 入口 |
| Exit_Right | (1840, 0) | RequireClear |
| P_elite | (880–1280, 28) | 精英高台 |
| P_approach_L | (640–800, 28) | 左接近台 |
| P_approach_R | (1360–1520, 28) | 右接近台 |
| AffixAnchor | (960, 0) | 三选一 |

## 变更

| 日期 | 说明 |
|------|------|
| 2026-11-05 | 初版相对入口表（高台精英 Y=28） |
