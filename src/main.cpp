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
#include <commdlg.h>
#include <shellapi.h>
#include <mmsystem.h>

// ── 控件 ID ──────────────────────────────────────
#define IDC_BTN_LEFT        1001
#define IDC_BTN_RIGHT       1002
#define IDC_BTN_MIDDLE      1003
#define IDC_SINGLE          1004
#define IDC_DOUBLE          1005
#define IDC_EDIT_SEC        1006
#define IDC_EDIT_MS         1007
#define IDC_EDIT_US         1008
#define IDC_REPEAT_FOREVER  1010
#define IDC_REPEAT_COUNT    1011
#define IDC_EDIT_COUNT      1012
#define IDC_CURSOR_FOLLOW   1013
#define IDC_CURSOR_FIXED    1014
#define IDC_EDIT_X          1015
#define IDC_EDIT_Y          1016
#define IDC_BTN_START       1018
#define IDC_BTN_STOP        1019
#define IDC_COUNT_LABEL     1021
#define IDC_STATUS_LABEL    1022
#define IDC_CHECK_TOPMOST   1023
#define IDC_BTN_RECORD      1024
#define IDC_BTN_REPLAY      1025
#define IDC_BTN_CLEAR_REC   1026
#define IDC_LIST_RECORD     1027
#define IDC_REPLAY_LOOP     1028
#define IDC_EDIT_REPLAY_N   1029
#define IDC_LABEL_RECORD    1030
#define IDC_BTN_EXPORT      1031
#define IDC_BTN_IMPORT      1032
#define IDC_LIST_IMPORTED   1033
#define IDC_CHECK_TRAJECTORY 1034
#define IDC_CHECK_HIDE      1035
#define IDC_COMBO_SAMPLE_RATE  1036
#define IDC_COMBO_CLOSE_ACTION 1037
#define IDC_CHECK_RAND_DELAY   1038
#define IDC_CHECK_RAND_POS     1039
#define IDC_CHECK_RECORD_SELF  1040
#define IDC_BTN_PICK_POS      1041
#define IDC_TIMER              1
#define IDC_REPLAY_TIMER       2
#define WM_TRAYICON            (WM_APP + 1)

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
    bool topmost = false; // 置顶行为
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
} g;

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

    SetCursorPos(pt.x, pt.y);
    Sleep(3);

    INPUT in = {};
    in.type = INPUT_MOUSE;
    int times = (mode == 0) ? 1 : 2;

    for (int t = 0; t < times; t++) {
        in.mi.dwFlags = down;
        SendInput(1, &in, sizeof(INPUT));
        Sleep(10);
        in.mi.dwFlags = up;
        SendInput(1, &in, sizeof(INPUT));
        if (t < times - 1) Sleep(30); // 双击间隔
    }
}

void SimulateDrag(int button, POINT from, POINT to, DWORD holdMs) {
    UINT down, up;
    GetMouseFlags(button, down, up);
    if (!down) return;

    INPUT in = {};
    in.type = INPUT_MOUSE;

    // 1) 移动到起点
    SetCursorPos(from.x, from.y);
    Sleep(5);

    // 2) 按下
    in.mi.dwFlags = down;
    SendInput(1, &in, sizeof(INPUT));
    Sleep(20); // 让目标程序感知到按下

    // 3) 移动到终点 (拖动)
    SetCursorPos(to.x, to.y);
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &in, sizeof(INPUT));

    // 4) 按住等待
    DWORD elapsed = 25; // 已用约25ms
    if (holdMs > elapsed) {
        Sleep(holdMs - elapsed);
    }

    // 5) 松手
    in.mi.dwFlags = up;
    SendInput(1, &in, sizeof(INPUT));
    Sleep(10);
}

