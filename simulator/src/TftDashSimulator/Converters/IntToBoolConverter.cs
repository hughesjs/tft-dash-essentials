using Avalonia.Data.Converters;
using System;
using System.Globalization;

namespace TftDashSimulator.Converters;

/// <summary>
/// Converts between int (0/1) and bool for checkbox bindings.
/// DashboardState uses int for indicator flags, but Avalonia CheckBox requires bool.
/// </summary>
public class IntToBoolConverter : IValueConverter
{
    public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        if (value is int intValue)
        {
            return intValue != 0;
        }
        return false;
    }

    public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        if (value is bool boolValue)
        {
            return boolValue ? 1 : 0;
        }
        return 0;
    }
}
