#include "seatui/vision/Mog2.h"

namespace vision {

Mog2Manager::Mog2Manager(const Mog2Config& cfg) {
    mog2_ = cv::createBackgroundSubtractorMOG2(cfg.history,         // background model memory length, larger stabler
                                               cfg.var_threshold,   // judger if pixel is bg
                                               cfg.detect_shadows); // whether detect shadow
}

cv::Mat Mog2Manager::apply(const cv::Mat& bgr) {
    cv::Mat fg;
    mog2_->apply(bgr, fg);  // call apply from cv
    return fg;
}

// Get fg_ratio in rect
float Mog2Manager::ratioInRoi(const cv::Mat& fg, const cv::Rect& roi) const {
    cv::Rect bounded = roi & cv::Rect(0,0, fg.cols, fg.rows);
    if (bounded.width <=0 || bounded.height <=0) return 0.f;
    cv::Mat sub = fg(bounded);                     // take sub-part of fg model (ROI area)
    int nonzero = cv::countNonZero(sub);           // cnt fg pixel in ROI
    int total = bounded.width * bounded.height;
    if (total == 0) return 0.f;
    return static_cast<float>(nonzero) / static_cast<float>(total);  // fg_ratio = fg_pixel_cnt / ROI_area
}

// Get fg_ratio in poly
static float ratioInPolyImpl(const cv::Mat& fg, const std::vector<cv::Point>& poly) {
    if (poly.size() < 3) return 0.f;
    cv::Rect bounds = cv::boundingRect(poly) & cv::Rect(0,0, fg.cols, fg.rows); // bounding-rect of poly
    if (bounds.width <= 0 || bounds.height <= 0) return 0.f;
    std::vector<cv::Point> shifted; 
    shifted.reserve(poly.size());
    for (auto &p : poly) shifted.emplace_back(p.x - bounds.x, p.y - bounds.y);  //  
    cv::Mat mask(bounds.height, bounds.width, CV_8UC1, cv::Scalar(0));
    std::vector<std::vector<cv::Point>> contours = { shifted };
    cv::fillPoly(mask, contours, cv::Scalar(255));
    cv::Mat fg_sub = fg(bounds), fg_inside;
    cv::bitwise_and(fg_sub, mask, fg_inside);
    int nonzero = cv::countNonZero(fg_inside);
    int area = cv::countNonZero(mask);
    if (area == 0) return 0.f;
    return static_cast<float>(nonzero) / static_cast<float>(area);
}

float ratioInPoly(const cv::Mat& fg, const std::vector<cv::Point>& poly) {
    return ratioInPolyImpl(fg, poly);
}

float Mog2Manager::ratioInPoly(const cv::Mat& fg, const std::vector<cv::Point>& poly) {
    return ratioInPolyImpl(fg, poly);
}

} // namespace vision