void SimulateTrajectoryDrag(const RecordedAction& a) {
    UINT down, up;
    GetMouseFlags(a.button, down, up);
    if (!down) return;

    INPUT in = {};
    in.type = INPUT_MOUSE;

    // 1) 移动到起点 + 按下
    SetCursorPos(a.pt.x, a.pt.y);
    Sleep(5);
    in.mi.dwFlags = down;
    SendInput(1, &in, sizeof(INPUT));
    Sleep(15); // 让目标程序感知到按下

    // 2) 逐点回放轨迹
    int nPts = (int)a.trajectory.size() + 1; // +1 for endPt
    DWORD interval = a.holdMs / (nPts > 0 ? nPts : 1);
    if (interval < 3) interval = 3;
    if (interval > 50) interval = 50;

    for (auto& pt : a.trajectory) {
        Sleep(interval);
        SetCursorPos(pt.x, pt.y);
        in.mi.dwFlags = MOUSEEVENTF_MOVE;
        SendInput(1, &in, sizeof(INPUT));
    }

    // 3) 移到终点
    Sleep(interval);
    SetCursorPos(a.endPt.x, a.endPt.y);
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &in, sizeof(INPUT));
    Sleep(10);

    // 4) 松手
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

void DoClick() {
    POINT pt;
    if (!g.followCursor) {
        pt.x = g.fixedX;
        pt.y = g.fixedY;
    } else {
        GetCursorPos(&pt);
    }
    if (g.randomizePosition) {
        pt.x += RandomRange(-4, 4);
        pt.y += RandomRange(-4, 4);
    }
    SimulateClick(g.mouseButton, g.clickMode, pt);
}

// ── 录制 ────────────────────────────────────────
void StopReplay(HWND hwnd); // 前置声明
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
    g.actions.clear();
    g.selectedImport = -1;
    g.timeline.clear();
    g.lastTimelineUs = 0;
    g.lastRecordTick = GetTickCount();
    g.isRecording = true;
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
    SetDlgItemText(hwnd, IDC_BTN_RECORD, L"● 录制");
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

    // 处理所有 accumulate delta ≤ elapsed 的事件
    while (g.timelineReplayIdx < g.timeline.size()) {
        if (g.replayAccumUs + g.timeline[g.timelineReplayIdx].deltaUs > elapsedUs)
            break;  // 还没到时间

        auto& tp = g.timeline[g.timelineReplayIdx];
        g.replayAccumUs += tp.deltaUs;

        SetCursorPos(tp.pt.x, tp.pt.y);

        if (tp.event == -1) {
            UINT down, up;
            GetMouseFlags(tp.button, down, up);
            if (down) {
                INPUT in = {};
                in.type = INPUT_MOUSE;
                in.mi.dwFlags = down;
                SendInput(1, &in, sizeof(INPUT));
            }
        } else if (tp.event == -2) {
            UINT down, up;
            GetMouseFlags(tp.button, down, up);
            if (up) {
                INPUT in = {};
                in.type = INPUT_MOUSE;
                in.mi.dwFlags = up;
                SendInput(1, &in, sizeof(INPUT));
            }
        }

        g.clickCount++;
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
        // 下一轮循环
        g.timelineReplayIdx = 0;
        g.replayAccumUs = 0;
        g.replayBaseUs = GetTimeUs();
    }
}

void StartReplay(HWND hwnd) {
    if (g.isReplaying) return;
    if (g.running) return;
    if (g.actions.empty() && g.timeline.empty()) return;

    wchar_t buf[16];
    GetDlgItemText(hwnd, IDC_EDIT_REPLAY_N, buf, 16);
    g.replayCount = _wtoi(buf);
    if (g.replayCount <= 0) g.replayCount = 1;

    g.replayLoop = (SendMessage(GetDlgItem(hwnd, IDC_REPLAY_LOOP), BM_GETCHECK, 0, 0) == BST_CHECKED);
    g.currentReplay = 0;
    g.isReplaying = true;
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
    SetDlgItemText(hwnd, IDC_BTN_REPLAY, L"▶ 播放(F8)");
    EnableWindow(GetDlgItem(hwnd, IDC_BTN_START), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_BTN_RECORD), TRUE);
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

    BOOL fixed = !g.followCursor;
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_X), fixed);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_Y), fixed);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_COUNT), !g.repeatForever);
}

