# 2D横スクロールアクションゲーム (C++)

C++で動く、シンプルな **2D横スクロールアクションゲーム（ターミナル版）** です。  
Windows / Linux の両方でビルドできるようにしています。

- 左右移動: `A / D`
- ジャンプ: `W / Space`
- 終了: `Q`
- コインを取るとスコア加算
- 敵に触れる、または落下でリスポーン

## 使用技術

- C++17
- 標準ライブラリ（Windows: `conio.h` / Linux: `termios`, `select`）
- CMake（任意）

---

## Linuxで「cmake: command not found」が出る場合

あなたのログのエラーは、**CMakeが未インストール**なのが原因です。

### まずは必要ツールを入れる

Ubuntu / Debian:

```bash
sudo apt update
sudo apt install -y build-essential cmake
```

### CMakeでビルドする場合

```bash
cmake -S . -B build
cmake --build build
./build/side_scroller
```

---

## LinuxでCMakeを使わない方法（代替）

このリポジトリには `Makefile` も追加してあるので、CMakeなしでもビルドできます。

```bash
sudo apt update
sudo apt install -y build-essential
make
./side_scroller
```

---

## Windowsでの実行手順（重要）

よくあるエラー:

- `CMAKE_CXX_COMPILER not set`
- `nmake ... failed`
- `./build/side_scroller` が認識されない

は、主に **(1) C++コンパイラ環境未設定** と **(2) 実行コマンドがLinux形式** が原因です。

### 1) C++コンパイラを用意する

- **Visual Studio 2022**（推奨）
  - 「Desktop development with C++」ワークロードを有効化
- または **Build Tools for Visual Studio**
  - 同様にMSVCツールチェーンを有効化

### 2) 正しいシェルでCMakeを実行

- おすすめ: **x64 Native Tools Command Prompt for VS 2022**

### 3) configure / build / run

```bat
cmake -S . -B build -G "NMake Makefiles"
cmake --build build
build\side_scroller.exe
```

> Windowsでは `./build/side_scroller` ではなく `build\side_scroller.exe` を使います。

### 4) `cmake --install` は必要？

通常実行に **install不要** です。`build\side_scroller.exe` を直接起動してください。

---

## コードの入口

- `src/main.cpp`

ゲームロジック（重力、当たり判定、カメラ追従、敵の往復移動）は1ファイルにまとめています。
