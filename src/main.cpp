#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ole32.lib")
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <mmsystem.h>

// ── 控件 ID ──────────────────────────────────────
#define IDC_BTN_LEFT              1001  // 鼠标左键
#define IDC_BTN_RIGHT             1002  // 鼠标右键
#define IDC_BTN_MIDDLE            1003  // 鼠标中键
#define IDC_SINGLE                1004  // 单击模式
#define IDC_DOUBLE                1005  // 双击模式
#define IDC_EDIT_SEC              1006  // 间隔秒数输入
#define IDC_EDIT_MS               1007  // 间隔毫秒输入
#define IDC_EDIT_US               1008  // 间隔微秒输入
#define IDC_REPEAT_FOREVER        1010  // 直到停止
#define IDC_REPEAT_COUNT          1011  // 指定次数
#define IDC_EDIT_COUNT            1012  // 次数输入
#define IDC_CURSOR_FOLLOW         1013  // 跟随光标
#define IDC_CURSOR_FIXED          1014  // 固定位置
#define IDC_EDIT_X                1015  // 固定X坐标
#define IDC_EDIT_Y                1016  // 固定Y坐标
#define IDC_BTN_START             1018  // 开始按钮
#define IDC_BTN_STOP              1019  // 停止按钮
#define IDC_COUNT_LABEL           1021  // 点击计数标签
#define IDC_STATUS_LABEL          1022  // 状态标签
#define IDC_CHECK_TOPMOST         1023  // 置顶
#define IDC_BTN_RECORD            1024  // 录制按钮
#define IDC_BTN_REPLAY            1025  // 回放按钮
#define IDC_BTN_CLEAR_REC         1026  // 清空录制
#define IDC_LIST_RECORD           1027  // 动作列表
#define IDC_REPLAY_LOOP           1028  // 循环播放
#define IDC_EDIT_REPLAY_N         1029  // 播放次数
#define IDC_LABEL_RECORD          1030  // 当前动作数标签
#define IDC_BTN_EXPORT            1031  // 导出按钮
#define IDC_BTN_IMPORT            1032  // 导入按钮
#define IDC_LIST_IMPORTED         1033  // 已导入文件列表
#define IDC_CHECK_TRAJECTORY      1034  // 记录轨迹
#define IDC_CHECK_HIDE            1035  // 开始时隐藏
#define IDC_COMBO_SAMPLE_RATE     1036  // 采样率
#define IDC_COMBO_CLOSE_ACTION    1037  // 关闭行为
#define IDC_CHECK_RAND_DELAY      1038  // 随机间隔
#define IDC_CHECK_RAND_POS        1039  // 位置微抖
#define IDC_CHECK_RECORD_SELF     1040  // 录制自窗口光标
#define IDC_BTN_PICK_POS          1041  // 取点按钮
#define IDC_BTN_SETTINGS          1042  // 齿轮设置按钮
#define IDC_SETTINGS_OK           1043  // 设置确定按钮
#define IDC_BTN_HK_START          1044  // 热键显示-开始
#define IDC_BTN_HK_RECORD         1045  // 热键显示-录制
#define IDC_BTN_HK_REPLAY         1046  // 热键显示-回放
#define IDC_HOTKEY_LABEL          1047  // 底部热键标签
#define IDC_EDIT_DEFAULT_PATH     1048  // 默认文件路径
#define IDC_BTN_BROWSE_PATH       1049  // 浏览文件夹按钮
#define IDC_TIMER                    1  // 连点定时器
#define IDC_REPLAY_TIMER             2  // 回放定时器
#define WM_TRAYICON          (WM_APP + 1) // 托盘消息

// ── 窗口大小 ──────────────────────────────────────
#define MAIN_HEIGHT 730
#define MAIN_WIDTH  465

#define SET_HEIGHT  550
#define SET_WIDTH   380


// ── 版本号 ──────────────────────────────────────
#define VERSION_NUMBER  L"1.1.1"

// ── 录制数据结构 ─────────────────────────────────
struct RecordedAction {
    int   type;        // 0=单击 1=拖拽 2=轨迹拖拽
    int   button;      // 0=左键 1=右键 2=中键
    DWORD delayMs;     // 距上一个动作的延迟
    POINT pt;          // 单击坐标 / 拖拽起始坐标
    POINT endPt;       // 拖拽终点坐标
    DWORD holdMs;      // 拖拽按住时长(ms)
    std::vector<POINT> trajectory; // 轨迹中间点 (type==2)
};

// 完整轨迹点（从录制开始到结束的所有采样 + 事件）
struct TimelinePoint {
    POINT pt;          // 鼠标位置
    DWORD64 deltaUs;   // 距上一个采样点的时间(微秒)
    int   event;       // 0=移动 -1=按下 -2=释放
    int   button;      // 0=左键 1=右键 2=中键 (仅event!=0时有效)
};

struct ImportedFile {
    std::wstring path;
    std::wstring name;
    std::vector<RecordedAction> actions;
};

// ── QPC 高精度计时辅助 ────────────────────────────
DWORD64 GetTimeUs() {
    static LARGE_INTEGER freq;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (now.QuadPart * 1000000ULL) / freq.QuadPart;
}

// ── 全局 ─────────────────────────────────────────
HINSTANCE g_hInst = nullptr;

struct AppState {
    int  mouseButton = 0;
    int  clickMode = 0;
    int  intervalUs = 1000000;   // 间隔 (微秒), 默认1秒
    bool repeatForever = true;
    int  targetCount = 100;
    bool followCursor = true;
    int  fixedX = 0;
    int  fixedY = 0;
    bool topmost = true; // 置顶行为
    bool pickingPosition = false;  // 正在等待用户点击屏幕选取位置

    // 运行时
    bool running = false;
    int  clickCount = 0;

    // 录制
    bool isRecording = false;
    bool isReplaying = false;
    bool replayLoop = false;
    int  replayCount = 1;
    int  currentReplay = 0;
    size_t replayIndex = 0;
    DWORD lastRecordTick = 0;
    std::vector<RecordedAction> actions;
    HHOOK mouseHook = nullptr;

    // 拖拽录制状态
    bool  dragActive = false;
    int   dragButton = 0;
    DWORD dragDownTick = 0;
    POINT dragStartPt = {0, 0};
    bool  dragMoved = false;

    // 轨迹录制
    bool  recordTrajectory = false;
    bool  recordSelfWin = false;     // 录制鼠标在软件界面上的移动
    RECT  selfRect = {};             // 录制开始时缓存的窗口矩形
    bool  hideOnAction = false;
    int   sampleRateHz = 1000;    // 轨迹采样率 Hz
    DWORD lastTrajTick = 0;
    std::vector<POINT> trajBuffer;

    // 完整轨迹 (始终记录鼠标移动)
    std::vector<TimelinePoint> timeline;
    DWORD64 lastTimelineUs = 0;
    size_t timelineReplayIdx = 0;
    bool replayingTimeline = false;
    DWORD64 replayBaseUs = 0;         // 回放起始时刻 (GetTimeUs)
    DWORD64 replayAccumUs = 0;        // 已回放到的累计微秒
    UINT    mmReplayTimerId = 0;      // 回放用多媒体定时器

    // 反检测
    bool randomizeInterval = true;  // 随机点击间隔
    bool randomizePosition = true;  // 鼠标位置微抖

    // 关闭行为 & 托盘
    int  closeAction = 0; // 0=询问, 1=托盘, 2=退出
    bool inTray = false;
    NOTIFYICONDATA nid = {};

    // 已导入文件库
    std::vector<ImportedFile> importedFiles;
    int  selectedImport = -1;  // -1 = 当前录制 (非导入文件)

    HWND hwnd = nullptr;
    HWND hList = nullptr;
    HWND hImportedList = nullptr;

    // 防止录制回调里触发 replay/click 被录进去
    bool ignoreNextDown = false;
    bool settingRadio = false;   // 抑制 CheckRadioButton 的异步通知

    // 自定义热键
    UINT hotkeyStart  = VK_F6;   UINT hkModStart  = 0;
    UINT hotkeyRecord = VK_F7;   UINT hkModRecord = 0;
    UINT hotkeyReplay = VK_F8;   UINT hkModReplay = 0;

    // 设置窗口
    HWND hwndSettings = nullptr;
    bool capturingHotkey = false;
    int  capturingWhich = 0;  // 1=开始, 2=录制, 3=回放

    // 导入导出默认路径
    std::wstring defaultFilePath;
} g;

HHOOK g_pickHook = nullptr;

// ── 辅助函数 ─────────────────────────────────────
int RandomRange(int lo, int hi) {
    return lo + (rand() % (hi - lo + 1));
}

UINT GetMouseDownFlag() {
    switch (g.mouseButton) {
        case 0: return MOUSEEVENTF_LEFTDOWN;
        case 1: return MOUSEEVENTF_RIGHTDOWN;
        case 2: return MOUSEEVENTF_MIDDLEDOWN;
        default: return MOUSEEVENTF_LEFTDOWN;
    }
}

UINT GetMouseUpFlag() {
    switch (g.mouseButton) {
        case 0: return MOUSEEVENTF_LEFTUP;
        case 1: return MOUSEEVENTF_RIGHTUP;
        case 2: return MOUSEEVENTF_MIDDLEUP;
        default: return MOUSEEVENTF_LEFTUP;
    }
}

void FixEmptyNumericEdit(HWND hwnd, int id) {
    wchar_t buf[16];
    GetDlgItemText(hwnd, id, buf, 16);
    if (buf[0] == L'\0' || (buf[0] == L'-' && buf[1] == L'\0')) {
        SetDlgItemText(hwnd, id, L"0");
    }
}

int ReadInterval(HWND hwnd) {
    wchar_t buf[16];
    int s = 0, ms = 0, us = 0;
    GetDlgItemText(hwnd, IDC_EDIT_SEC, buf, 16);  s = _wtoi(buf);
    GetDlgItemText(hwnd, IDC_EDIT_MS, buf, 16);   ms = _wtoi(buf);
    GetDlgItemText(hwnd, IDC_EDIT_US, buf, 16);    us = _wtoi(buf);
    int totalUs = (s * 1000000) + (ms * 1000) + us;
    g.intervalUs = totalUs > 1000 ? totalUs : 1000; // 最小 1μs... actually 1ms for timer
    return g.intervalUs;
}

void ApplyInterval(HWND hwnd) {
    int us = ReadInterval(hwnd);
    if (g.running) {
        KillTimer(hwnd, IDC_TIMER);
        int ms = us / 1000;
        if (ms < 1) ms = 1;
        SetTimer(hwnd, IDC_TIMER, ms, nullptr);
    }
}

void GetMouseFlags(int button, UINT& down, UINT& up) {
    switch (button) {
        case 0: down = MOUSEEVENTF_LEFTDOWN;  up = MOUSEEVENTF_LEFTUP;   break;
        case 1: down = MOUSEEVENTF_RIGHTDOWN; up = MOUSEEVENTF_RIGHTUP;  break;
        case 2: down = MOUSEEVENTF_MIDDLEDOWN; up = MOUSEEVENTF_MIDDLEUP; break;
        default: down = 0; up = 0; break;
    }
}

