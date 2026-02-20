using System.Globalization;
using System.Text;

namespace TftDashSimulator.Core;

/// <summary>
/// Generates TFT Dash serial protocol messages from DashboardState.
/// Implements the live data and menu data message formats.
/// </summary>
public static class MessageGenerator
{
    /// <summary>
    /// Generates a live data message from the current dashboard state.
    /// Format: {,speed,rpm,coolant,battery,time,fuel,...,nav,}
    /// </summary>
    public static string GenerateLiveDataMessage(DashboardState state)
    {
        var sb = new StringBuilder();

        // Start marker
        sb.Append('{');
        sb.Append(',');

        // Field 1: Speed (unsigned int)
        sb.Append(state.Speed);
        sb.Append(',');

        // Field 2: RPM (unsigned int)
        sb.Append(state.Rpm);
        sb.Append(',');

        // Field 3: Coolant temperature (int)
        sb.Append(state.Coolant);
        sb.Append(',');

        // Field 4: Battery voltage (float)
        sb.Append(state.Battery.ToString("F1", CultureInfo.InvariantCulture));
        sb.Append(',');

        // Field 5: Current hour (int)
        sb.Append(state.Hour);
        sb.Append(',');

        // Field 6: Current minute (int)
        sb.Append(state.Minute);
        sb.Append(',');

        // Field 7: Fuel float sensor value
        sb.Append(state.Fuel);
        sb.Append(',');

        // Field 8: Neutral light
        sb.Append(state.Neutral);
        sb.Append(',');

        // Field 9: Oil level warning
        sb.Append(state.Oil);
        sb.Append(',');

        // Field 10: High beam indicator
        sb.Append(state.HighBeam);
        sb.Append(',');

        // Field 11: Left indicator
        sb.Append(state.Left);
        sb.Append(',');

        // Field 12: Right indicator
        sb.Append(state.Right);
        sb.Append(',');

        // Field 13: Menu state
        sb.Append(state.MenuState);
        sb.Append(',');

        // Field 14: Info panel display mode
        sb.Append(state.InfoMode);
        sb.Append(',');

        // Field 15: Trip 1 distance (float)
        sb.Append(state.Trip1.ToString("F1", CultureInfo.InvariantCulture));
        sb.Append(',');

        // Field 16: Trip 2 distance (float)
        sb.Append(state.Trip2.ToString("F1", CultureInfo.InvariantCulture));
        sb.Append(',');

        // Field 17: Odometer reading (float)
        sb.Append(state.Odo.ToString("F1", CultureInfo.InvariantCulture));
        sb.Append(',');

        // Field 18: Unit mode (1 = km, 0 = miles)
        sb.Append(state.Km);
        sb.Append(',');

        // Field 19: Speed correction factor (float)
        sb.Append(state.SpeedCorrection.ToString("F1", CultureInfo.InvariantCulture));
        sb.Append(',');

        // Field 20: Current display theme ID
        sb.Append(state.Theme);
        sb.Append(',');

        // Field 21: Ambient temperature
        sb.Append(state.Ambient);
        sb.Append(',');

        // Field 22: Current gear
        sb.Append(state.Gear);
        sb.Append(',');

        // Field 23: Miles per gallon
        sb.Append(state.Mpg);
        sb.Append(',');

        // Field 24: Estimated range remaining
        sb.Append(state.Range);
        sb.Append(',');

        // Field 25: Maximum speed recorded (unsigned int)
        sb.Append(state.MaxSpeed);
        sb.Append(',');

        // Field 26: Trip time hours
        sb.Append(state.TripTimeHour);
        sb.Append(',');

        // Field 27: Trip time minutes
        sb.Append(state.TripTimeMin);
        sb.Append(',');

        // Field 28: Oil pressure hardware available
        sb.Append(state.OilPressAvail);
        sb.Append(',');

        // Field 29: Oil pressure sensor reading
        sb.Append(state.OilPress);
        sb.Append(',');

        // Field 30: Oil temperature sensor reading
        sb.Append(state.OilTemp);
        sb.Append(',');

        // Field 31: Temperature unit (1 = Fahrenheit, 0 = Celsius)
        sb.Append(state.Fahrenheit);
        sb.Append(',');

        // Field 32: Pressure unit (1 = Bar, 0 = PSI)
        sb.Append(state.Bar);
        sb.Append(',');

        // Field 33: TPMS front sensor ID
        sb.Append(state.FrontSensor);
        sb.Append(',');

        // Field 34: TPMS rear sensor ID
        sb.Append(state.RearSensor);
        sb.Append(',');

        // Field 35: Front tyre low pressure threshold
        sb.Append(state.FrontPressureLow);
        sb.Append(',');

        // Field 36: Rear tyre low pressure threshold
        sb.Append(state.RearPressureLow);
        sb.Append(',');

        // Field 37: Navigation message (can be empty)
        sb.Append(state.Nav);
        sb.Append(',');

        // End marker
        sb.Append('}');

        return sb.ToString();
    }

