#pragma once
#include <string>
#include <vector>

namespace vision {

// VisionA running config (load from vision.yml)
struct VisionConfig {
    // ===================== fields ===================== //

    std::string seats_json   = "assets/vision/config/demo_seats.json";  // seats ROI configs
    std::string model_path   = "assets/vision/weights/fine_tune02.onnx";// available onnx models path
    std::string vision_yaml  = "assets/vision/config/vision.yml";       // config file self path (yaml file contains the overall configuration for VisionA)
    std::string log_dir      = "logs";
    std::string snapshot_dir = "cache/snap";
    std::string states_output = "runtime/seat_states.jsonl";    // seat state output path (.jsonl)
    std::string annotated_frames_dir = "annotate/BBox";         // annotation output path
    bool annotate_enabled = false;                              // enable annotation

    // model input size
    int input_w = 640;
    int input_h = 640;

    // thresholds
    float conf_thres_person      = 0.65f; // person threshold, box_conf > ~: is person
    float conf_thres_person_low  = 0.50f; // person low thresh, judge with other cond. if between
    float conf_thres_object      = 0.361f;// object threshold, box_conf > ~: is object
    float nms_iou                = 0.55f; // overlapping box IoU thres
    float iou_seat_intersect     = 0.40f; // IoU thres
    float mog2_fg_ratio_thres    = 0.08f; // foreground ratio threshold

    // Snapshot policy
    int snapshot_jpg_quality     = 90;
    int snapshot_min_interval_ms = 3000;  // min saving time interval
    bool snapshot_on_change_only = true;  // whether save only if state chg
    int snapshot_heartbeat_ms    = 5000;  // save periodically even state not chg

    // object class allowed
    std::vector<std::string> object_allow = {
        "laptop","pad","bag","book","phone","bottle","clothes","umbrella","other","backpack"
    };

    // annotation
    int annotated_save_freq = 100;          // annotation save freq. (save every N frames)

    // ONNX Runtime 线程配置
    int intra_threads = 0; // 0=auto

    // MOG2 args
    int mog2_history         = 500;     // bg model 历史帧数, ↑: 稳定性↑ 适应性↓
    int mog2_var_threshold   = 16;      // 判定属于bg的方差阈值
    bool mog2_detect_shadows = false;   // 是否检测阴影

    // fg noise post_process
    bool fg_morph_enable = false;       // 是否启用形态学处理
    int  fg_morph_erode_iterations = 0;

    // Perf / test
    bool dump_perf_log = true;
    bool enable_async_snapshot = true;

    std::string yolo_variant = "yolov8n"; // or "yolov5", "yolov8"

    // only use multiclass model: true 
    bool use_single_multiclass_model = true;

    // ===================== methods ===================== //

    // Load configs
    static VisionConfig fromYaml(const std::string& yaml_path);
    static VisionConfig fromJson(const std::string& json_path);
};

} // namespace vision
