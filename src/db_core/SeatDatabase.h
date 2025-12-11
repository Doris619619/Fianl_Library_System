#ifndef SEAT_DATABASE_H
#define SEAT_DATABASE_H

//#include "SQLiteCpp/SQLiteCpp.h"
#include "../../../third_party/sqlite/include/SQLiteCpp/SQLiteCpp.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>

#include "DataTypes.h"  

class SeatDatabase {
public:
    // Get instance using Singleton pattern
    static SeatDatabase& getInstance(const std::string& db_path = "../out/seating_system.db");
    
    // Copying and assignment are prohibited
    SeatDatabase(const SeatDatabase&) = delete;
    SeatDatabase& operator=(const SeatDatabase&) = delete;
    
    // Database Initialization
    bool initialize();
    
    // Insert operation
    bool insertSeatEvent(const std::string& seat_id, 
                        const std::string& state, // "Seated", "Unseated", "Anomaly"
                        const std::string& timestamp, 
                        int duration_sec = 0);
    
    bool insertSnapshot(const std::string& timestamp, 
                       const std::string& seat_id, 
                       const std::string& state, 
                       int person_count = 0);
    
    bool insertHourlyAggregation(const std::string& date_hour, 
                                const std::string& seat_id, 
                                int occupied_minutes);
    
    // Basic Data Insertion
    bool insertSeat(const std::string& seat_id, 
                   int roi_x, int roi_y, 
                   int roi_width, int roi_height);
    
    // Query operation
    int getOccupiedMinutes(const std::string& seat_id, 
                          const std::string& start_time, 
                          const std::string& end_time);
    
    double getOverallOccupancyRate(const std::string& date_hour);
    
    // UI Data Interface
    std::vector<SeatStatus> getCurrentSeatStatus();
    BasicStats getCurrentBasicStats();
    std::vector<HourlyData> getTodayHourlyData();
    std::vector<double> getDailyHourlyOccupancy(const std::string& date);
    
    // Batch operation
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    
    // Utility method
    std::vector<std::string> getAllSeatIds();
    std::string getCurrentTimestamp();

     // Add exec method
    bool exec(const std::string& sql);
    
    // Alarm Entry Interface
    bool insertAlert(
        const std::string& alert_id,
        const std::string& seat_id,
        const std::string& alert_type,
        const std::string& alert_desc,
        const std::string& timestamp,
        bool is_processed = false
    );

     // Get Unprocessed Alerts
    std::vector<AlertData> getUnprocessedAlerts();

    // Mark alert as resolved
    bool markAlertAsProcessed(const std::string& alert_id);
private:
    SeatDatabase(const std::string& db_path);
    ~SeatDatabase() = default;
    
    std::string db_path_;
    std::unique_ptr<SQLite::Database> database_;
    std::mutex db_mutex_;
    
    bool createTables();
    bool createIndexes();
};

#endif // SEAT_DATABASE_H
