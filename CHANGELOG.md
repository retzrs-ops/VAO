# CHANGELOG（開発者向け）

## 0.1.0 開発中（buildspec.json の版は未更新）
- コンプレッサー・先読みリミッター（src/dsp）、測定（src/calib/measurer）、自動設定の計算（src/calib/auto_settings）を追加。
- フィルタ画面: コンプ／出力ゲイン／リミッターを各グループのチェックボックスでオンオフ。自動設定は「すべて自動設定（測定する）」の1つだけ。
- 設計メモに仕様（1 画面・オンオフ・自動設定・EQ プリセット方針）を追記。
- tests を4本に分割（test_util.h を共通化）。

## 0.0.0（未公開）
- 開発環境・フォルダ構成・開発ルール（CLAUDE.md、vao-dev スキル、引継ぎ資料）を用意。
- 名前を VAO – Voice Auto Optimizer に決定。
- obs-plugintemplate から作成。buildspec.json にプラグイン名 VAO・作者を設定。
- 音を素通しするフィルタ「VAO Voice」（src/obs）、ゲイン処理 src/dsp/gain、そのテスト tests/test_gain.cpp を追加。
- tools/build.bat・test.bat・install_local.bat を追加。
- CMakePresets.json の Windows SDK 版の固定を外した。
