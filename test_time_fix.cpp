// Unit test for time conversion fix
#include <cstdio>
#include <cstdlib>

// Helper function to check if a year is a leap year
bool isLeapYear(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

// Helper function to get days in a specific month
void getDaysInMonth(int year, int daysInMonth[13]) {
    // Initialize with standard days per month (index 0 unused, 1-12 are months)
    int standardDays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    for (int i = 0; i <= 12; i++) {
        daysInMonth[i] = standardDays[i];
    }
    
    // Adjust February for leap year
    if (isLeapYear(year)) {
        daysInMonth[2] = 29;
    }
}

// Convert Date and Time to absolute minutes from base date (01/01/2024 00:00)
long long getMinutes(int day, int month, int year, int hour, int minute) {
    long long totalMinutes = 0;
    
    // Add years (from 2024 base), accounting for leap years
    for (int y = 2024; y < year; y++) {
        totalMinutes += (isLeapYear(y) ? 366 : 365) * 1440;
    }
    
    // Add months (using actual days per month for accuracy)
    int daysInMonth[13];
    getDaysInMonth(year, daysInMonth);
    
    for (int m = 1; m < month; m++) {
        totalMinutes += daysInMonth[m] * 1440;
    }
    
    // Add days (subtract 1 because day 1 is the first day of the month)
    totalMinutes += (long long)(day - 1) * 1440;
    
    // Add hours and minutes
    totalMinutes += hour * 60;
    totalMinutes += minute;
    
    return totalMinutes;
}

// Convert absolute minutes back to Date/Time (FIXED VERSION)
void convertMinutesToDate(long long simTimeMinutes, int& day, int& month, int& year, int& hour, int& minute) {
    long long remaining = simTimeMinutes;
    
    // Extract year (accounting for leap years)
    year = 2024;
    while (remaining > 0) {
        long long yearMinutes = (isLeapYear(year) ? 366 : 365) * 1440;
        if (remaining >= yearMinutes) {
            remaining -= yearMinutes;
            year++;
        } else {
            break;
        }
    }
    
    // Extract month (using actual days per month)
    int daysInMonth[13];
    getDaysInMonth(year, daysInMonth);
    
    month = 1;
    while (month <= 12) {
        long long monthMinutes = daysInMonth[month] * 1440;
        if (remaining >= monthMinutes) {
            remaining -= monthMinutes;
            month++;
        } else {
            break;
        }
    }
    // If month exceeds 12, it indicates a bug in the conversion logic
    if (month > 12) {
        printf("ERROR: Month exceeded 12 in time conversion. This is a bug!\n");
        month = 12;
    }
    
    // Extract day (add 1 because day 1 is the first day)
    day = (int)(remaining / 1440) + 1;
    remaining = remaining % 1440;
    
    // Extract hour and minute
    hour = (int)(remaining / 60);
    minute = (int)(remaining % 60);
    
    // Safety checks
    if (month < 1) month = 1;
    if (day < 1) day = 1;
}

int testCount = 0;
int passCount = 0;

void testRoundTrip(int inputDay, int inputMonth, int inputYear, int inputHour, int inputMinute) {
    testCount++;
    
    // Convert to minutes
    long long minutes = getMinutes(inputDay, inputMonth, inputYear, inputHour, inputMinute);
    
    // Convert back
    int day, month, year, hour, minute;
    convertMinutesToDate(minutes, day, month, year, hour, minute);
    
    // Verify
    bool pass = (day == inputDay && month == inputMonth && year == inputYear && 
                 hour == inputHour && minute == inputMinute);
    
    if (pass) {
        passCount++;
        printf("✓ Test %d PASSED: %02d/%02d/%04d %02d:%02d -> %lld -> %02d/%02d/%04d %02d:%02d\n",
               testCount, inputDay, inputMonth, inputYear, inputHour, inputMinute, 
               minutes, day, month, year, hour, minute);
    } else {
        printf("✗ Test %d FAILED: %02d/%02d/%04d %02d:%02d -> %lld -> %02d/%02d/%04d %02d:%02d\n",
               testCount, inputDay, inputMonth, inputYear, inputHour, inputMinute, 
               minutes, day, month, year, hour, minute);
    }
}

int main() {
    printf("=== Time Conversion Round-Trip Tests ===\n\n");
    
    // Test cases from the bug report
    printf("Bug Report Test Cases:\n");
    testRoundTrip(20, 12, 2024, 10, 0);  // Karachi → Shanghai departure
    testRoundTrip(20, 12, 2024, 6, 0);   // Karachi → Istanbul departure
    testRoundTrip(20, 12, 2024, 8, 0);   // Karachi → Mumbai departure
    
    printf("\nEdge Cases:\n");
    testRoundTrip(1, 1, 2024, 0, 0);     // Base date
    testRoundTrip(31, 12, 2024, 23, 59); // End of 2024
    testRoundTrip(1, 1, 2025, 0, 0);     // Start of 2025
    testRoundTrip(29, 2, 2024, 12, 30);  // Leap day 2024
    
    printf("\nMonth Boundaries:\n");
    testRoundTrip(31, 1, 2024, 15, 45);  // End of January
    testRoundTrip(1, 2, 2024, 8, 20);    // Start of February
    testRoundTrip(30, 4, 2024, 18, 10);  // 30-day month
    testRoundTrip(31, 5, 2024, 22, 55);  // 31-day month
    
    printf("\nRandom Mid-Year Dates:\n");
    testRoundTrip(15, 6, 2024, 9, 30);   // Mid-June
    testRoundTrip(4, 7, 2024, 14, 15);   // Early July
    testRoundTrip(25, 9, 2024, 20, 45);  // Late September
    
    printf("\n=== Test Summary ===\n");
    printf("Total Tests: %d\n", testCount);
    printf("Passed: %d\n", passCount);
    printf("Failed: %d\n", testCount - passCount);
    
    if (passCount == testCount) {
        printf("\n✓ ALL TESTS PASSED!\n");
        return 0;
    } else {
        printf("\n✗ SOME TESTS FAILED!\n");
        return 1;
    }
}
