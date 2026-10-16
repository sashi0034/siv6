# このフォークにおける変更点

## MainRuntime

Siv3D の `Main()` エントリポイントを呼び出さずに Siv3D エンジンを初期化する RAII API `s3d::MainRuntime` を追加しました。

これは別のホスト側がエントリポイントを管理する環境において、Siv3D エンジンコンポーネントを利用したい埋め込み用途を想定しています。

```cpp
# include <Siv3D.hpp>

SIV3D_SET(s3d::EngineOption::Renderer::Headless);

void UseSiv3DFromHost()
{
    s3d::MainRuntime runtime;

    const s3d::Font font{ 32, s3d::Typeface::Regular };

    // ...
}
```

現在は `EngineOption::Renderer::Headless` の場合のみ対応しています。


## Ubuntu 24.04 対応

Ubuntu 24.04 のビルドエラーを修正。

`Platform.hpp` の `SIV3D_INTRINSIC(SSE)` 判定で `__SSE4_2__` 要求していた。しかし、`Linux/CMakeLists.txt` では `-msse4.1` に設定されていたので `-msse4.2` に修正しました

## Image::paint() の SSE 実装のバグを修正

`Image::paint` 内の `Paint_SSE4_1()` でクラッシュする問題を修正しました。


## Texture::atlas()

複数の不変テクスチャ領域を共通のテクスチャへ集約する `Texture::atlas()` を追加しました。

```cpp
const TextureRegion region = texture.atlas(Point{ 32, 16 }, Size{ 16, 16 });
region.draw();
```

補足

- `Graphics2D::ClearTextureAtlasCache()` で現在のアトラスキャッシュを消去できます。以前のアトラスから作られた `TextureRegion` は有効なままで、古いアトラスページは参照されなくなった時点で解放されます。
- 現在のネイティブ実装は D3D11 のみです。
- D3D11 では 4096×4096 の固定ページへ領域を配置し、ページが満杯になると次のページを作成します。
- 各領域の周囲 1 ピクセルに、最寄りの端のピクセルを複製したパディングを設けます。
- GPU へのコピーは 2D 描画の flush 直前にまとめて実行します。
- 同じ Texture ID と `Rect` の組み合わせは、キャッシュの消去またはエンジン終了まで再利用されます。
- DynamicTexture、RenderTexture、MSRenderTexture、範囲外または大きすぎる領域はアトラス化せず、通常の `TextureRegion` にフォールバックします。
