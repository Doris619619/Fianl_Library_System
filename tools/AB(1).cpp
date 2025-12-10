#include <iostream>
#include <QThread>
#include <filesystem>
#include "seatui/vision/VisionClient.h"
#include "seatui/judger/seat_state_judger.hpp"

using namespace std;
namespace fs = std::filesystem;

// --------------------- Vision 线程 ---------------------
class VisionThread : public QThread {
public:
    void run() override {
        vision::VisionClient vc;
        cout << "[VisionThread] 开始运行 VisionClient ..." << endl;

        // 输入视频或目录（相对 Release）
        string video_path = "../assets/vision/videos/demo.mp4";
        string out_jsonl  = "../out/000000.jsonl";

        vc.runVision(
            video_path,  // 输入视频或目录
            out_jsonl,   // 输出 JSONL 到 out
            "500",       // 最大帧数
            "0.5",       // FPS 抽帧
            "true"       // 是否流式处理
        );

        cout << "[VisionThread] VisionClient 运行结束" << endl;
    }
};

// --------------------- Judger 线程 ---------------------
class JudgerThread : public QThread {
public:
    void run() override {
        SeatStateJudger judger;
        cout << "[JudgerThread] 开始运行 SeatStateJudger ..." << endl;

        // Judger 监听 out 目录（相对于 Release）
        judger.run("../out");

        cout << "[JudgerThread] SeatStateJudger 运行结束" << endl;
    }
};

int main() {
    // 确保输出目录存在
    fs::path out_folder("../out");
    if (!fs::exists(out_folder)) {
        fs::create_directory(out_folder);
    }

    // 启动 Vision 线程
    auto* visionThread = new VisionThread();
    visionThread->start();

    // 启动 Judger 线程
    auto* judgerThread = new JudgerThread();
    judgerThread->start();

    cout << "[Main] Vision + Judger 已启动" << endl;

    // 阻塞主线程，等待两个线程结束（可用 Ctrl+C 停止）
    visionThread->wait();
    judgerThread->wait();

    cout << "[Main] 程序结束" << endl;
    return 0;
}