    /// <summary>
    /// Generates a menu data message.
    /// Format: [,menustate,ododigit1,ododigit2,...,]
    /// </summary>
    public static string GenerateMenuDataMessage(
        int menuState,
        int[] odoDigits,
        int[] odo2Digits,
        int odoError,
        int[] setTimeDigits,
        int[] spcDigits,
        int frontSprocket,
        int rearSprocket,
        int coolantFanTemp,
        int km,
        int fh,
        int bar,
        int frontSensor,
        int rearSensor,
        int frontPressureLow,
        int rearPressureLow,
        int controlLayout,
        int dayTheme,
        int nightTheme,
        int currentLightLevel,
        int lightSwitchValue,
        int fanNeutralOption,
        int gearRatioInterval)
    {
        var sb = new StringBuilder();

        // Start marker
        sb.Append('[');
        sb.Append(',');

        // Field 1: Menu state
        sb.Append(menuState);
        sb.Append(',');

        // Fields 2-7: Odometer setup digits 1-6
        for (int i = 0; i < 6; i++)
        {
            sb.Append(odoDigits[i]);
            sb.Append(',');
        }

        // Fields 8-13: Odometer confirmation digits 1-6
        for (int i = 0; i < 6; i++)
        {
            sb.Append(odo2Digits[i]);
            sb.Append(',');
        }

        // Field 14: Odometer error state
        sb.Append(odoError);
        sb.Append(',');

        // Fields 15-18: Set time digits 0-3
        for (int i = 0; i < 4; i++)
        {
            sb.Append(setTimeDigits[i]);
            sb.Append(',');
        }

        // Fields 19-22: Speed correction digits 0-3
        for (int i = 0; i < 4; i++)
        {
            sb.Append(spcDigits[i]);
            sb.Append(',');
        }

        // Field 23: Front sprocket tooth count
        sb.Append(frontSprocket);
        sb.Append(',');

        // Field 24: Rear sprocket tooth count
        sb.Append(rearSprocket);
        sb.Append(',');

        // Field 25: Coolant fan activation temperature
        sb.Append(coolantFanTemp);
        sb.Append(',');

        // Field 26: Using kilometres (1 = km, 0 = miles)
        sb.Append(km);
        sb.Append(',');

        // Field 27: Using Fahrenheit (1 = Fahrenheit, 0 = Celsius)
        sb.Append(fh);
        sb.Append(',');

        // Field 28: Using Bar (1 = Bar, 0 = PSI)
        sb.Append(bar);
        sb.Append(',');

        // Field 29: TPMS front sensor ID
        sb.Append(frontSensor);
        sb.Append(',');

        // Field 30: TPMS rear sensor ID
        sb.Append(rearSensor);
        sb.Append(',');

        // Field 31: Front tyre low pressure threshold
        sb.Append(frontPressureLow);
        sb.Append(',');

        // Field 32: Rear tyre low pressure threshold
        sb.Append(rearPressureLow);
        sb.Append(',');

        // Field 33: Control button layout
        sb.Append(controlLayout);
        sb.Append(',');

        // Field 34: Day theme ID
        sb.Append(dayTheme);
        sb.Append(',');

        // Field 35: Night theme ID
        sb.Append(nightTheme);
        sb.Append(',');

        // Field 36: Current ambient light sensor reading
        sb.Append(currentLightLevel);
        sb.Append(',');

        // Field 37: Light level threshold for day/night switching
        sb.Append(lightSwitchValue);
        sb.Append(',');

        // Field 38: Fan neutral mode option
        sb.Append(fanNeutralOption);
        sb.Append(',');

        // Field 39: Gear ratio update interval setting
        sb.Append(gearRatioInterval);
        sb.Append(',');

        // End marker
        sb.Append(']');

        return sb.ToString();
    }

    /// <summary>
    /// Validates that a live data message is within the expected length range (90-400 characters).
    /// </summary>
    public static bool IsValidLiveDataMessage(string message)
    {
        return message.Length > 90 && message.Length < 400 &&
               message.StartsWith('{') && message.EndsWith('}');
    }

    /// <summary>
    /// Validates that a menu data message is within the expected length range (80-100 characters).
    /// </summary>
    public static bool IsValidMenuDataMessage(string message)
    {
        return message.Length > 78 && message.Length < 150 &&
               message.StartsWith('[') && message.EndsWith(']');
    }
}
