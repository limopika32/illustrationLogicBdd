#include <iostream>
#include <chrono>
#include <ctime>

int main() {
    // 現在時刻をsystem_clockを用いて取得
    auto now = std::chrono::system_clock::now();

    // 現在時刻をtime_t形式で出力
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::cout << "Time Recorded at " << std::ctime(&now_c);

    // ミリ秒単位での現在時刻を取得
    // auto duration = now.time_since_epoch();
    // auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    // std::cout << ": " << millis << std::endl;

    return 0;
}