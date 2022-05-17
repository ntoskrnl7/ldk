#include "ntdll.h"
#include "../ldk.h"

BOOLEAN
NTAPI
RtlCutoverTimeToSystemTime (
    _In_ PTIME_FIELDS CutoverTime,
    _Out_ PLARGE_INTEGER SystemTime,
    _Out_ PLARGE_INTEGER CurrentSystemTime,
    _In_ BOOLEAN ThisYear
    )
{
    TIME_FIELDS CurrentTimeFields;
    RtlTimeToTimeFields( CurrentSystemTime,
                         &CurrentTimeFields );
    if (CutoverTime->Year) {
        if (! RtlTimeFieldsToTime( CutoverTime,
                                   SystemTime )) {
            return FALSE;
        }
        return SystemTime->QuadPart >= CurrentSystemTime->QuadPart;
    } else {
        TIME_FIELDS WorkingTimeField;
        TIME_FIELDS ScratchTimeField;
        LARGE_INTEGER ScratchTime;
        CSHORT BestWeekdayDate;
        CSHORT WorkingWeekdayNumber;
        CSHORT TargetWeekdayNumber;
        CSHORT TargetYear;
        CSHORT TargetMonth;
        CSHORT TargetWeekday;
        BOOLEAN MonthMatches;
        TargetWeekdayNumber = CutoverTime->Day;
        if (TargetWeekdayNumber > 5 || TargetWeekdayNumber == 0) {
            return FALSE;
        }
        TargetWeekday = CutoverTime->Weekday;
        TargetMonth = CutoverTime->Month;
        MonthMatches = FALSE;
        if (! ThisYear) {
            if ( TargetMonth < CurrentTimeFields.Month ) {
                TargetYear = CurrentTimeFields.Year + 1;
            } else if ( TargetMonth > CurrentTimeFields.Month ) {
                TargetYear = CurrentTimeFields.Year;
            } else {
                TargetYear = CurrentTimeFields.Year;
                MonthMatches = TRUE;
            }
        } else {
            TargetYear = CurrentTimeFields.Year;
        }

try_next_year:
        BestWeekdayDate = 0;

        WorkingTimeField.Year = TargetYear;
        WorkingTimeField.Month = TargetMonth;
        WorkingTimeField.Day = 1;
        WorkingTimeField.Hour = CutoverTime->Hour;
        WorkingTimeField.Minute = CutoverTime->Minute;
        WorkingTimeField.Second = CutoverTime->Second;
        WorkingTimeField.Milliseconds = CutoverTime->Milliseconds;
        WorkingTimeField.Weekday = 0;
        if (! RtlTimeFieldsToTime( &WorkingTimeField,
                                   &ScratchTime )) {
            return FALSE;
        }

        RtlTimeToTimeFields( &ScratchTime,
                             &ScratchTimeField );
        if (ScratchTimeField.Weekday > TargetWeekday) {
            WorkingTimeField.Day += (7 - (ScratchTimeField.Weekday - TargetWeekday));
        } else if (ScratchTimeField.Weekday < TargetWeekday) {
            WorkingTimeField.Day += (TargetWeekday - ScratchTimeField.Weekday);
        }

        BestWeekdayDate = WorkingTimeField.Day;
        WorkingWeekdayNumber = 1;

        while (WorkingWeekdayNumber < TargetWeekdayNumber) {
            WorkingTimeField.Day += 7;
            if (! RtlTimeFieldsToTime( &WorkingTimeField,
                                       &ScratchTime )) {
                break;
            }
            RtlTimeToTimeFields( &ScratchTime,
                                 &ScratchTimeField );
            WorkingWeekdayNumber++;
            BestWeekdayDate = ScratchTimeField.Day;
        }
        WorkingTimeField.Day = BestWeekdayDate;
        if (! RtlTimeFieldsToTime( &WorkingTimeField,
                                   &ScratchTime) ) {
            return FALSE;
        }

        if (MonthMatches) {
            if (WorkingTimeField.Day < CurrentTimeFields.Day) {
                MonthMatches = FALSE;
                TargetYear++;
                goto try_next_year;
            } else if (WorkingTimeField.Day == CurrentTimeFields.Day) {
                if (ScratchTime.QuadPart < CurrentSystemTime->QuadPart) {
                    MonthMatches = FALSE;
                    TargetYear++;
                    goto try_next_year;
                }
            }
        }
        *SystemTime = ScratchTime;
        return TRUE;
    }
}