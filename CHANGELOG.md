# CHANGELOG（開発者向け）

## 0.2.1
- インストーラーの操作を減らした: ようこそ画面で「インストール」を押すだけにし（準備完了の画面を出さない）、Windows が日本語／英語なら言語選択も出さない。
- 利用者向け案内ページ（docs/index.html、GitHub Pages 用）と、実機のスクリーンショットを追加。

## 0.2.0（初回リリース）
- GitHub Actions は Windows のみビルド（macOS・Linux は無効化）。clang-format で整形。
- ハイパス（src/dsp/highpass）、ノイズゲート（gate）、4 バンド EQ と 6 つのプリセット（eq）、ディエッサー（deesser）、共通の biquad を追加。
- 測定で声の帯域（100Hz／300Hz／3kHz／7kHz）の強さも測り、EQ・ディエッサー・ゲートの値を「すべて自動設定」で決める。
- フィルタ画面を 7 グループ（ハイパス・ゲート・EQ・コンプ・ディエッサー・出力ゲイン・リミッター）に拡張。EQ はプリセットを選ぶとスライダーが動く。
- 測定にキーボード（4 秒）とマウス（3 秒）の段階を追加（合計約 26 秒）。打鍵音・クリック音の強さでノイズゲートのしきい値とノイズ抑制の強さを決める。
- 日本語のインストーラー（Inno Setup、installer/vao.iss）を追加。入れ先は C:\ProgramData\obs-studio\plugins\VAO（Windows 版 OBS はここだけを読む。%APPDATA% は読まれないことを実機で確認）。
- AI ノイズ抑制（src/dsp/denoiser。RNNoise を obs-deps から静的リンク、48 kHz のみ）を追加。強さは測定したノイズの大きさで決める。licenses/ に RNNoise のライセンス文を同梱。
- tests に test_filters を追加。
- 効きが弱いとの指摘で強化: ハイパスを 24 dB/oct に、EQ の自動補正とプリセットの幅を拡大（範囲 ±12 dB）、ディエッサーを 6:1・最大 15 dB・しきい値 平均+1 dB に。

## 0.1.0（試作・配布なし）
- コンプレッサー・先読みリミッター（src/dsp）、測定（src/calib/measurer）、自動設定の計算（src/calib/auto_settings）を追加。
- フィルタ画面: コンプ／出力ゲイン／リミッターを各グループのチェックボックスでオンオフ。自動設定は「すべて自動設定（測定する）」の1つだけ。
- 設計メモに仕様（1 画面・オンオフ・自動設定・EQ プリセット方針）を追記。
- tests を4本に分割（test_util.h を共通化）。
- 自動設定のコンプを強めた（しきい値 平均-6dB・4:1）、目標音量を -16 dB に変更（音量を押し上げるため）。

## 0.0.0（未公開）
- 開発環境・フォルダ構成・開発ルール（CLAUDE.md、vao-dev スキル、引継ぎ資料）を用意。
- 名前を VAO – Voice Auto Optimizer に決定。
- obs-plugintemplate から作成。buildspec.json にプラグイン名 VAO・作者を設定。
- 音を素通しするフィルタ「VAO Voice」（src/obs）、ゲイン処理 src/dsp/gain、そのテスト tests/test_gain.cpp を追加。
- tools/build.bat・test.bat・install_local.bat を追加。
- CMakePresets.json の Windows SDK 版の固定を外した。
