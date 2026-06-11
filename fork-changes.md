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