void SimulateClick(int button, int mode, POINT pt) {
    UINT down, up;
    GetMouseFlags(button, down, up);
    if (!down) return;

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    LONG dx = (LONG)((pt.x * 65536LL) / sw);
    LONG dy = (LONG)((pt.y * 65536LL) / sh);

    // 移动+按下+释放 放入一次 SendInput 调用，保证不会和用户物理鼠标输入交叉
    if (mode == 0) { // 单击
        SetCursorPos(pt.x, pt.y);
        INPUT in[3] = {};
        in[0].type = INPUT_MOUSE;
        in[0].mi.dx = dx; in[0].mi.dy = dy;
        in[0].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
        in[1].type = INPUT_MOUSE;
        in[1].mi.dwFlags = down;
        in[2].type = INPUT_MOUSE;
        in[2].mi.dwFlags = up;
        SendInput(3, in, sizeof(INPUT));
    } else { // 双击
        SetCursorPos(pt.x, pt.y);
        INPUT in[5] = {};
        in[0].type = INPUT_MOUSE;
        in[0].mi.dx = dx; in[0].mi.dy = dy;
        in[0].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
        in[1].type = INPUT_MOUSE;
        in[1].mi.dwFlags = down;
        in[2].type = INPUT_MOUSE;
        in[2].mi.dwFlags = up;
        // 双击的两击之间 Windows 需要约 30ms 间隔来识别为双击
        SendInput(3, in, sizeof(INPUT));
        Sleep(30);
        in[3].type = INPUT_MOUSE;
        in[3].mi.dwFlags = down;
        in[4].type = INPUT_MOUSE;
        in[4].mi.dwFlags = up;
        SendInput(2, &in[3], sizeof(INPUT));
    }
}

void SimulateDrag(int button, POINT from, POINT to, DWORD holdMs) {
    UINT down, up;
    GetMouseFlags(button, down, up);
    if (!down) return;

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    INPUT in = {};
    in.type = INPUT_MOUSE;

    SetCursorPos(from.x, from.y);
    LONG dx = (LONG)((from.x * 65536LL) / sw);
    LONG dy = (LONG)((from.y * 65536LL) / sh);
    in.mi.dx = dx; in.mi.dy = dy;
    in.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | down;
    SendInput(1, &in, sizeof(INPUT));
    Sleep(20);

    SetCursorPos(to.x, to.y);
    dx = (LONG)((to.x * 65536LL) / sw);
    dy = (LONG)((to.y * 65536LL) / sh);
    in.mi.dx = dx; in.mi.dy = dy;
    in.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
    SendInput(1, &in, sizeof(INPUT));

    DWORD elapsed = 20;
    if (holdMs > elapsed) Sleep(holdMs - elapsed);

    in.mi.dwFlags = up;
    SendInput(1, &in, sizeof(INPUT));
    Sleep(10);
}

void SimulateTrajectoryDrag(const RecordedAction& a) {
    UINT down, up;
    GetMouseFlags(a.button, down, up);
    if (!down) return;

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    INPUT in = {};
    in.type = INPUT_MOUSE;

    SetCursorPos(a.pt.x, a.pt.y);
    LONG dx = (LONG)((a.pt.x * 65536LL) / sw);
    LONG dy = (LONG)((a.pt.y * 65536LL) / sh);
    in.mi.dx = dx; in.mi.dy = dy;
    in.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | down;
    SendInput(1, &in, sizeof(INPUT));
    Sleep(15);

    int nPts = (int)a.trajectory.size() + 1;
    DWORD interval = a.holdMs / (nPts > 0 ? nPts : 1);
    if (interval < 3) interval = 3;
    if (interval > 50) interval = 50;

    for (auto& pt : a.trajectory) {
        Sleep(interval);
        SetCursorPos(pt.x, pt.y);
        dx = (LONG)((pt.x * 65536LL) / sw);
        dy = (LONG)((pt.y * 65536LL) / sh);
        in.mi.dx = dx; in.mi.dy = dy;
        in.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
        SendInput(1, &in, sizeof(INPUT));
    }

    Sleep(interval);
    SetCursorPos(a.endPt.x, a.endPt.y);
    dx = (LONG)((a.endPt.x * 65536LL) / sw);
    dy = (LONG)((a.endPt.y * 65536LL) / sh);
    in.mi.dx = dx; in.mi.dy = dy;
    in.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
    SendInput(1, &in, sizeof(INPUT));
    Sleep(10);

    in.mi.dwFlags = up;
    SendInput(1, &in, sizeof(INPUT));
    Sleep(10);
}

void SimulateAction(const RecordedAction& a) {
    if (a.type == 0) {
        SimulateClick(a.button, 0, a.pt);
    } else if (a.type == 1) {
        SimulateDrag(a.button, a.pt, a.endPt, a.holdMs);
    } else if (a.type == 2) {
        SimulateTrajectoryDrag(a);
    }
}

// 诊断: 记录最近一次点击的模式和坐标, UI中显示
int  g_lastClickMode = -1;   // -1=未点击, 0=跟随, 1=固定
POINT g_lastClickPt = {0, 0};

// 固定位置专用: 通过 PostMessage 直接把鼠标消息投递到目标窗口。
// 完全不移动光标，从根源上避免和用户物理鼠标争抢同一光标资源。
void ClickAtFixedPos(POINT pt, int button, int mode) {
    HWND hTarget = WindowFromPoint(pt);
    if (!hTarget) return;

    POINT cp = pt;
    ScreenToClient(hTarget, &cp);

    UINT downMsg, upMsg;
    WPARAM wpDown;
    switch (button) {
        case 0:
            downMsg = WM_LBUTTONDOWN; upMsg = WM_LBUTTONUP;
            wpDown = MK_LBUTTON; break;
        case 1:
            downMsg = WM_RBUTTONDOWN; upMsg = WM_RBUTTONUP;
            wpDown = MK_RBUTTON; break;
        case 2:
            downMsg = WM_MBUTTONDOWN; upMsg = WM_MBUTTONUP;
            wpDown = MK_MBUTTON; break;
        default: return;
    }

    LPARAM lp = MAKELPARAM(cp.x, cp.y);
    int times = (mode == 0) ? 1 : 2;
    for (int t = 0; t < times; t++) {
        PostMessage(hTarget, downMsg, wpDown, lp);
        PostMessage(hTarget, upMsg, 0, lp);
        if (t < times - 1) Sleep(30);
    }
}

void DoClick() {
    POINT pt;
    bool isFixed = (IsDlgButtonChecked(g.hwnd, IDC_CURSOR_FIXED) == BST_CHECKED);
    if (isFixed) {
        pt.x = g.fixedX;
        pt.y = g.fixedY;
    } else {
        GetCursorPos(&pt);
    }
    g_lastClickMode = isFixed ? 1 : 0;
    g_lastClickPt = pt;
    if (g.randomizePosition) {
        pt.x += RandomRange(-4, 4);
        pt.y += RandomRange(-4, 4);
    }
    if (isFixed) {
        ClickAtFixedPos(pt, g.mouseButton, g.clickMode);
    } else {
        SimulateClick(g.mouseButton, g.clickMode, pt);
        if (g.randomizePosition) SetCursorPos(g_lastClickPt.x, g_lastClickPt.y);
    }
}

// ── 录制 ────────────────────────────────────────
void StopReplay(HWND hwnd);   // 前置声明
void StopClicking(HWND hwnd); // 前置声明
void UpdateUI(HWND hwnd);     // 前置声明
std::wstring HotkeyToString(UINT vk, UINT mod);
void RefreshRecordList(HWND hwnd) {
    SendMessage(g.hList, LB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < g.actions.size(); i++) {
        auto& a = g.actions[i];
        const wchar_t* btn = (a.button == 0) ? L"左键" : (a.button == 1) ? L"右键" : L"中键";
        wchar_t line[128];
        if (a.type == 2) {
            swprintf(line, 128, L"#%zu  轨迹拖拽%s  (%d,%d)→(%d,%d)  %zu点 %lums",
                     i + 1, btn, a.pt.x, a.pt.y,
                     a.endPt.x, a.endPt.y, a.trajectory.size(), a.holdMs);
        } else if (a.type == 1) {
            swprintf(line, 128, L"#%zu  拖拽%s  (%d,%d)→(%d,%d)  按住%lums",
                     i + 1, btn, a.pt.x, a.pt.y, a.endPt.x, a.endPt.y, a.holdMs);
        } else {
            swprintf(line, 128, L"#%zu  单击%s  延迟:%lums  (%d, %d)",
                     i + 1, btn, a.delayMs, a.pt.x, a.pt.y);
        }
        SendMessage(g.hList, LB_ADDSTRING, 0, (LPARAM)line);
    }
    int cnt = (int)SendMessage(g.hList, LB_GETCOUNT, 0, 0);
    if (cnt > 0) SendMessage(g.hList, LB_SETTOPINDEX, cnt - 1, 0);

    // 更新录制计数标签
    std::wstringstream ss;
    ss << L"当前动作: " << g.actions.size() << L" 个";
    if (!g.timeline.empty()) ss << L"  轨迹点: " << g.timeline.size();
    SetDlgItemText(hwnd, IDC_LABEL_RECORD, ss.str().c_str());
}

void RefreshImportedList(HWND hwnd) {
    SendMessage(g.hImportedList, LB_RESETCONTENT, 0, 0);
    // 第一个条目始终是"当前录制"
    int idx = (int)SendMessage(g.hImportedList, LB_ADDSTRING, 0,
                                (LPARAM)L"<当前录制>");
    SendMessage(g.hImportedList, LB_SETITEMDATA, idx, -1);

    for (size_t i = 0; i < g.importedFiles.size(); i++) {
        std::wstring line = g.importedFiles[i].name + L"  (" +
                            std::to_wstring(g.importedFiles[i].actions.size()) + L"个动作)";
        idx = (int)SendMessage(g.hImportedList, LB_ADDSTRING, 0, (LPARAM)line.c_str());
        SendMessage(g.hImportedList, LB_SETITEMDATA, idx, (LPARAM)i);
    }

    // 选中当前项
    int sel = g.selectedImport + 1; // +1 因为第0项是"当前录制"
    SendMessage(g.hImportedList, LB_SETCURSEL, sel, 0);
}

