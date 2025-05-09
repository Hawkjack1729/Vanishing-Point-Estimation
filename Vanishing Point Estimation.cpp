#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;
using namespace cv;
using namespace std;

Point2f computeIntersection(Vec4i a, Vec4i b) {
    Point2f p1(a[0], a[1]), p2(a[2], a[3]);
    Point2f q1(b[0], b[1]), q2(b[2], b[3]);

    float A1 = p2.y - p1.y;
    float B1 = p1.x - p2.x;
    float C1 = A1 * p1.x + B1 * p1.y;

    float A2 = q2.y - q1.y;
    float B2 = q1.x - q2.x;
    float C2 = A2 * q1.x + B2 * q1.y;

    float det = A1 * B2 - A2 * B1;
    if (fabs(det) < 1e-10) return Point2f(-1, -1);

    float x = (B2 * C1 - B1 * C2) / det;
    float y = (A1 * C2 - A2 * C1) / det;
    return Point2f(x, y);
}

void processImage(const string& inputPath, const string& outputPath) {
    Mat img = imread(inputPath);
    if (img.empty()) {
        cerr << "Failed to load: " << inputPath << endl;
        return;
    }

    Mat gray, blurred, edges;
    cvtColor(img, gray, COLOR_BGR2GRAY);
    GaussianBlur(gray, blurred, Size(5, 5), 1.5);
    Canny(blurred, edges, 50, 150);

    vector<Vec4i> lines;
    HoughLinesP(edges, lines, 1, CV_PI / 180, 100, 100, 10);

    vector<Vec4i> filtered;
    for (auto& l : lines) {
        float dx = l[2] - l[0];
        float dy = l[3] - l[1];
        float angle = atan2(dy, dx) * 180 / CV_PI;
        if (fabs(angle) > 20 && fabs(angle) < 160)
            filtered.push_back(l);
    }

    vector<Point2f> intersections;
    for (size_t i = 0; i < filtered.size(); i++) {
        for (size_t j = i + 1; j < filtered.size(); j++) {
            Point2f pt = computeIntersection(filtered[i], filtered[j]);
            if (pt.x >= 0 && pt.y >= 0 && pt.x < img.cols * 2 && pt.y < img.rows * 2) {
                intersections.push_back(pt);
            }
        }
    }

    Point2f vp(0, 0);
    for (auto& p : intersections) vp += p;
    if (!intersections.empty()) vp *= 1.0 / intersections.size();

    Mat result = img.clone();
    for (auto& l : filtered)
        line(result, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 255, 0), 2);

    if (!intersections.empty())
        circle(result, vp, 10, Scalar(0, 0, 255), -1);

    imwrite(outputPath, result);
    cout << "Processed: " << inputPath << " → " << outputPath << endl;
}

int main() {
    string folder = "Estimate_vanishing_points_data";
    string outputFolder = "output_results";
    fs::create_directories(outputFolder);

    for (const auto& entry : fs::directory_iterator(folder)) {
        string path = entry.path().string();
        string filename = entry.path().filename().string();
        string outputPath = outputFolder + "/" + filename;
        processImage(path, outputPath);
    }

    return 0;
}