void StartClicking(HWND hwnd) {
    if (g.running || g.isReplaying) return;

    ReadInterval(hwnd);

    if (!g.repeatForever) {
        wchar_t buf[16];
        GetDlgItemText(hwnd, IDC_EDIT_COUNT, buf, 16);
        g.targetCount = _wtoi(buf);
        if (g.targetCount <= 0) g.targetCount = 100;
    }

    if (!g.followCursor) {
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
HWND CreateLabel(HWND p, const wchar_t* t, int x, int y, int w, int h) {
    return CreateWindow(L"STATIC", t, WS_CHILD | WS_VISIBLE, x, y, w, h,
                        p, nullptr, GetModuleHandle(nullptr), nullptr);
}

HWND CreateRadio(HWND p, const wchar_t* t, int x, int y, int w, int h, int id, bool chk) {
    HWND c = CreateWindow(L"BUTTON", t,
                          WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | (chk ? WS_TABSTOP : 0),
                          x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
    if (chk) SendMessage(c, BM_SETCHECK, BST_CHECKED, 0);
    return c;
}

HWND CreateEdit(HWND p, const wchar_t* t, int x, int y, int w, int h, int id) {
    return CreateWindow(L"EDIT", t,
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_RIGHT | WS_TABSTOP,
                        x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
}

HWND CreateCheck(HWND p, const wchar_t* t, int x, int y, int w, int h, int id, bool chk) {
    HWND c = CreateWindow(L"BUTTON", t,
                          WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                          x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
    if (chk) SendMessage(c, BM_SETCHECK, BST_CHECKED, 0);
    return c;
}

HWND CreateCombo(HWND p, int x, int y, int w, int h, int id) {
    return CreateWindow(L"COMBOBOX", nullptr,
                        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                        x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
}

HWND CreateButton(HWND p, const wchar_t* t, int x, int y, int w, int h, int id) {
    return CreateWindow(L"BUTTON", t,
                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
}

HWND CreateSep(HWND p, int x, int y, int w) {
    return CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                        x, y, w, 2, p, nullptr, GetModuleHandle(nullptr), nullptr);
}

// ── 托盘 & 注册表 ────────────────────────────────
void SaveCloseAction() {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\AutoClicker",
                       0, nullptr, REG_OPTION_NON_VOLATILE,
                       KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD val = (DWORD)g.closeAction;
        RegSetValueEx(hKey, L"CloseAction", 0, REG_DWORD, (BYTE*)&val, sizeof(val));
        RegCloseKey(hKey);
    }
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

// ── 窗口过程 ─────────────────────────────────────
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
        CreateRadio(hwnd, L"左键", 20, 42, 70, 22, IDC_BTN_LEFT, true);
        CreateRadio(hwnd, L"右键", 110, 42, 70, 22, IDC_BTN_RIGHT, false);
        CreateRadio(hwnd, L"中键", 200, 42, 70, 22, IDC_BTN_MIDDLE, false);

        // ── 点击模式 ──
        CreateLabel(hwnd, L"点击模式:", 20, 78, 80, 22);
        CreateRadio(hwnd, L"单击", 20, 104, 80, 22, IDC_SINGLE, true);
        CreateRadio(hwnd, L"双击", 120, 104, 80, 22, IDC_DOUBLE, false);

        CreateSep(hwnd, 20, 136, 410);

        // ── 点击间隔 (秒/毫秒/微秒) ──
        CreateLabel(hwnd, L"点击间隔:", 20, 146, 80, 22);
        CreateLabel(hwnd, L"秒", 20, 176, 20, 22);
        CreateEdit(hwnd, L"0", 40, 174, 50, 22, IDC_EDIT_SEC);
        CreateLabel(hwnd, L"毫秒", 102, 176, 35, 22);
        CreateEdit(hwnd, L"100", 138, 174, 55, 22, IDC_EDIT_MS);
        CreateLabel(hwnd, L"微秒", 205, 176, 35, 22);
        CreateEdit(hwnd, L"0", 240, 174, 65, 22, IDC_EDIT_US);

        CreateSep(hwnd, 20, 208, 410);

        // ── 重复设置 ──
        CreateLabel(hwnd, L"重复模式:", 20, 218, 80, 22);
        CreateRadio(hwnd, L"重复直到停止", 20, 244, 120, 22, IDC_REPEAT_FOREVER, true);
        CreateRadio(hwnd, L"指定次数", 155, 244, 90, 22, IDC_REPEAT_COUNT, false);
        CreateEdit(hwnd, L"100", 255, 244, 55, 22, IDC_EDIT_COUNT);

        CreateSep(hwnd, 20, 276, 410);

        // ── 光标位置 ──
        CreateLabel(hwnd, L"光标位置:", 20, 286, 80, 22);
        CreateRadio(hwnd, L"跟随鼠标", 20, 312, 100, 22, IDC_CURSOR_FOLLOW, true);
        CreateRadio(hwnd, L"固定位置", 130, 312, 100, 22, IDC_CURSOR_FIXED, false);
        CreateLabel(hwnd, L"X:", 240, 314, 18, 22);
        CreateEdit(hwnd, L"0", 258, 312, 55, 22, IDC_EDIT_X);
        CreateLabel(hwnd, L"Y:", 320, 314, 18, 22);
        CreateEdit(hwnd, L"0", 338, 312, 55, 22, IDC_EDIT_Y);
        CreateButton(hwnd, L"📌", 400, 312, 28, 22, IDC_BTN_PICK_POS);

        CreateSep(hwnd, 20, 344, 410);

        // ── 选项 ──
        CreateCheck(hwnd, L"置顶", 20, 354, 55, 24, IDC_CHECK_TOPMOST, g.topmost);
        CreateCheck(hwnd, L"记录轨迹", 80, 354, 85, 24, IDC_CHECK_TRAJECTORY, true);
        CreateLabel(hwnd, L"采样率:", 170, 356, 50, 20);
        HWND hSampleCombo = CreateCombo(hwnd, 215, 353, 80, 200, IDC_COMBO_SAMPLE_RATE);
        CreateCheck(hwnd, L"开始时隐藏", 305, 354, 105, 24, IDC_CHECK_HIDE, true);
        CreateCheck(hwnd, L"录制程序界面上的光标", 20, 380, 200, 24, IDC_CHECK_RECORD_SELF, false);

        CreateSep(hwnd, 20, 412, 410);

        // ── 关闭行为 ──
        CreateLabel(hwnd, L"关闭行为:", 20, 422, 80, 20);
        HWND hCloseCombo = CreateCombo(hwnd, 100, 420, 200, 200, IDC_COMBO_CLOSE_ACTION);

        // 填充采样率选项
        const wchar_t* hzItems[] = {L"125 Hz", L"250 Hz", L"500 Hz", L"1K Hz", L"2K Hz", L"4K Hz", L"8K Hz"};
        for (int i = 0; i < 7; i++) SendMessage(hSampleCombo, CB_ADDSTRING, 0, (LPARAM)hzItems[i]);
        SendMessage(hSampleCombo, CB_SETCURSEL, 3, 0); // 默认 1K Hz

        // 填充关闭行为选项
        SendMessage(hCloseCombo, CB_ADDSTRING, 0, (LPARAM)L"询问");
        SendMessage(hCloseCombo, CB_ADDSTRING, 0, (LPARAM)L"最小化到托盘");
        SendMessage(hCloseCombo, CB_ADDSTRING, 0, (LPARAM)L"退出程序");

        // 从注册表加载关闭行为
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\AutoClicker", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD val = 0, sz = sizeof(val);
            RegQueryValueEx(hKey, L"CloseAction", nullptr, nullptr, (LPBYTE)&val, &sz);
            if (val <= 2) g.closeAction = (int)val;
            RegCloseKey(hKey);
        }
        SendMessage(hCloseCombo, CB_SETCURSEL, g.closeAction, 0);

        // ── 反检测 ──
        CreateCheck(hwnd, L"随机间隔 ±20%", 20, 454, 130, 24, IDC_CHECK_RAND_DELAY, true);
        CreateCheck(hwnd, L"位置微抖 ±4px", 160, 454, 130, 24, IDC_CHECK_RAND_POS, true);

        // ── 操作录制 ──
        CreateLabel(hwnd, L"操作录制:", 20, 488, 80, 22);

        CreateButton(hwnd, L"● 录制(F7)", 20, 512, 75, 28, IDC_BTN_RECORD);
        CreateButton(hwnd, L"▶ 播放(F8)", 98, 512, 80, 28, IDC_BTN_REPLAY);
        CreateButton(hwnd, L"清空", 176, 512, 55, 28, IDC_BTN_CLEAR_REC);
        CreateButton(hwnd, L"导出", 236, 512, 55, 28, IDC_BTN_EXPORT);
        CreateButton(hwnd, L"导入", 296, 512, 55, 28, IDC_BTN_IMPORT);

        // 已导入文件列表
        CreateLabel(hwnd, L"已导入文件 (点击切换):", 20, 548, 180, 20);

        g.hImportedList = CreateWindow(L"LISTBOX", nullptr,
                                        WS_CHILD | WS_VISIBLE | WS_BORDER |
                                        WS_VSCROLL | LBS_NOTIFY,
                                        20, 568, 410, 52, hwnd,
                                        (HMENU)(INT_PTR)IDC_LIST_IMPORTED,
                                        GetModuleHandle(nullptr), nullptr);
        // 初始化：只有"当前录制"
        SendMessage(g.hImportedList, LB_ADDSTRING, 0, (LPARAM)L"<当前录制>");
        SendMessage(g.hImportedList, LB_SETITEMDATA, 0, -1);
        SendMessage(g.hImportedList, LB_SETCURSEL, 0, 0);

        // 播放设置 + 当前动作数
        SetDlgItemText(hwnd, IDC_LABEL_RECORD, L"当前动作: 0 个");
        CreateLabel(hwnd, L"当前动作: 0 个", 20, 626, 140, 20);
        CreateLabel(hwnd, L"播放次数:", 170, 626, 70, 22);
        CreateEdit(hwnd, L"1", 240, 624, 40, 22, IDC_EDIT_REPLAY_N);
        CreateCheck(hwnd, L"循环", 290, 626, 55, 22, IDC_REPLAY_LOOP, false);

        g.hList = CreateWindow(L"LISTBOX", nullptr,
                                WS_CHILD | WS_VISIBLE | WS_BORDER |
                                WS_VSCROLL | LBS_NOSEL | LBS_NOTIFY,
                                20, 652, 410, 84, hwnd,
                                (HMENU)(INT_PTR)IDC_LIST_RECORD,
                                GetModuleHandle(nullptr), nullptr);

        CreateSep(hwnd, 20, 744, 410);

        // ── 底部状态区 ──
        CreateLabel(hwnd, L"热键: F6 开始    F7 录制    F8 播放", 20, 752, 260, 22);

        SetDlgItemText(hwnd, IDC_COUNT_LABEL, L"已点击: 0 次");
        CreateLabel(hwnd, L"已点击: 0 次", 300, 752, 130, 22);

        SetDlgItemText(hwnd, IDC_STATUS_LABEL, L"●  已停止");
        CreateLabel(hwnd, L"●  已停止", 20, 780, 120, 22);

        CreateButton(hwnd, L"▶  开始 (F6)", 240, 742, 95, 30, IDC_BTN_START);
        CreateButton(hwnd, L"■  停止", 340, 742, 90, 30, IDC_BTN_STOP);

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

        RegisterHotKey(hwnd, 1, 0, VK_F6);
        RegisterHotKey(hwnd, 2, 0, VK_F7);
        RegisterHotKey(hwnd, 3, 0, VK_F8);

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
        if (g.pickingPosition) {
            POINT pt;
            GetCursorPos(&pt);
            g.fixedX = pt.x; g.fixedY = pt.y;
            wchar_t buf[16];
            _itow_s(g.fixedX, buf, 10); SetDlgItemText(hwnd, IDC_EDIT_X, buf);
            _itow_s(g.fixedY, buf, 10); SetDlgItemText(hwnd, IDC_EDIT_Y, buf);
            g.followCursor = false;
            g.pickingPosition = false;
            ReleaseCapture();
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            SetDlgItemText(hwnd, IDC_BTN_PICK_POS, L"📌");
            CheckRadioButton(hwnd, IDC_CURSOR_FOLLOW, IDC_CURSOR_FIXED, IDC_CURSOR_FIXED);
            UpdateUI(hwnd);
            return 0;
        }
        SetFocus(hwnd);  // 点击窗口空白处使输入框失焦
        return 0;

    case WM_RBUTTONDOWN:
        if (g.pickingPosition) {
            g.pickingPosition = false;
            ReleaseCapture();
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            SetDlgItemText(hwnd, IDC_BTN_PICK_POS, L"📌");
            return 0;
        }
        return 0;

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE && g.pickingPosition) {
            g.pickingPosition = false;
            ReleaseCapture();
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            SetDlgItemText(hwnd, IDC_BTN_PICK_POS, L"📌");
            return 0;
        }
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
                g.clickCount++;
                g.replayIndex++;

                std::wstringstream ss;
                ss << L"已点击: " << g.clickCount << L" 次";
                SetDlgItemText(hwnd, IDC_COUNT_LABEL, ss.str().c_str());

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
        case IDC_BTN_LEFT:    g.mouseButton = 0; return 0;
        case IDC_BTN_RIGHT:   g.mouseButton = 1; return 0;
        case IDC_BTN_MIDDLE:  g.mouseButton = 2; return 0;
        case IDC_SINGLE:      g.clickMode = 0; return 0;
        case IDC_DOUBLE:      g.clickMode = 1; return 0;

        case IDC_REPEAT_FOREVER: g.repeatForever = true;  UpdateUI(hwnd); return 0;
        case IDC_REPEAT_COUNT:   g.repeatForever = false; UpdateUI(hwnd); return 0;

        case IDC_CURSOR_FOLLOW: g.followCursor = true;  UpdateUI(hwnd); return 0;
        case IDC_CURSOR_FIXED:  g.followCursor = false; UpdateUI(hwnd); return 0;
        case IDC_BTN_PICK_POS:
            g.pickingPosition = true;
            SetCapture(hwnd);
            SetCursor(LoadCursor(nullptr, IDC_CROSS));
            SetDlgItemText(hwnd, IDC_BTN_PICK_POS, L"...");
            return 0;

        case IDC_CHECK_TOPMOST:
            g.topmost = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_TOPMOST), BM_GETCHECK, 0, 0) == BST_CHECKED);
            ToggleTopmost(hwnd);
            return 0;

        case IDC_CHECK_TRAJECTORY:
            g.recordTrajectory = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_TRAJECTORY), BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;

        case IDC_CHECK_HIDE:
            g.hideOnAction = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_HIDE), BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;

        case IDC_CHECK_RECORD_SELF:
            g.recordSelfWin = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_RECORD_SELF), BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;

        case IDC_CHECK_RAND_DELAY:
            g.randomizeInterval = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_RAND_DELAY), BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;

        case IDC_CHECK_RAND_POS:
            g.randomizePosition = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_RAND_POS), BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;

        case IDC_COMBO_SAMPLE_RATE:
            if (HIWORD(wp) == CBN_SELCHANGE) {
                int hzVals[] = {125, 250, 500, 1000, 2000, 4000, 8000};
                int idx = (int)SendMessage(GetDlgItem(hwnd, IDC_COMBO_SAMPLE_RATE), CB_GETCURSEL, 0, 0);
                if (idx >= 0 && idx < 7) g.sampleRateHz = hzVals[idx];
            }
            return 0;

        case IDC_COMBO_CLOSE_ACTION:
            if (HIWORD(wp) == CBN_SELCHANGE) {
                g.closeAction = (int)SendMessage(GetDlgItem(hwnd, IDC_COMBO_CLOSE_ACTION), CB_GETCURSEL, 0, 0);
            }
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
                SaveCloseAction();
                SendMessage(GetDlgItem(hwnd, IDC_COMBO_CLOSE_ACTION), CB_SETCURSEL, 1, 0);
                g.inTray = true;
                CreateTrayIcon(hwnd);
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            } else {
                g.closeAction = 2;
                SaveCloseAction();
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
        return (LRESULT)GetStockObject(HOLLOW_BRUSH);
    }

    case WM_DESTROY:
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

    WNDCLASS wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(1));

    RegisterClass(&wc);

    int ww = 450, wh = 850;
    int x  = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2;
    int y  = (GetSystemMetrics(SM_CYSCREEN) - wh) / 2;

    HWND hwnd = CreateWindowEx(
        g.topmost ? WS_EX_TOPMOST : 0,
        CLASS_NAME,
        L"连点器  Auto Clicker",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, ww, wh,
        nullptr, nullptr, hInst, nullptr
    );

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