void StartRecording(HWND hwnd) {
    if (g.isRecording) return;
    if (g.running) StopClicking(hwnd);
    if (g.isReplaying) StopReplay(hwnd);
    g.actions.clear();
    g.selectedImport = -1;
    g.timeline.clear();
    g.lastTimelineUs = 0;
    g.lastRecordTick = GetTickCount();
    g.recordTrajectory = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_TRAJECTORY), BM_GETCHECK, 0, 0) == BST_CHECKED);
    g.isRecording = true;
    UpdateUI(hwnd);
    if (g.hideOnAction) ShowWindow(hwnd, SW_MINIMIZE);

    // 缓存窗口矩形, 用于快速判断鼠标是否在自家窗口上 (避免 WM_MOUSEMOVE 热路径调 WindowFromPoint)
    GetWindowRect(g.hwnd, &g.selfRect);

    g.mouseHook = SetWindowsHookEx(WH_MOUSE_LL, [](int nCode, WPARAM wParam, LPARAM lp) -> LRESULT {
        if (nCode >= 0 && g.isRecording) {
            MSLLHOOKSTRUCT* info = (MSLLHOOKSTRUCT*)lp;

            if (g.ignoreNextDown) return CallNextHookEx(g.mouseHook, nCode, wParam, lp);

            // ── 完整轨迹: 记录所有真实鼠标移动事件 ──
            // 热路径优化: 用快速矩形判断替代 WindowFromPoint, 避免丢帧
            if (g.recordTrajectory && wParam == WM_MOUSEMOVE) {
                bool overSelf = !g.recordSelfWin &&
                    (info->pt.x >= g.selfRect.left && info->pt.x < g.selfRect.right &&
                     info->pt.y >= g.selfRect.top && info->pt.y < g.selfRect.bottom);
                if (!overSelf) {
                    DWORD64 nowUs = GetTimeUs();
                    bool record = g.timeline.empty();
                    if (!record && g.sampleRateHz >= 1000) {
                        record = true;
                    } else if (!record) {
                        int minUs = 1000000 / g.sampleRateHz;
                        record = (nowUs - g.lastTimelineUs >= (DWORD64)minUs);
                    }
                    if (record) {
                        TimelinePoint tp;
                        tp.pt = info->pt;
                        tp.deltaUs = g.timeline.empty() ? 0 : (nowUs - g.lastTimelineUs);
                        tp.event = 0;
                        tp.button = 0;
                        g.timeline.push_back(tp);
                        g.lastTimelineUs = nowUs;
                    }
                }
            }

            int btn = -1;
            if      (wParam == WM_LBUTTONDOWN) btn = 0;
            else if (wParam == WM_RBUTTONDOWN) btn = 1;
            else if (wParam == WM_MBUTTONDOWN) btn = 2;

            if (btn >= 0) {
                // WindowFromPoint 仅用于点击事件 (稀少, 不影响帧率)
                HWND hWndAtPt = WindowFromPoint(info->pt);
                bool overSelf = (hWndAtPt == g.hwnd || IsChild(g.hwnd, hWndAtPt));
                if (!overSelf) {
                    // 记录按下事件到完整轨迹
                    if (g.recordTrajectory) {
                        DWORD64 nowUs = GetTimeUs();
                        TimelinePoint tp;
                        tp.pt = info->pt;
                        tp.deltaUs = g.timeline.empty() ? 0 : (nowUs - g.lastTimelineUs);
                        tp.event = -1;
                        tp.button = btn;
                        g.timeline.push_back(tp);
                        g.lastTimelineUs = nowUs;
                    }

                    // 按下：开始追踪拖拽
                    g.dragActive = true;
                    g.dragButton = btn;
                    g.dragDownTick = GetTickCount();
                    g.dragStartPt = info->pt;
                    g.dragMoved = false;
                    g.trajBuffer.clear();
                    g.lastTrajTick = g.dragDownTick;
                }
            }
            else if (wParam == WM_MOUSEMOVE && g.dragActive) {
                if (abs(info->pt.x - g.dragStartPt.x) > 3 ||
                    abs(info->pt.y - g.dragStartPt.y) > 3) {
                    g.dragMoved = true;
                }
                if (g.recordTrajectory && g.dragMoved) {
                    DWORD now = GetTickCount();
                    int trajMs = 1000 / g.sampleRateHz;
                    if (trajMs < 1) trajMs = 1;
                    bool timeOk = (now - g.lastTrajTick >= (DWORD)trajMs);
                    bool distOk = g.trajBuffer.empty() ||
                        (abs(info->pt.x - g.trajBuffer.back().x) > 4 ||
                         abs(info->pt.y - g.trajBuffer.back().y) > 4);
                    if (timeOk && distOk) {
                        g.lastTrajTick = now;
                        g.trajBuffer.push_back(info->pt);
                    }
                }
            }
            else if ((wParam == WM_LBUTTONUP   && g.dragButton == 0) ||
                     (wParam == WM_RBUTTONUP   && g.dragButton == 1) ||
                     (wParam == WM_MBUTTONUP   && g.dragButton == 2)) {
                if (g.dragActive) {
                    HWND hWndAtPt2 = WindowFromPoint(info->pt);
                    bool overSelf2 = (hWndAtPt2 == g.hwnd || IsChild(g.hwnd, hWndAtPt2));
                    if (!overSelf2) {
                        RecordedAction a;
                        a.delayMs = g.dragDownTick - g.lastRecordTick;
                        g.lastRecordTick = GetTickCount();
                        a.button = g.dragButton;
                        a.pt = g.dragStartPt;

                        if (g.dragMoved) {
                            a.endPt = info->pt;
                            a.holdMs = GetTickCount() - g.dragDownTick;
                            if (g.recordTrajectory && !g.trajBuffer.empty()) {
                                a.type = 2;
                                a.trajectory = std::move(g.trajBuffer);
                            } else {
                                a.type = 1;
                            }
                        } else {
                            a.type = 0;
                        }
                        g.trajBuffer.clear();
                        g.actions.push_back(a);

                        if (g.recordTrajectory) {
                            DWORD64 nowUs = GetTimeUs();
                            TimelinePoint tp;
                            tp.pt = info->pt;
                            tp.deltaUs = g.timeline.empty() ? 0 : (nowUs - g.lastTimelineUs);
                            tp.event = -2;
                            tp.button = g.dragButton;
                            g.timeline.push_back(tp);
                            g.lastTimelineUs = nowUs;
                        }

                        PostMessage(g.hwnd, WM_APP, 0, 0);
                    }
                }
                g.dragActive = false;
            }
        }
        return CallNextHookEx(g.mouseHook, nCode, wParam, lp);
    }, GetModuleHandle(nullptr), 0);

    SetDlgItemText(hwnd, IDC_BTN_RECORD, L"■ 停止录制");
    RefreshRecordList(hwnd);
    RefreshImportedList(hwnd);
}

void StopRecording(HWND hwnd) {
    if (!g.isRecording) return;
    if (g.mouseHook) {
        UnhookWindowsHookEx(g.mouseHook);
        g.mouseHook = nullptr;
    }
    g.isRecording = false;
    UpdateUI(hwnd);
    std::wstring recText = L"● 录制(" + HotkeyToString(g.hotkeyRecord, g.hkModRecord) + L")";
    SetDlgItemText(hwnd, IDC_BTN_RECORD, recText.c_str());
    RefreshRecordList(hwnd);
}

void ClearRecording(HWND hwnd) {
    if (g.isReplaying) return;
    StopRecording(hwnd);
    g.actions.clear();
    g.timeline.clear();
    g.selectedImport = -1;
    RefreshRecordList(hwnd);
    RefreshImportedList(hwnd);
}

void ExportRecording(HWND hwnd) {
    if (g.actions.empty() && g.timeline.empty()) {
        MessageBox(hwnd, L"没有可导出的录制数据。", L"导出失败", MB_ICONWARNING);
        return;
    }

    wchar_t path[MAX_PATH] = L"recording.acr";
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"连点器录制文件 (*.acr)\0*.acr\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"acr";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!g.defaultFilePath.empty())
        ofn.lpstrInitialDir = g.defaultFilePath.c_str();

    if (!GetSaveFileName(&ofn)) return;

    HANDLE hFile = CreateFile(path, GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBox(hwnd, L"无法创建文件。", L"导出失败", MB_ICONERROR);
        return;
    }

    // 写入 UTF-8 BOM
    unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
    DWORD written;
    WriteFile(hFile, bom, 3, &written, nullptr);

    // 写入文件头
    char header[256];
    int len = sprintf(header,
        "# 连点器录制文件 v5\r\n"
        "# 单击: 0 delayMs button x y 0 0 0 0\r\n"
        "# 拖拽: 1 delayMs button x1 y1 x2 y2 holdMs 0\r\n"
        "# 轨迹: 2 delayMs button x1 y1 x2 y2 holdMs N\r\n"
        "#   (后跟N行 : x y 表示轨迹中间点)\r\n"
        "# button: 0=左键 1=右键 2=中键\r\n"
        "# @TIMELINE N 后跟完整轨迹采样 (N行)\r\n"
        "#   格式: deltaUs x y event button (deltaUs单位=微秒)\r\n"
        "#   event: 0=移动 -1=按下 -2=释放\r\n#\r\n");
    WriteFile(hFile, header, len, &written, nullptr);

    for (auto& a : g.actions) {
        char line[256];
        int nPts = (int)a.trajectory.size();
        len = sprintf(line, "%d %lu %d %ld %ld %ld %ld %lu %d\r\n",
                      a.type, a.delayMs, a.button, a.pt.x, a.pt.y,
                      a.endPt.x, a.endPt.y, a.holdMs, nPts);
        WriteFile(hFile, line, len, &written, nullptr);
        for (auto& pt : a.trajectory) {
            len = sprintf(line, ": %ld %ld\r\n", pt.x, pt.y);
            WriteFile(hFile, line, len, &written, nullptr);
        }
    }

    // 完整轨迹 @TIMELINE
    if (!g.timeline.empty()) {
        char tline[128];
        len = sprintf(tline, "@TIMELINE %zu\r\n", g.timeline.size());
        WriteFile(hFile, tline, len, &written, nullptr);
        for (auto& tp : g.timeline) {
            len = sprintf(tline, "%llu %ld %ld %d %d\r\n",
                          tp.deltaUs, tp.pt.x, tp.pt.y, tp.event, tp.button);
            WriteFile(hFile, tline, len, &written, nullptr);
        }
    }

    CloseHandle(hFile);

    std::wstringstream ss;
    ss << L"已导出 " << g.actions.size() << L" 个动作";
    if (!g.timeline.empty()) ss << L", " << g.timeline.size() << L" 个轨迹点";
    ss << L" 到:\n" << path;
    MessageBox(hwnd, ss.str().c_str(), L"导出成功", MB_ICONINFORMATION);
}

