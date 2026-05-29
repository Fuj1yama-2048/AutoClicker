# Auto Clicker (连点器)

Windows 鼠标连点器，C++17 + Win32 API 原生实现，无外部依赖。

## 功能

- 鼠标左/中/右键单击或双击
- 点击间隔支持时、分、秒、毫秒、微秒精度
- 无限重复或指定次数
- 跟随鼠标或固定坐标点击（坐标支持屏幕取点）
- **热键自定义** — 开始/录制/回放热键可在设置中修改，支持组合键
- **设置持久化** — 所有配置自动保存到注册表，重启不丢失
- 操作录制与回放（支持鼠标轨迹高精度录制）
- 录制文件导入/导出（支持默认文件夹设置）
- 随机延迟、位置微抖（反检测）
- 窗口置顶开关
- 最小化到系统托盘
- 关闭行为可配置（询问 / 托盘 / 退出）

## 编译

### MSVC (Visual Studio 2022)

双击 `src\build.bat`，或命令行：

```
src\build.bat
```

脚本自动查找 MSVC 环境，编译后自动启动。

### MinGW-w64

```
cd src
windres resources.rc resources.o
g++ -O2 -std=c++17 -o ..\bin\AutoClicker.exe main.cpp resources.o -static -lgdi32 -luser32 -lshell32 -lcomdlg32 -ladvapi32 -lwinmm -mwindows
```

## 热键（默认，可自定义）

| 按键 | 功能 |
|------|------|
| F6 | 开始 / 停止点击 |
| F7 | 开始 / 停止录制 |
| F8 | 回放录制 |
| ESC | 取消屏幕取点 |

## 项目结构

```
AutoClicker/
├── src/
│   ├── main.cpp          # 全部源码
│   ├── resources.rc      # 资源定义（图标、版本信息）
│   ├── icon.ico          # 程序图标
│   ├── build.bat         # MSVC/MinGW 编译脚本
│   ├── build_msvc.bat    # MSVC 专用编译脚本
│   └── clear_reg.bat     # 注册表清除（恢复默认设置）
├── bin/
│   └── AutoClicker.exe   # 编译输出
├── .vscode/              # VSCode 任务与调试配置
├── README.md
└── CHANGELOG.md          # 更新日志
```

## 设置存储

所有偏好设置保存在注册表 `HKEY_CURRENT_USER\Software\AutoClicker`，不同 Windows 账户互不干扰。

如需恢复默认设置，运行 `src\clear_reg.bat`。

## 更新日志

详见 [CHANGELOG.md](CHANGELOG.md)
