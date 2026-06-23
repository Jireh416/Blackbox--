# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-06-23
### Added
- 專案基礎目錄結構 (`src/`, `docs/`, `source/`)。
- 初始硬體架構與腳位定義 (`docs/hardware_spec.md`)。
- 核心狀態機韌體 (`main.ino`) 實作：
  - 整合 MPR121 觸控多樣性「習慣化」演算法。
  - 實作五階段情感流動 (Phase 0 ~ 4)。
  - 整合 FastLED 呼吸/脈動光效。
  - 整合 PCA9685 伺服馬達動態頂蓋。
  - 整合 DFPlayer Mini 非阻塞序列音效。
  - 加入震動馬達、風扇與超音波霧化片控制邏輯。