void ImportRecording(HWND hwnd) {
    wchar_t path[MAX_PATH] = L"";
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"连点器录制文件 (*.acr)\0*.acr\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"acr";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!g.defaultFilePath.empty())
        ofn.lpstrInitialDir = g.defaultFilePath.c_str();

    if (!GetOpenFileName(&ofn)) return;

    HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBox(hwnd, L"无法打开文件。", L"导入失败", MB_ICONERROR);
        return;
    }

    DWORD size = GetFileSize(hFile, nullptr);
    if (size == INVALID_FILE_SIZE || size > 10 * 1024 * 1024) {
        CloseHandle(hFile);
        MessageBox(hwnd, L"文件无效或过大。", L"导入失败", MB_ICONERROR);
        return;
    }

    std::vector<char> buf(size + 1);
    DWORD read;
    ReadFile(hFile, buf.data(), size, &read, nullptr);
    CloseHandle(hFile);
    buf[read] = '\0';

    // 跳过 UTF-8 BOM
    char* p = buf.data();
    if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF)
        p += 3;

    std::vector<RecordedAction> imported;
    std::vector<TimelinePoint> importedTimeline;
    char* line = p;
    while (*line) {
        // 跳过空行和注释
        if (*line == '#' || *line == '\r' || *line == '\n') {
            while (*line && *line != '\n') line++;
            if (*line == '\n') line++;
            continue;
        }

        // 完整轨迹 @TIMELINE N
        if (strncmp(line, "@TIMELINE", 9) == 0) {
            size_t tcount = 0;
            sscanf(line + 9, "%zu", &tcount);
            while (*line && *line != '\n') line++;
            if (*line == '\n') line++;
            for (size_t i = 0; i < tcount && *line; i++) {
                while ((*line == '\r' || *line == '\n') && *line) {
                    if (*line == '\n') line++;
                }
                if (!*line) break;
                DWORD64 dUs = 0;
                LONG tx = 0, ty = 0;
                int ev = 0, bt = 0;
                if (sscanf(line, "%llu %ld %ld %d %d", &dUs, &tx, &ty, &ev, &bt) >= 5) {
                    TimelinePoint tp;
                    tp.deltaUs = dUs;
                    tp.pt.x = tx;
                    tp.pt.y = ty;
                    tp.event = ev;
                    tp.button = bt;
                    importedTimeline.push_back(tp);
                }
                while (*line && *line != '\n') line++;
                if (*line == '\n') line++;
            }
            continue;
        }

        // 轨迹中间点 (: x y)
        if (*line == ':') {
            LONG tx = 0, ty = 0;
            if (sscanf(line, ": %ld %ld", &tx, &ty) == 2) {
                if (!imported.empty()) {
                    POINT pt = {tx, ty};
                    imported.back().trajectory.push_back(pt);
                }
            }
            while (*line && *line != '\n') line++;
            if (*line == '\n') line++;
            continue;
        }

        int type = 0, nPts = 0;
        DWORD delayMs = 0, holdMs = 0;
        int btn = 0;
        LONG px = 0, py = 0, ex = 0, ey = 0;

        int n = sscanf(line, "%d %lu %d %ld %ld %ld %ld %lu %d",
                       &type, &delayMs, &btn, &px, &py, &ex, &ey, &holdMs, &nPts);

        if (n >= 4) {
            RecordedAction a;
            if (n == 4) {
                // v1: delayMs button x y
                a.type = 0;
                a.delayMs = (DWORD)type;
                a.button = (int)delayMs;
                a.pt.x = (LONG)btn;
                a.pt.y = (LONG)px;
            } else {
                // v2/v3/v4: type delayMs button x y endX endY holdMs [nPts]
                a.type = type;
                a.delayMs = delayMs;
                a.button = btn;
                a.pt.x = px;
                a.pt.y = py;
                a.endPt.x = ex;
                a.endPt.y = ey;
                a.holdMs = holdMs;
            }
            imported.push_back(a);
        }

        while (*line && *line != '\n') line++;
        if (*line == '\n') line++;
    }

    if (imported.empty() && importedTimeline.empty()) {
        MessageBox(hwnd, L"文件中没有有效的录制数据。",
                          L"导入失败", MB_ICONWARNING);
        return;
    }

    // 停止当前录制/播放
    if (g.isRecording) StopRecording(hwnd);
    if (g.isReplaying) StopReplay(hwnd);

    // 检查是否已导入过该文件 (去重)
    int existingIdx = -1;
    for (size_t i = 0; i < g.importedFiles.size(); i++) {
        if (_wcsicmp(g.importedFiles[i].path.c_str(), path) == 0) {
            existingIdx = (int)i;
            break;
        }
    }

    // 提取文件名
    std::wstring fullPath(path);
    std::wstring fileName = fullPath;
    size_t slash = fullPath.find_last_of(L"\\/");
    if (slash != std::wstring::npos) fileName = fullPath.substr(slash + 1);

    ImportedFile f;
    f.path = path;
    f.name = fileName;
    f.actions = imported;

    if (existingIdx >= 0) {
        g.importedFiles[existingIdx] = std::move(f);
        g.selectedImport = existingIdx;
    } else {
        g.importedFiles.push_back(std::move(f));
        g.selectedImport = (int)g.importedFiles.size() - 1;
    }

    g.actions = imported;
    g.timeline = importedTimeline;
    RefreshRecordList(hwnd);
    RefreshImportedList(hwnd);

    std::wstringstream ss;
    ss << L"已导入 " << g.actions.size() << L" 个动作";
    if (!g.timeline.empty()) ss << L", " << g.timeline.size() << L" 个轨迹点";
    ss << L"。";
    MessageBox(hwnd, ss.str().c_str(), L"导入成功", MB_ICONINFORMATION);
}

// 回放用多媒体定时器回调 (独立线程, 1ms tick)
// 每次 tick 用 QPC 计算实际已流逝时间, 播放所有应在此时间段内触发的事件
void CALLBACK ReplayTimerProc(UINT uID, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR) {
    if (!g.isReplaying || !g.replayingTimeline) return;

    DWORD64 elapsedUs = GetTimeUs() - g.replayBaseUs;

    while (g.timelineReplayIdx < g.timeline.size()) {
        if (g.replayAccumUs + g.timeline[g.timelineReplayIdx].deltaUs > elapsedUs)
            break;

        auto& tp = g.timeline[g.timelineReplayIdx];
        g.replayAccumUs += tp.deltaUs;

        {
            int sw = GetSystemMetrics(SM_CXSCREEN);
            int sh = GetSystemMetrics(SM_CYSCREEN);
            LONG dx = (LONG)((tp.pt.x * 65536LL) / sw);
            LONG dy = (LONG)((tp.pt.y * 65536LL) / sh);

            SetCursorPos(tp.pt.x, tp.pt.y);
            if (tp.event == -1) {
                UINT down, up;
                GetMouseFlags(tp.button, down, up);
                if (down) {
                    INPUT in = {};
                    in.type = INPUT_MOUSE;
                    in.mi.dx = dx; in.mi.dy = dy;
                    in.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | down;
                    SendInput(1, &in, sizeof(INPUT));
                }
            } else if (tp.event == -2) {
                UINT down, up;
                GetMouseFlags(tp.button, down, up);
                if (up) {
                    INPUT in = {};
                    in.type = INPUT_MOUSE;
                    in.mi.dx = dx; in.mi.dy = dy;
                    in.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | up;
                    SendInput(1, &in, sizeof(INPUT));
                }
            } else {
                INPUT in = {};
                in.type = INPUT_MOUSE;
                in.mi.dx = dx; in.mi.dy = dy;
                in.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
                SendInput(1, &in, sizeof(INPUT));
            }
        }

        g.timelineReplayIdx++;
    }

    // 本轮播放完毕
    if (g.timelineReplayIdx >= g.timeline.size()) {
        g.currentReplay++;
        bool done = g.replayLoop ? false : (g.currentReplay >= g.replayCount);
        if (done) {
            timeKillEvent(g.mmReplayTimerId);
            g.mmReplayTimerId = 0;
            PostMessage(g.hwnd, WM_APP + 2, 0, 0);
            return;
        }
        g.timelineReplayIdx = 0;
        g.replayAccumUs = 0;
        g.replayBaseUs = GetTimeUs();
    }
}

void StartReplay(HWND hwnd) {
    if (g.isReplaying) return;
    if (g.running) StopClicking(hwnd);
    if (g.isRecording) StopRecording(hwnd);
    if (g.actions.empty() && g.timeline.empty()) return;

    wchar_t buf[16];
    GetDlgItemText(hwnd, IDC_EDIT_REPLAY_N, buf, 16);
    g.replayCount = _wtoi(buf);
    if (g.replayCount <= 0) g.replayCount = 1;

    g.replayLoop = (SendMessage(GetDlgItem(hwnd, IDC_REPLAY_LOOP), BM_GETCHECK, 0, 0) == BST_CHECKED);
    g.currentReplay = 0;
    g.isReplaying = true;
    UpdateUI(hwnd);
    g.clickCount = 0;

    g.replayingTimeline = !g.timeline.empty();
    g.replayIndex = 0;
    g.timelineReplayIdx = 0;
    g.replayAccumUs = 0;
    g.replayBaseUs = GetTimeUs();

    if (g.hideOnAction) ShowWindow(hwnd, SW_MINIMIZE);

    SetDlgItemText(hwnd, IDC_BTN_REPLAY, L"■ 停止播放");
    EnableWindow(GetDlgItem(hwnd, IDC_BTN_START), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_BTN_RECORD), FALSE);

    if (g.replayingTimeline) {
        // 多媒体定时器驱动, 1ms tick, QPC 精确计时
        g.mmReplayTimerId = timeSetEvent(1, 0, ReplayTimerProc, 0,
                                          TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
    } else {
        SetTimer(hwnd, IDC_REPLAY_TIMER, 1, nullptr);
    }
}

void StopReplay(HWND hwnd) {
    if (!g.isReplaying) return;
    if (g.mmReplayTimerId) {
        timeKillEvent(g.mmReplayTimerId);
        g.mmReplayTimerId = 0;
    }
    KillTimer(hwnd, IDC_REPLAY_TIMER);
    g.isReplaying = false;
    g.replayingTimeline = false;
    UpdateUI(hwnd);
    std::wstring repText = L"▶ 播放(" + HotkeyToString(g.hotkeyReplay, g.hkModReplay) + L")";
    SetDlgItemText(hwnd, IDC_BTN_REPLAY, repText.c_str());
    EnableWindow(GetDlgItem(hwnd, IDC_BTN_START), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_BTN_RECORD), TRUE);
}

void UpdateHotkeyLabel(HWND hwnd) {
    std::wstring s = L"热键:  ";
    s += HotkeyToString(g.hotkeyStart, g.hkModStart) + L" 开始    ";
    s += HotkeyToString(g.hotkeyRecord, g.hkModRecord) + L" 录制    ";
    s += HotkeyToString(g.hotkeyReplay, g.hkModReplay) + L" 播放";
    SetDlgItemText(hwnd, IDC_HOTKEY_LABEL, s.c_str());

    std::wstring btnStart = L"▶  开始 (";
    btnStart += HotkeyToString(g.hotkeyStart, g.hkModStart) + L")";
    SetDlgItemText(hwnd, IDC_BTN_START, btnStart.c_str());

    std::wstring btnRec = L"● 录制(" + HotkeyToString(g.hotkeyRecord, g.hkModRecord) + L")";
    SetDlgItemText(hwnd, IDC_BTN_RECORD, btnRec.c_str());

    std::wstring btnRep = L"▶ 播放(" + HotkeyToString(g.hotkeyReplay, g.hkModReplay) + L")";
    SetDlgItemText(hwnd, IDC_BTN_REPLAY, btnRep.c_str());
}

// ── UI 更新 ─────────────────────────────────────
void UpdateUI(HWND hwnd) {
    std::wstringstream ss;
    ss << L"已点击: " << g.clickCount << L" 次";
    SetDlgItemText(hwnd, IDC_COUNT_LABEL, ss.str().c_str());

    if (g.running) {
        SetDlgItemText(hwnd, IDC_STATUS_LABEL, L"●  运行中");
    } else if (g.isReplaying) {
        SetDlgItemText(hwnd, IDC_STATUS_LABEL, L"●  播放中");
    } else if (g.isRecording) {
        SetDlgItemText(hwnd, IDC_STATUS_LABEL, L"●  录制中");
    } else {
        SetDlgItemText(hwnd, IDC_STATUS_LABEL, L"●  已停止");
    }

    EnableWindow(GetDlgItem(hwnd, IDC_BTN_START), !g.running && !g.isReplaying);
    EnableWindow(GetDlgItem(hwnd, IDC_BTN_STOP), g.running);

    BOOL fixed = (IsDlgButtonChecked(hwnd, IDC_CURSOR_FIXED) == BST_CHECKED);
    g.followCursor = !fixed;
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_X), fixed);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_Y), fixed);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_COUNT), !g.repeatForever);
}

