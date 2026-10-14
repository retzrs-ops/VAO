# CHANGELOG（開発者向け）

## 0.0.0（未公開）
- 開発環境・フォルダ構成・開発ルール（CLAUDE.md、vao-dev スキル、引継ぎ資料）を用意。
- 名前を VAO – Voice Auto Optimizer に決定。
- obs-plugintemplate から作成。buildspec.json にプラグイン名 VAO・作者を設定。
- 音を素通しするフィルタ「VAO Voice」（src/obs）、ゲイン処理 src/dsp/gain、そのテスト tests/test_gain.cpp を追加。
- tools/build.bat・test.bat・install_local.bat を追加。
- CMakePresets.json の Windows SDK 版の固定を外した。