// 取点用低级鼠标钩子 — 拦截点击, 不穿透到目标窗口
LRESULT CALLBACK PickMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && g.pickingPosition) {
        if (wParam == WM_LBUTTONDOWN) {
            MSLLHOOKSTRUCT* info = (MSLLHOOKSTRUCT*)lParam;
            HWND hTarget = WindowFromPoint(info->pt);
            if (hTarget == g.hwnd || IsChild(g.hwnd, hTarget)) {
                // 点到了自家窗口 → 取消取点, 放行点击
                g.pickingPosition = false;
                if (g_pickHook) { UnhookWindowsHookEx(g_pickHook); g_pickHook = nullptr; }
                SetCursor(LoadCursor(nullptr, IDC_ARROW));
                SetDlgItemText(g.hwnd, IDC_BTN_PICK_POS, L"📌");
                return CallNextHookEx(nullptr, nCode, wParam, lParam);  // 放行
            }
            // 点外部窗口 → 抓取坐标, 吞掉点击
            g.fixedX = info->pt.x; g.fixedY = info->pt.y;
            wchar_t buf[16];
            _itow_s(g.fixedX, buf, 10); SetDlgItemText(g.hwnd, IDC_EDIT_X, buf);
            _itow_s(g.fixedY, buf, 10); SetDlgItemText(g.hwnd, IDC_EDIT_Y, buf);
            g.pickingPosition = false;
            if (g_pickHook) { UnhookWindowsHookEx(g_pickHook); g_pickHook = nullptr; }
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            SetDlgItemText(g.hwnd, IDC_BTN_PICK_POS, L"📌");
            g.settingRadio = true;
            g.followCursor = false;
            CheckRadioButton(g.hwnd, IDC_CURSOR_FOLLOW, IDC_CURSOR_FIXED, IDC_CURSOR_FIXED);
            g.settingRadio = false;
            UpdateUI(g.hwnd);
            return 1;  // 吞掉点击, 不传递到目标窗口
        }
        if (wParam == WM_RBUTTONDOWN) {
            g.pickingPosition = false;
            if (g_pickHook) { UnhookWindowsHookEx(g_pickHook); g_pickHook = nullptr; }
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            SetDlgItemText(g.hwnd, IDC_BTN_PICK_POS, L"📌");
            return 1;
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void StartClicking(HWND hwnd) {
    if (g.running) return;
    if (g.isReplaying) StopReplay(hwnd);
    if (g.isRecording) StopRecording(hwnd);

    ReadInterval(hwnd);

    if (!g.repeatForever) {
        wchar_t buf[16];
        GetDlgItemText(hwnd, IDC_EDIT_COUNT, buf, 16);
        g.targetCount = _wtoi(buf);
        if (g.targetCount <= 0) g.targetCount = 100;
    }

    if (IsDlgButtonChecked(hwnd, IDC_CURSOR_FIXED) == BST_CHECKED) {
        wchar_t buf[16];
        GetDlgItemText(hwnd, IDC_EDIT_X, buf, 16); g.fixedX = _wtoi(buf);
        GetDlgItemText(hwnd, IDC_EDIT_Y, buf, 16); g.fixedY = _wtoi(buf);
    }

    g.clickCount = 0;
    g.running = true;
    if (g.hideOnAction) ShowWindow(hwnd, SW_MINIMIZE);
    int ms = g.intervalUs / 1000;
    if (ms < 1) ms = 1;
    SetTimer(hwnd, IDC_TIMER, ms, nullptr);
    UpdateUI(hwnd);
}

void StopClicking(HWND hwnd) {
    if (!g.running) return;
    KillTimer(hwnd, IDC_TIMER);
    g.running = false;
    UpdateUI(hwnd);
}

void ToggleTopmost(HWND hwnd) {
    if (g.topmost) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    } else {
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

// ── 控件创建辅助 ────────────────────────────────
// 静态文本 — 无 ID，不需要响应交互
HWND CreateLabel(HWND p, const wchar_t* t, int x, int y, int w, int h) {
    return CreateWindow(L"STATIC", t, WS_CHILD | WS_VISIBLE, x, y, w, h,
                        p, nullptr, GetModuleHandle(nullptr), nullptr);
}

// 单选按钮 — groupStart 标记一组单选按钮的起始
HWND CreateRadio(HWND p, const wchar_t* t, int x, int y, int w, int h, int id, bool chk, bool groupStart = false) {
    HWND c = CreateWindow(L"BUTTON", t,
                          WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | (chk ? WS_TABSTOP : 0) | (groupStart ? WS_GROUP : 0),
                          x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
    if (chk) SendMessage(c, BM_SETCHECK, BST_CHECKED, 0);
    return c;
}

// 编辑框 — 数字输入专用 (ES_NUMBER | ES_RIGHT)
HWND CreateEdit(HWND p, const wchar_t* t, int x, int y, int w, int h, int id) {
    return CreateWindow(L"EDIT", t,
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_RIGHT | WS_TABSTOP,
                        x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
}

// 复选框 — chk=true 则初始选中
HWND CreateCheck(HWND p, const wchar_t* t, int x, int y, int w, int h, int id, bool chk) {
    HWND c = CreateWindow(L"BUTTON", t,
                          WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                          x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
    if (chk) SendMessage(c, BM_SETCHECK, BST_CHECKED, 0);
    return c;
}

// 下拉框 — 只读选择列表 (CBS_DROPDOWNLIST)
HWND CreateCombo(HWND p, int x, int y, int w, int h, int id) {
    return CreateWindow(L"COMBOBOX", nullptr,
                        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                        x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
}

// 普通按钮 — BS_PUSHBUTTON
HWND CreateButton(HWND p, const wchar_t* t, int x, int y, int w, int h, int id) {
    return CreateWindow(L"BUTTON", t,
                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
}

// 只读展示框 — 居中文本，不可编辑不可选中 (STATIC + SS_CENTER + SS_SUNKEN)
HWND CreateDisplay(HWND p, const wchar_t* t, int x, int y, int w, int h, int id) {
    return CreateWindow(L"STATIC", t,
                        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_SUNKEN,
                        x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
}

// 水平分隔线 — SS_ETCHEDHORZ
HWND CreateSep(HWND p, int x, int y, int w) {
    return CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                        x, y, w, 2, p, nullptr, GetModuleHandle(nullptr), nullptr);
}

// ── 托盘 & 注册表 ────────────────────────────────
void SaveAllSettings() {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\AutoClicker",
                       0, nullptr, REG_OPTION_NON_VOLATILE,
                       KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD v;
        v = g.closeAction;       RegSetValueEx(hKey, L"CloseAction", 0, REG_DWORD, (BYTE*)&v, sizeof(v));
        v = g.topmost;           RegSetValueEx(hKey, L"Topmost", 0, REG_DWORD, (BYTE*)&v, sizeof(v));
        v = g.randomizeInterval; RegSetValueEx(hKey, L"RandomizeInterval", 0, REG_DWORD, (BYTE*)&v, sizeof(v));
        v = g.randomizePosition; RegSetValueEx(hKey, L"RandomizePosition", 0, REG_DWORD, (BYTE*)&v, sizeof(v));
        v = g.sampleRateHz;      RegSetValueEx(hKey, L"SampleRateHz", 0, REG_DWORD, (BYTE*)&v, sizeof(v));
        v = g.hideOnAction;      RegSetValueEx(hKey, L"HideOnAction", 0, REG_DWORD, (BYTE*)&v, sizeof(v));
        v = g.recordSelfWin;     RegSetValueEx(hKey, L"RecordSelfWin", 0, REG_DWORD, (BYTE*)&v, sizeof(v));
        v = g.hotkeyStart;       RegSetValueEx(hKey, L"HotkeyStartVK", 0, REG_DWORD, (BYTE*)&v, 4);
        v = g.hkModStart;        RegSetValueEx(hKey, L"HotkeyStartMod",0, REG_DWORD, (BYTE*)&v, 4);
        v = g.hotkeyRecord;      RegSetValueEx(hKey, L"HotkeyRecordVK",0, REG_DWORD, (BYTE*)&v, 4);
        v = g.hkModRecord;       RegSetValueEx(hKey, L"HotkeyRecordMod",0,REG_DWORD, (BYTE*)&v, 4);
        v = g.hotkeyReplay;      RegSetValueEx(hKey, L"HotkeyReplayVK",0, REG_DWORD, (BYTE*)&v, 4);
        v = g.hkModReplay;       RegSetValueEx(hKey, L"HotkeyReplayMod",0,REG_DWORD, (BYTE*)&v, 4);
        RegSetValueEx(hKey, L"DefaultFilePath", 0, REG_SZ,
                      (BYTE*)g.defaultFilePath.c_str(),
                      (DWORD)((g.defaultFilePath.length() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
    }
}

// ── 热键序列化 ───────────────────────────────────
std::wstring HotkeyToString(UINT vk, UINT mod) {
    std::wstring s;
    if (mod & MOD_CONTROL) s += L"Ctrl+";
    if (mod & MOD_ALT)     s += L"Alt+";
    if (mod & MOD_SHIFT)   s += L"Shift+";
    if (mod & MOD_WIN)     s += L"Win+";

    if (vk >= VK_F1 && vk <= VK_F24) {
        s += L"F" + std::to_wstring(vk - VK_F1 + 1);
    } else if (vk >= 'A' && vk <= 'Z') {
        s += (wchar_t)vk;
    } else if (vk >= '0' && vk <= '9') {
        s += (wchar_t)vk;
    } else if (vk == VK_SPACE) {
        s += L"Space";
    } else if (vk == VK_TAB) {
        s += L"Tab";
    } else {
        wchar_t name[64] = {};
        LONG scan = MapVirtualKey(vk, MAPVK_VK_TO_VSC) << 16;
        if (vk == VK_INSERT || vk == VK_DELETE || vk == VK_HOME ||
            vk == VK_END || vk == VK_PRIOR || vk == VK_NEXT ||
            vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN)
            scan |= 1 << 24; // extended-key flag
        if (GetKeyNameText(scan, name, 64))
            s += name;
        else
            s += L"VK_" + std::to_wstring(vk);
    }
    return s;
}

void LoadAllSettings() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\AutoClicker", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD v = 0, sz = sizeof(DWORD);

        if (RegQueryValueEx(hKey, L"CloseAction", nullptr, nullptr, (LPBYTE)&v, &sz) == ERROR_SUCCESS && v <= 2)
            g.closeAction = (int)v;
        sz = sizeof(v);
        if (RegQueryValueEx(hKey, L"Topmost", nullptr, nullptr, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
            g.topmost = (v != 0);
        sz = sizeof(v);
        if (RegQueryValueEx(hKey, L"RandomizeInterval", nullptr, nullptr, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
            g.randomizeInterval = (v != 0);
        sz = sizeof(v);
        if (RegQueryValueEx(hKey, L"RandomizePosition", nullptr, nullptr, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
            g.randomizePosition = (v != 0);
        sz = sizeof(v);
        if (RegQueryValueEx(hKey, L"SampleRateHz", nullptr, nullptr, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
            g.sampleRateHz = (int)v;
        sz = sizeof(v);
        if (RegQueryValueEx(hKey, L"HideOnAction", nullptr, nullptr, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
            g.hideOnAction = (v != 0);
        sz = sizeof(v);
        if (RegQueryValueEx(hKey, L"RecordSelfWin", nullptr, nullptr, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
            g.recordSelfWin = (v != 0);

        sz = sizeof(DWORD);
        if (RegQueryValueEx(hKey, L"HotkeyStartVK", nullptr, nullptr, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
            g.hotkeyStart = v;
        sz = sizeof(DWORD);
        if (RegQueryValueEx(hKey, L"HotkeyStartMod", nullptr, nullptr, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
            g.hkModStart = v;
        sz = sizeof(DWORD);
        if (RegQueryValueEx(hKey, L"HotkeyRecordVK", nullptr, nullptr, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
            g.hotkeyRecord = v;
        sz = sizeof(DWORD);
        if (RegQueryValueEx(hKey, L"HotkeyRecordMod", nullptr, nullptr, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
            g.hkModRecord = v;
        sz = sizeof(DWORD);
        if (RegQueryValueEx(hKey, L"HotkeyReplayVK", nullptr, nullptr, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
            g.hotkeyReplay = v;
        sz = sizeof(DWORD);
        if (RegQueryValueEx(hKey, L"HotkeyReplayMod", nullptr, nullptr, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
            g.hkModReplay = v;

        DWORD pathSize = 0;
        if (RegQueryValueEx(hKey, L"DefaultFilePath", nullptr, nullptr, nullptr, &pathSize) == ERROR_SUCCESS && pathSize > 0) {
            std::vector<wchar_t> buf(pathSize / sizeof(wchar_t) + 1);
            if (RegQueryValueEx(hKey, L"DefaultFilePath", nullptr, nullptr, (LPBYTE)buf.data(), &pathSize) == ERROR_SUCCESS)
                g.defaultFilePath = buf.data();
        }

        RegCloseKey(hKey);
    }
}

void ReregisterHotkeys(HWND hwnd) {
    UnregisterHotKey(hwnd, 1);
    UnregisterHotKey(hwnd, 2);
    UnregisterHotKey(hwnd, 3);
    RegisterHotKey(hwnd, 1, g.hkModStart,  g.hotkeyStart);
    RegisterHotKey(hwnd, 2, g.hkModRecord, g.hotkeyRecord);
    RegisterHotKey(hwnd, 3, g.hkModReplay, g.hotkeyReplay);
}

void CreateTrayIcon(HWND hwnd) {
    HICON hTrayIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(1), IMAGE_ICON, 16, 16, 0);
    if (!hTrayIcon) hTrayIcon = LoadIcon(nullptr, IDI_APPLICATION);

    g.nid.cbSize = sizeof(NOTIFYICONDATA);
    g.nid.hWnd = hwnd;
    g.nid.uID = 1;
    g.nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g.nid.uCallbackMessage = WM_TRAYICON;
    g.nid.hIcon = hTrayIcon;
    wcscpy(g.nid.szTip, L"连点器 Auto Clicker");
    Shell_NotifyIcon(NIM_ADD, &g.nid);
}

void RemoveTrayIcon(HWND hwnd) {
    g.nid.cbSize = sizeof(NOTIFYICONDATA);
    g.nid.hWnd = hwnd;
    g.nid.uID = 1;
    Shell_NotifyIcon(NIM_DELETE, &g.nid);
}

// ── 设置窗口过程 ─────────────────────────────────
LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  DEFAULT_PITCH | FF_DONTCARE,
                                  L"Microsoft YaHei UI");

        int y = 10;
        // ── 通用 ──
        CreateLabel(hwnd, L"── 通用 ──", 15, y, 120, 20); y += 28;
        CreateCheck(hwnd, L"置顶", 15, y, 60, 24, IDC_CHECK_TOPMOST, g.topmost); y += 26;
        CreateCheck(hwnd, L"随机间隔 ±20%", 15, y, 200, 24, IDC_CHECK_RAND_DELAY, g.randomizeInterval); y += 26;
        CreateCheck(hwnd, L"位置微抖 ±4px", 15, y, 200, 24, IDC_CHECK_RAND_POS, g.randomizePosition); y += 32;
        CreateLabel(hwnd, L"关闭行为:", 15, y, 80, 22);
        HWND hCloseCombo = CreateCombo(hwnd, 65, y-2, 160, 200, IDC_COMBO_CLOSE_ACTION);
        SendMessage(hCloseCombo, CB_ADDSTRING, 0, (LPARAM)L"询问");
        SendMessage(hCloseCombo, CB_ADDSTRING, 0, (LPARAM)L"最小化到托盘");
        SendMessage(hCloseCombo, CB_ADDSTRING, 0, (LPARAM)L"退出程序");
        SendMessage(hCloseCombo, CB_SETCURSEL, g.closeAction, 0);
        y += 34;

        // ── 录制 ──
        CreateLabel(hwnd, L"── 录制 ──", 15, y, 120, 20); y += 28;
        CreateLabel(hwnd, L"采样率:", 15, y, 55, 22);
        HWND hSampleCombo = CreateCombo(hwnd, 70, y-2, 90, 200, IDC_COMBO_SAMPLE_RATE);
        const wchar_t* hzItems[] = {L"125 Hz", L"250 Hz", L"500 Hz", L"1K Hz", L"2K Hz", L"4K Hz", L"8K Hz"};
        for (int i = 0; i < 7; i++) SendMessage(hSampleCombo, CB_ADDSTRING, 0, (LPARAM)hzItems[i]);
        int hzVals[] = {125, 250, 500, 1000, 2000, 4000, 8000};
        int sel = 3;
        for (int i = 0; i < 7; i++) { if (g.sampleRateHz == hzVals[i]) sel = i; }
        SendMessage(hSampleCombo, CB_SETCURSEL, sel, 0);
        y += 28;
        CreateCheck(hwnd, L"开始时隐藏", 15, y, 200, 24, IDC_CHECK_HIDE, g.hideOnAction); y += 26;
        CreateCheck(hwnd, L"录制程序界面上的光标", 15, y, 250, 24, IDC_CHECK_RECORD_SELF, g.recordSelfWin); y += 32;

        // ── 文件 ──
        CreateLabel(hwnd, L"── 文件 ──", 15, y, 120, 20); y += 28;
        CreateLabel(hwnd, L"默认路径:", 15, y, 80, 22);
        CreateWindow(L"EDIT", g.defaultFilePath.c_str(),
                     WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | WS_TABSTOP,
                     65, y-2, 220, 22,
                     hwnd, (HMENU)(INT_PTR)IDC_EDIT_DEFAULT_PATH, GetModuleHandle(nullptr), nullptr);
        CreateButton(hwnd, L"浏览...", 290, y-2, 60, 22, IDC_BTN_BROWSE_PATH);
        y += 32;

        // ── 热键 ──
        int revampBut = 300; //    修改按钮的x位置
        CreateLabel(hwnd, L"── 热键 ──", 15, y, 120, 20); y += 28;
        CreateLabel(hwnd, L"开始/停止:", 15, y, 80, 24);
        std::wstring hkStart = HotkeyToString(g.hotkeyStart, g.hkModStart);
        CreateDisplay(hwnd, hkStart.c_str(), 100, y-2, 120, 26, IDC_BTN_HK_START);
        CreateButton(hwnd, L"修改", revampBut, y-2, 50, 26, IDC_BTN_HK_START+10);
        y += 30;
        CreateLabel(hwnd, L"录制:", 15, y, 80, 24);
        std::wstring hkRec = HotkeyToString(g.hotkeyRecord, g.hkModRecord);
        CreateDisplay(hwnd, hkRec.c_str(), 100, y-2, 120, 26, IDC_BTN_HK_RECORD);
        CreateButton(hwnd, L"修改", revampBut, y-2, 50, 26, IDC_BTN_HK_RECORD+10);
        y += 30;
        CreateLabel(hwnd, L"回放:", 15, y, 80, 24);
        std::wstring hkRep = HotkeyToString(g.hotkeyReplay, g.hkModReplay);
        CreateDisplay(hwnd, hkRep.c_str(), 100, y-2, 120, 26, IDC_BTN_HK_REPLAY);
        CreateButton(hwnd, L"修改", revampBut, y-2, 50, 26, IDC_BTN_HK_REPLAY+10);
        y += 40;

        int btnW = 80;
        CreateButton(hwnd, L"确定", (SET_WIDTH - btnW) / 2 - 10, y, btnW, 28, IDC_SETTINGS_OK);

        // 设置字体
        EnumChildWindows(hwnd, [](HWND child, LPARAM lp) -> BOOL {
            SendMessage(child, WM_SETFONT, (WPARAM)lp, TRUE);
            return TRUE;
        }, (LPARAM)hFont);

        return 0;
    }

    case WM_COMMAND: {
        WORD id = LOWORD(wp);
        switch (id) {
        case IDC_CHECK_TOPMOST:
            g.topmost = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_TOPMOST), BM_GETCHECK, 0, 0) == BST_CHECKED);
            ToggleTopmost(g.hwnd);
            return 0;
        case IDC_CHECK_RAND_DELAY:
            g.randomizeInterval = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_RAND_DELAY), BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;
        case IDC_CHECK_RAND_POS:
            g.randomizePosition = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_RAND_POS), BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;
        case IDC_COMBO_CLOSE_ACTION:
            if (HIWORD(wp) == CBN_SELCHANGE)
                g.closeAction = (int)SendMessage(GetDlgItem(hwnd, IDC_COMBO_CLOSE_ACTION), CB_GETCURSEL, 0, 0);
            return 0;
        case IDC_COMBO_SAMPLE_RATE:
            if (HIWORD(wp) == CBN_SELCHANGE) {
                int hzArr[] = {125, 250, 500, 1000, 2000, 4000, 8000};
                int idx = (int)SendMessage(GetDlgItem(hwnd, IDC_COMBO_SAMPLE_RATE), CB_GETCURSEL, 0, 0);
                if (idx >= 0 && idx < 7) g.sampleRateHz = hzArr[idx];
            }
            return 0;
        case IDC_CHECK_HIDE:
            g.hideOnAction = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_HIDE), BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;
        case IDC_CHECK_RECORD_SELF:
            g.recordSelfWin = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_RECORD_SELF), BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;

        case IDC_BTN_BROWSE_PATH: {
            BROWSEINFO bi = {};
            bi.hwndOwner = hwnd;
            bi.lpszTitle = L"选择默认导入/导出文件夹";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
            if (pidl) {
                wchar_t path[MAX_PATH];
                if (SHGetPathFromIDList(pidl, path)) {
                    g.defaultFilePath = path;
                    SetDlgItemText(hwnd, IDC_EDIT_DEFAULT_PATH, path);
                }
                CoTaskMemFree(pidl);
            }
            return 0;
        }
        case IDC_EDIT_DEFAULT_PATH:
            if (HIWORD(wp) == EN_CHANGE) {
                wchar_t buf[MAX_PATH];
                GetDlgItemText(hwnd, IDC_EDIT_DEFAULT_PATH, buf, MAX_PATH);
                g.defaultFilePath = buf;
            }
            return 0;

        // 热键修改按钮
        case IDC_BTN_HK_START+10:
            g.capturingHotkey = true; g.capturingWhich = 1;
            SetDlgItemText(hwnd, IDC_BTN_HK_START, L"...按任意键...");
            SetFocus(hwnd);
            return 0;
        case IDC_BTN_HK_RECORD+10:
            g.capturingHotkey = true; g.capturingWhich = 2;
            SetDlgItemText(hwnd, IDC_BTN_HK_RECORD, L"...按任意键...");
            SetFocus(hwnd);
            return 0;
        case IDC_BTN_HK_REPLAY+10:
            g.capturingHotkey = true; g.capturingWhich = 3;
            SetDlgItemText(hwnd, IDC_BTN_HK_REPLAY, L"...按任意键...");
            SetFocus(hwnd);
            return 0;

        case IDC_SETTINGS_OK:
            SaveAllSettings();
            DestroyWindow(hwnd);
            return 0;
        }
        return 0;
    }

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (g.capturingHotkey && g.capturingWhich > 0) {
            UINT vk = (UINT)wp;
            if (vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU || vk == VK_LWIN || vk == VK_RWIN)
                return 0; // 等下一个非修饰键

            UINT mod = 0;
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) mod |= MOD_CONTROL;
            if (GetAsyncKeyState(VK_SHIFT)   & 0x8000) mod |= MOD_SHIFT;
            if (GetAsyncKeyState(VK_MENU)    & 0x8000) mod |= MOD_ALT;
            if (GetAsyncKeyState(VK_LWIN)    & 0x8000) mod |= MOD_WIN;
            if (GetAsyncKeyState(VK_RWIN)    & 0x8000) mod |= MOD_WIN;

            std::wstring text = HotkeyToString(vk, mod);
            switch (g.capturingWhich) {
            case 1:
                g.hotkeyStart = vk; g.hkModStart = mod;
                SetDlgItemText(hwnd, IDC_BTN_HK_START, text.c_str());
                break;
            case 2:
                g.hotkeyRecord = vk; g.hkModRecord = mod;
                SetDlgItemText(hwnd, IDC_BTN_HK_RECORD, text.c_str());
                break;
            case 3:
                g.hotkeyReplay = vk; g.hkModReplay = mod;
                SetDlgItemText(hwnd, IDC_BTN_HK_REPLAY, text.c_str());
                break;
            }
            g.capturingHotkey = false;
            g.capturingWhich = 0;
            return 0;
        }
        break;

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        SetBkMode(hdc, TRANSPARENT);
        static HBRUSH hBg = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
        return (LRESULT)hBg;
    }

    case WM_CLOSE:
        SaveAllSettings();
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        SetForegroundWindow(g.hwnd);
        ReregisterHotkeys(g.hwnd);
        UpdateHotkeyLabel(g.hwnd);
        g.hwndSettings = nullptr;
        // 关闭设置窗口后刷新主窗口的标签
        InvalidateRect(GetDlgItem(g.hwnd, IDC_COUNT_LABEL), nullptr, TRUE);
        InvalidateRect(GetDlgItem(g.hwnd, IDC_HOTKEY_LABEL), nullptr, TRUE);
        InvalidateRect(GetDlgItem(g.hwnd, IDC_STATUS_LABEL), nullptr, TRUE);
        break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// ── 主窗口过程 ─────────────────────────────────────
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g.hwnd = hwnd;

        HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  DEFAULT_PITCH | FF_DONTCARE,
                                  L"Microsoft YaHei UI");

        // ── 鼠标按钮 ──
        CreateLabel(hwnd, L"鼠标按钮:", 20, 16, 80, 22);
        CreateRadio(hwnd, L"左键", 20, 42, 70, 22, IDC_BTN_LEFT, true, true);
        CreateRadio(hwnd, L"右键", 110, 42, 70, 22, IDC_BTN_RIGHT, false);
        CreateRadio(hwnd, L"中键", 200, 42, 70, 22, IDC_BTN_MIDDLE, false);

        // ── 点击模式 ──
        CreateLabel(hwnd, L"点击模式:", 20, 78, 80, 22);
        CreateRadio(hwnd, L"单击", 20, 104, 80, 22, IDC_SINGLE, true, true);
        CreateRadio(hwnd, L"双击", 120, 104, 80, 22, IDC_DOUBLE, false);

        CreateSep(hwnd, 20, 136, 410);

        // ── 点击间隔 (秒/毫秒/微秒) ──
        CreateLabel(hwnd, L"点击间隔:", 20, 146, 80, 22);
        CreateLabel(hwnd, L"秒", 20, 176, 20, 22);
        CreateEdit(hwnd, L"0", 40, 174, 50, 22, IDC_EDIT_SEC);
        CreateLabel(hwnd, L"毫秒", 102, 176, 35, 22);
        CreateEdit(hwnd, L"0", 138, 174, 55, 22, IDC_EDIT_MS);
        CreateLabel(hwnd, L"微秒", 205, 176, 35, 22);
        CreateEdit(hwnd, L"10", 240, 174, 65, 22, IDC_EDIT_US);

        CreateSep(hwnd, 20, 208, 410);

        // ── 重复设置 ──
        CreateLabel(hwnd, L"重复模式:", 20, 218, 80, 22);
        CreateRadio(hwnd, L"重复直到停止", 20, 244, 120, 22, IDC_REPEAT_FOREVER, true, true);
        CreateRadio(hwnd, L"指定次数", 155, 244, 90, 22, IDC_REPEAT_COUNT, false);
        CreateEdit(hwnd, L"100", 255, 244, 55, 22, IDC_EDIT_COUNT);

        CreateSep(hwnd, 20, 276, 410);

        // ── 光标位置 ──
        CreateLabel(hwnd, L"光标位置:", 20, 286, 80, 22);
        CreateRadio(hwnd, L"跟随鼠标", 20, 312, 100, 22, IDC_CURSOR_FOLLOW, true, true);
        CreateRadio(hwnd, L"固定位置", 130, 312, 100, 22, IDC_CURSOR_FIXED, false);
        CreateLabel(hwnd, L"X:", 240, 314, 18, 22);
        CreateEdit(hwnd, L"0", 258, 312, 55, 22, IDC_EDIT_X);
        CreateLabel(hwnd, L"Y:", 320, 314, 18, 22);
        CreateEdit(hwnd, L"0", 338, 312, 55, 22, IDC_EDIT_Y);
        CreateButton(hwnd, L"📌", 400, 312, 28, 22, IDC_BTN_PICK_POS);

        CreateSep(hwnd, 20, 344, 410);

        // ── 齿轮按钮 ──
        CreateButton(hwnd, L"⚙", 405, 14, 28, 24, IDC_BTN_SETTINGS);

        // ── 操作录制 ──
        CreateLabel(hwnd, L"操作录制:", 20, 358, 80, 22);

        
        std::wstring recText = L"● 录制(" + HotkeyToString(g.hotkeyRecord, g.hkModRecord) + L")";
        CreateButton(hwnd, recText.c_str(), 20, 382, 75, 28, IDC_BTN_RECORD);
        std::wstring repText = L"▶ 播放(" + HotkeyToString(g.hotkeyReplay, g.hkModReplay) + L")";
        CreateButton(hwnd, repText.c_str(), 98, 382, 75, 28, IDC_BTN_REPLAY);
        CreateButton(hwnd, L"清空", 176, 382, 55, 28, IDC_BTN_CLEAR_REC);
        CreateButton(hwnd, L"导出", 236, 382, 55, 28, IDC_BTN_EXPORT);
        CreateButton(hwnd, L"导入", 296, 382, 55, 28, IDC_BTN_IMPORT);
        CreateCheck(hwnd, L"记录轨迹", 20, 414, 85, 24, IDC_CHECK_TRAJECTORY, true);

        // 已导入文件列表
        CreateLabel(hwnd, L"已导入文件 (点击切换):", 20, 444, 180, 20);

        g.hImportedList = CreateWindow(L"LISTBOX", nullptr,
                                        WS_CHILD | WS_VISIBLE | WS_BORDER |
                                        WS_VSCROLL | LBS_NOTIFY,
                                        20, 464, 410, 52, hwnd,
                                        (HMENU)(INT_PTR)IDC_LIST_IMPORTED,
                                        GetModuleHandle(nullptr), nullptr);
        SendMessage(g.hImportedList, LB_ADDSTRING, 0, (LPARAM)L"<当前录制>");
        SendMessage(g.hImportedList, LB_SETITEMDATA, 0, -1);
        SendMessage(g.hImportedList, LB_SETCURSEL, 0, 0);

        // 播放设置 + 当前动作数
        CreateWindow(L"STATIC", L"当前动作: 0 个", WS_CHILD | WS_VISIBLE, 20, 522, 200, 20,
                     hwnd, (HMENU)(INT_PTR)IDC_LABEL_RECORD, GetModuleHandle(nullptr), nullptr);
        CreateLabel(hwnd, L"播放次数:", 170, 522, 70, 22);
        CreateEdit(hwnd, L"1", 240, 520, 40, 22, IDC_EDIT_REPLAY_N);
        CreateCheck(hwnd, L"循环", 290, 522, 55, 22, IDC_REPLAY_LOOP, false);

        g.hList = CreateWindow(L"LISTBOX", nullptr,
                                WS_CHILD | WS_VISIBLE | WS_BORDER |
                                WS_VSCROLL | LBS_NOSEL | LBS_NOTIFY,
                                20, 548, 410, 84, hwnd,
                                (HMENU)(INT_PTR)IDC_LIST_RECORD,
                                GetModuleHandle(nullptr), nullptr);

        CreateSep(hwnd, 20, 640, 410);

        // ── 底部状态区 ──
        // Row 1: 热键标签 + 点击计数
        CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 20, 646, 300, 22,
                     hwnd, (HMENU)(INT_PTR)IDC_HOTKEY_LABEL, GetModuleHandle(nullptr), nullptr);
        UpdateHotkeyLabel(hwnd);

        CreateWindow(L"STATIC", L"已点击: 0 次", WS_CHILD | WS_VISIBLE, 280, 646, 300, 22,
                     hwnd, (HMENU)(INT_PTR)IDC_COUNT_LABEL, GetModuleHandle(nullptr), nullptr);

        // Row 2: 状态 + 开始/停止按钮
        CreateWindow(L"STATIC", L"●  已停止", WS_CHILD | WS_VISIBLE, 10, 674, 120, 22,
                     hwnd, (HMENU)(INT_PTR)IDC_STATUS_LABEL, GetModuleHandle(nullptr), nullptr);

        {
            std::wstring btnStart = L"▶  开始 (";
            btnStart += HotkeyToString(g.hotkeyStart, g.hkModStart) + L")";
            CreateButton(hwnd, btnStart.c_str(), 210, 670, 90, 26, IDC_BTN_START);
            CreateButton(hwnd, L"■  停止", 305, 670, 85, 26, IDC_BTN_STOP);
        }

        // 初始化禁用状态
        EnableWindow(GetDlgItem(hwnd, IDC_BTN_STOP), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_COUNT), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_X), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_Y), FALSE);

        // 设置字体
        EnumChildWindows(hwnd, [](HWND child, LPARAM lp) -> BOOL {
            SendMessage(child, WM_SETFONT, (WPARAM)lp, TRUE);
            return TRUE;
        }, (LPARAM)hFont);

        // 列表使用稍小字体
        HFONT hListFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                      DEFAULT_PITCH | FF_DONTCARE,
                                      L"Consolas");
        SendMessage(g.hList, WM_SETFONT, (WPARAM)hListFont, TRUE);
        SendMessage(g.hImportedList, WM_SETFONT, (WPARAM)hListFont, TRUE);

        // 加载注册表中的所有设置
        LoadAllSettings();
        UpdateHotkeyLabel(hwnd);
        ToggleTopmost(hwnd);
        RegisterHotKey(hwnd, 1, g.hkModStart,  g.hotkeyStart);
        RegisterHotKey(hwnd, 2, g.hkModRecord, g.hotkeyRecord);
        RegisterHotKey(hwnd, 3, g.hkModReplay, g.hotkeyReplay);

        return 0;
    }

    case WM_APP: {
        // 录制列表更新
        RefreshRecordList(hwnd);
        return 0;
    }


    case WM_APP + 2: {
        // 轨迹回放完成 (由多媒体定时器回调通知)
        StopReplay(hwnd);
        UpdateUI(hwnd);
        return 0;
    }

    case WM_LBUTTONDOWN:
        SetFocus(hwnd);  // 点击窗口空白处使输入框失焦
        return 0;


    case WM_HOTKEY:
        if (wp == 1) {
            if (g.running) StopClicking(hwnd);
            else StartClicking(hwnd);
        } else if (wp == 2) {
            if (g.isRecording) StopRecording(hwnd);
            else StartRecording(hwnd);
        } else if (wp == 3) {
            if (g.isReplaying) StopReplay(hwnd);
            else StartReplay(hwnd);
        }
        UpdateUI(hwnd);
        return 0;

    case WM_TIMER:
        if (wp == IDC_TIMER && g.running) {
            DoClick();
            g.clickCount++;
            if (!g.repeatForever && g.clickCount >= g.targetCount) {
                StopClicking(hwnd);
            } else if (g.randomizeInterval) {
                int baseMs = g.intervalUs / 1000;
                if (baseMs < 10) baseMs = 10;
                int jitter = baseMs / 5;
                if (jitter < 5) jitter = 5;
                int nextMs = baseMs + RandomRange(-jitter, jitter);
                if (nextMs < 5) nextMs = 5;
                KillTimer(hwnd, IDC_TIMER);
                SetTimer(hwnd, IDC_TIMER, nextMs, nullptr);
            }
            std::wstringstream ss;
            ss << L"已点击: " << g.clickCount << L" 次";
            if (g_lastClickMode >= 0) {
                ss << L"  [" << (g_lastClickMode ? L"固定" : L"跟随")
                   << L" (" << g_lastClickPt.x << L"," << g_lastClickPt.y << L")]";
            }
            SetDlgItemText(hwnd, IDC_COUNT_LABEL, ss.str().c_str());
        }
        else if (wp == IDC_REPLAY_TIMER && g.isReplaying) {
            // 仅动作回放走这里 (轨迹回放由多媒体定时器 ReplayTimerProc 驱动)
            KillTimer(hwnd, IDC_REPLAY_TIMER);

            if (!g.replayingTimeline) {
                if (g.replayIndex >= g.actions.size()) {
                    g.currentReplay++;
                    bool done = g.replayLoop ? false : (g.currentReplay >= g.replayCount);
                    if (done) { StopReplay(hwnd); UpdateUI(hwnd); return 0; }
                    g.replayIndex = 0;
                }

                auto& a = g.actions[g.replayIndex];
                g.ignoreNextDown = true;
                SimulateAction(a);
                g.ignoreNextDown = false;
                g.replayIndex++;

                DWORD nextDelay = (g.replayIndex < g.actions.size())
                                  ? g.actions[g.replayIndex].delayMs
                                  : 100;
                if (nextDelay < 1) nextDelay = 1;
                SetTimer(hwnd, IDC_REPLAY_TIMER, nextDelay, nullptr);
            }
        }
        return 0;

    case WM_COMMAND:
        // 处理已导入文件列表的点击切换
        if (HIWORD(wp) == LBN_SELCHANGE && LOWORD(wp) == IDC_LIST_IMPORTED) {
            int idx = (int)SendMessage(g.hImportedList, LB_GETCURSEL, 0, 0);
            if (idx == LB_ERR) return 0;
            int data = (int)SendMessage(g.hImportedList, LB_GETITEMDATA, idx, 0);
            if (data == -1) {
                // 切换到"当前录制" - 不影响 g.actions
                g.selectedImport = -1;
            } else if (data >= 0 && data < (int)g.importedFiles.size()) {
                if (g.isRecording) StopRecording(hwnd);
                if (g.isReplaying) StopReplay(hwnd);
                g.selectedImport = data;
                g.actions = g.importedFiles[data].actions;
                RefreshRecordList(hwnd);
                UpdateUI(hwnd);
            }
            return 0;
        }
        switch (LOWORD(wp)) {
        case IDC_BTN_LEFT: case IDC_BTN_RIGHT: case IDC_BTN_MIDDLE:
            if (g.settingRadio) return 0;
            if      (IsDlgButtonChecked(hwnd, IDC_BTN_LEFT))   g.mouseButton = 0;
            else if (IsDlgButtonChecked(hwnd, IDC_BTN_MIDDLE)) g.mouseButton = 2;
            else                                               g.mouseButton = 1;
            return 0;
        case IDC_SINGLE: case IDC_DOUBLE:
            if (g.settingRadio) return 0;
            g.clickMode = (IsDlgButtonChecked(hwnd, IDC_SINGLE) == BST_CHECKED) ? 0 : 1;
            return 0;

        case IDC_REPEAT_FOREVER: case IDC_REPEAT_COUNT:
            if (g.settingRadio) return 0;
            g.repeatForever = (IsDlgButtonChecked(hwnd, IDC_REPEAT_FOREVER) == BST_CHECKED);
            UpdateUI(hwnd); return 0;

        case IDC_CURSOR_FOLLOW: case IDC_CURSOR_FIXED:
            if (g.settingRadio) return 0;
            g.followCursor = (IsDlgButtonChecked(hwnd, IDC_CURSOR_FOLLOW) == BST_CHECKED);
            UpdateUI(hwnd); return 0;
        case IDC_BTN_PICK_POS:
            g.pickingPosition = true;
            g_pickHook = SetWindowsHookEx(WH_MOUSE_LL, PickMouseProc, GetModuleHandle(nullptr), 0);
            SetCursor(LoadCursor(nullptr, IDC_CROSS));
            SetDlgItemText(hwnd, IDC_BTN_PICK_POS, L"...");
            return 0;

        case IDC_BTN_SETTINGS:
            if (!g.hwndSettings) {
                UnregisterHotKey(hwnd, 1);
                UnregisterHotKey(hwnd, 2);
                UnregisterHotKey(hwnd, 3);
                RECT mw;
                GetWindowRect(hwnd, &mw);
                g.hwndSettings = CreateWindowEx(WS_EX_TOPMOST, L"SettingsWindow", L"设置",
                    WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                    mw.left + 45, mw.top + 80, SET_WIDTH, SET_HEIGHT,
                    nullptr, nullptr, g_hInst, nullptr);
            }
            return 0;

        case IDC_CHECK_TRAJECTORY:
            g.recordTrajectory = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_TRAJECTORY), BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;

        case IDC_BTN_START:
            StartClicking(hwnd);
            return 0;

        case IDC_BTN_STOP:
            StopClicking(hwnd);
            return 0;

        case IDC_BTN_RECORD:
            if (g.isRecording) StopRecording(hwnd);
            else StartRecording(hwnd);
            UpdateUI(hwnd);
            return 0;

        case IDC_BTN_REPLAY:
            if (g.isReplaying) StopReplay(hwnd);
            else StartReplay(hwnd);
            UpdateUI(hwnd);
            return 0;

        case IDC_BTN_CLEAR_REC:
            ClearRecording(hwnd);
            return 0;

        case IDC_BTN_EXPORT:
            if (!g.isRecording && !g.isReplaying)
                ExportRecording(hwnd);
            return 0;

        case IDC_BTN_IMPORT:
            if (!g.isRecording && !g.isReplaying)
                ImportRecording(hwnd);
            return 0;

        case IDC_EDIT_SEC: case IDC_EDIT_MS: case IDC_EDIT_US:
            if (HIWORD(wp) == EN_CHANGE) ApplyInterval(hwnd);
            if (HIWORD(wp) == EN_KILLFOCUS) FixEmptyNumericEdit(hwnd, LOWORD(wp));
            return 0;

        case IDC_EDIT_COUNT: case IDC_EDIT_X: case IDC_EDIT_Y:
        case IDC_EDIT_REPLAY_N:
            if (HIWORD(wp) == EN_KILLFOCUS) FixEmptyNumericEdit(hwnd, LOWORD(wp));
            return 0;
        }
        return 0;

    case WM_CLOSE:
        if (g.closeAction == 1) {
            // 最小化到托盘
            g.inTray = true;
            CreateTrayIcon(hwnd);
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        } else if (g.closeAction == 2) {
            // 直接退出
            break;
        } else {
            // 询问
            int res = MessageBox(hwnd,
                L"您希望关闭时执行什么操作?\n\n"
                L"按\"是\"最小化到系统托盘\n"
                L"按\"否\"直接退出程序",
                L"关闭选项", MB_YESNO | MB_ICONQUESTION);
            if (res == IDYES) {
                g.closeAction = 1;
                SaveAllSettings();
                SendMessage(GetDlgItem(hwnd, IDC_COMBO_CLOSE_ACTION), CB_SETCURSEL, 1, 0);
                g.inTray = true;
                CreateTrayIcon(hwnd);
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            } else {
                g.closeAction = 2;
                SaveAllSettings();
                SendMessage(GetDlgItem(hwnd, IDC_COMBO_CLOSE_ACTION), CB_SETCURSEL, 2, 0);
                break;
            }
        }

    case WM_TRAYICON:
        if (wp == 1) {
            if (LOWORD(lp) == WM_LBUTTONUP) {
                g.inTray = false;
                RemoveTrayIcon(hwnd);
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
            } else if (LOWORD(lp) == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                AppendMenu(hMenu, MF_STRING, 1, L"显示窗口");
                AppendMenu(hMenu, MF_STRING, 2, L"退出");
                SetForegroundWindow(hwnd);
                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                                         pt.x, pt.y, 0, hwnd, nullptr);
                DestroyMenu(hMenu);
                if (cmd == 1) {
                    g.inTray = false;
                    RemoveTrayIcon(hwnd);
                    ShowWindow(hwnd, SW_RESTORE);
                    SetForegroundWindow(hwnd);
                } else if (cmd == 2) {
                    RemoveTrayIcon(hwnd);
                    DestroyWindow(hwnd);
                }
            }
        }
        return 0;

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        HWND hwStatic = (HWND)lp;
        int id = GetDlgCtrlID(hwStatic);
        SetBkMode(hdc, TRANSPARENT);
        if (id == IDC_STATUS_LABEL) {
            if (g.running)
                SetTextColor(hdc, RGB(0, 180, 0));
            else if (g.isReplaying)
                SetTextColor(hdc, RGB(0, 120, 220));
            else if (g.isRecording)
                SetTextColor(hdc, RGB(220, 50, 50));
            else
                SetTextColor(hdc, RGB(160, 160, 160));
        }
        static HBRUSH hBg = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
        return (LRESULT)hBg;
    }

    case WM_DESTROY:
        if (g.hwndSettings) DestroyWindow(g.hwndSettings);
        UnregisterHotKey(hwnd, 1);
        UnregisterHotKey(hwnd, 2);
        UnregisterHotKey(hwnd, 3);
        KillTimer(hwnd, IDC_TIMER);
        KillTimer(hwnd, IDC_REPLAY_TIMER);
        StopRecording(hwnd);
        RemoveTrayIcon(hwnd);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

// ── 入口点 ───────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    srand((unsigned)time(nullptr));
    g_hInst = hInst;

    const wchar_t CLASS_NAME[] = L"AutoClickerWindow";
    const wchar_t SETTINGS_CLASS[] = L"SettingsWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(1));
    RegisterClass(&wc);

    wc.lpfnWndProc   = SettingsWndProc;
    wc.lpszClassName = SETTINGS_CLASS;
    wc.hIcon         = nullptr;
    RegisterClass(&wc);

    int x  = (GetSystemMetrics(SM_CXSCREEN) - MAIN_WIDTH) / 2;
    int y  = (GetSystemMetrics(SM_CYSCREEN) - MAIN_HEIGHT ) / 2;

    std::wstring AppName = L"连点器  Auto Clicker";
    AppName += L" v" VERSION_NUMBER;
    HWND hwnd = CreateWindowEx(
        g.topmost ? WS_EX_TOPMOST : 0,
        CLASS_NAME,
        AppName.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, MAIN_WIDTH, MAIN_HEIGHT,
        nullptr, nullptr, hInst, nullptr
    );

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE && g.pickingPosition) {
            g.pickingPosition = false;
            if (g_pickHook) { UnhookWindowsHookEx(g_pickHook); g_pickHook = nullptr; }
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            SetDlgItemText(hwnd, IDC_BTN_PICK_POS, L"📌");
            continue;
        }
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
