namespace TftDashSimulator.Core;

/// <summary>
/// Provides preset dashboard scenarios for testing different riding conditions.
/// Each scenario represents a realistic state of the motorcycle dashboard.
/// </summary>
public static class PresetScenarios
{
    /// <summary>
    /// Applies idle/parked state to the dashboard: speed=0, rpm=1000, neutral=true, all lights off.
    /// </summary>
    public static void ApplyIdle(DashboardState state)
    {
        state.Speed = 0;
        state.Rpm = 1000;
        state.Coolant = 20;
        state.Battery = 12.5f;
        state.Neutral = 1;
        state.Oil = 0;
        state.HighBeam = 0;
        state.Left = 0;
        state.Right = 0;
        state.Gear = 0;
    }

    /// <summary>
    /// Applies cruise state to the dashboard: speed=65, rpm=5000, neutral=false, gear=5.
    /// </summary>
    public static void ApplyCruise(DashboardState state)
    {
        state.Speed = 65;
        state.Rpm = 5000;
        state.Coolant = 88;
        state.Battery = 14.1f;
        state.Neutral = 0;
        state.Oil = 0;
        state.HighBeam = 0;
        state.Left = 0;
        state.Right = 0;
        state.Gear = 5;
    }

    /// <summary>
    /// Applies warning state to the dashboard: speed=120, rpm=9500, coolant=110, oil=true, battery=11.2.
    /// </summary>
    public static void ApplyWarning(DashboardState state)
    {
        state.Speed = 120;
        state.Rpm = 9500;
        state.Coolant = 110;
        state.Battery = 11.2f;
        state.Neutral = 0;
        state.Oil = 1;
        state.HighBeam = 0;
        state.Left = 0;
        state.Right = 0;
        state.Gear = 6;
    }

    /// <summary>
    /// Applies navigation state with sample navigation message.
    /// Format: %TNR%A82%Edinburgh%3%350%0.2%1%12.5%
    /// </summary>
    public static void ApplyNavigation(DashboardState state)
    {
        state.Speed = 45;
        state.Rpm = 3500;
        state.Coolant = 85;
        state.Battery = 13.8f;
        state.Neutral = 0;
        state.Oil = 0;
        state.HighBeam = 0;
        state.Left = 0;
        state.Right = 0;
        state.Gear = 4;
        state.InfoMode = 3;
        state.Nav = "%TNR%A82%Edinburgh%3%350%0.2%1%12.5%";
    }

    // === Menu Presets ===

    public static void ApplyMainMenu(DashboardState state) { state.MenuState = 1; }

    public static void ApplyThemeMenu(DashboardState state) { state.MenuState = 200; }

    public static void ApplyCoolantFanMenu(DashboardState state) { state.MenuState = 500; state.CoolantFanTemp = 90; state.FanNeutralOption = 0; }

    public static void ApplySprocketMenu(DashboardState state) { state.MenuState = 400; state.FrontSprocket = 15; state.RearSprocket = 45; state.GearRatioInterval = 100; }

    public static void ApplySetTimeMenu(DashboardState state) { state.MenuState = 20; }

    public static void ApplyOdometerMenu(DashboardState state) { state.MenuState = 300; }

    public static void ApplyUnitsMenu(DashboardState state) { state.MenuState = 600; }

    public static void ApplySpeedCorrectionMenu(DashboardState state) { state.MenuState = 100; }

    public static void ApplyTPMSMenu(DashboardState state) { state.MenuState = 700; }

    public static void ApplyControlMenu(DashboardState state) { state.MenuState = 800; state.ControlLayout = 1; }

    public static void ApplyLightMenu(DashboardState state) { state.MenuState = 900; state.DayTheme = 0; state.NightTheme = 7; state.LightSwitchValue = 64; state.CurrentLightLevel = 128; }

    public static void ApplyBackToDash(DashboardState state) { state.MenuState = 0; }

    /// <summary>
    /// Idle/parked state - engine off, neutral, no warnings.
    /// </summary>
    public static DashboardState Idle()
    {
        return new DashboardState
        {
            Speed = 0,
            Rpm = 0,
            Coolant = 20,
            Battery = 12.5f,
            Hour = 12,
            Minute = 0,
            Fuel = 500,
            Neutral = 1,
            Oil = 0,
            HighBeam = 0,
            Left = 0,
            Right = 0,
            MenuState = 0,
            InfoMode = 0,
            Trip1 = 0.0f,
            Trip2 = 0.0f,
            Odo = 1234.5f,
            Km = 0,
            SpeedCorrection = 1.0f,
            Theme = 0,
            Ambient = 20,
            Gear = 0,
            Mpg = 0,
            Range = 0,
            MaxSpeed = 0,
            TripTimeHour = 0,
            TripTimeMin = 0,
            OilPressAvail = 0,
            OilPress = 0,
            OilTemp = 0,
            Fahrenheit = 0,
            Bar = 0,
            FrontSensor = 1,
            RearSensor = 2,
            FrontPressureLow = 30,
            RearPressureLow = 28,
            Nav = string.Empty
        };
    }

    /// <summary>
    /// City cruising - moderate speed, low RPM, normal conditions.
    /// </summary>
    public static DashboardState CityCruise()
    {
        return new DashboardState
        {
            Speed = 30,
            Rpm = 3000,
            Coolant = 85,
            Battery = 13.8f,
            Hour = 14,
            Minute = 35,
            Fuel = 450,
            Neutral = 0,
            Oil = 0,
            HighBeam = 0,
            Left = 0,
            Right = 0,
            MenuState = 0,
            InfoMode = 0,
            Trip1 = 12.5f,
            Trip2 = 24.8f,
            Odo = 1247.0f,
            Km = 0,
            SpeedCorrection = 1.0f,
            Theme = 0,
            Ambient = 22,
            Gear = 3,
            Mpg = 45,
            Range = 120,
            MaxSpeed = 35,
            TripTimeHour = 1,
            TripTimeMin = 35,
            OilPressAvail = 0,
            OilPress = 0,
            OilTemp = 0,
            Fahrenheit = 0,
            Bar = 0,
            FrontSensor = 1,
            RearSensor = 2,
            FrontPressureLow = 30,
            RearPressureLow = 28,
            Nav = string.Empty
        };
    }

    /// <summary>
    /// Motorway riding - high speed, high RPM, optimal conditions.
    /// </summary>
    public static DashboardState MotorwayRide()
    {
        return new DashboardState
        {
            Speed = 70,
            Rpm = 5500,
            Coolant = 90,
            Battery = 14.2f,
            Hour = 16,
            Minute = 45,
            Fuel = 300,
            Neutral = 0,
            Oil = 0,
            HighBeam = 0,
            Left = 0,
            Right = 0,
            MenuState = 0,
            InfoMode = 0,
            Trip1 = 45.3f,
            Trip2 = 89.7f,
            Odo = 1278.8f,
            Km = 0,
            SpeedCorrection = 1.0f,
            Theme = 0,
            Ambient = 18,
            Gear = 6,
            Mpg = 52,
            Range = 85,
            MaxSpeed = 78,
            TripTimeHour = 2,
            TripTimeMin = 15,
            OilPressAvail = 0,
            OilPress = 0,
            OilTemp = 0,
            Fahrenheit = 0,
            Bar = 0,
            FrontSensor = 1,
            RearSensor = 2,
            FrontPressureLow = 30,
            RearPressureLow = 28,
            Nav = string.Empty
        };
    }

    /// <summary>
    /// Spirited riding - very high speed and RPM, indicators active.
    /// </summary>
    public static DashboardState SpiritedRide()
    {
        return new DashboardState
        {
            Speed = 95,
            Rpm = 8500,
            Coolant = 95,
            Battery = 14.0f,
            Hour = 10,
            Minute = 20,
            Fuel = 200,
            Neutral = 0,
            Oil = 0,
            HighBeam = 0,
            Left = 1,
            Right = 0,
            MenuState = 0,
            InfoMode = 0,
            Trip1 = 78.2f,
            Trip2 = 156.4f,
            Odo = 1390.6f,
            Km = 0,
            SpeedCorrection = 1.0f,
            Theme = 2,  // Red theme
            Ambient = 15,
            Gear = 5,
            Mpg = 38,
            Range = 45,
            MaxSpeed = 105,
            TripTimeHour = 3,
            TripTimeMin = 42,
            OilPressAvail = 0,
            OilPress = 0,
            OilTemp = 0,
            Fahrenheit = 0,
            Bar = 0,
            FrontSensor = 1,
            RearSensor = 2,
            FrontPressureLow = 30,
            RearPressureLow = 28,
            Nav = string.Empty
        };
    }

    /// <summary>
    /// Night riding - high beam on, night theme enabled.
    /// </summary>
    public static DashboardState NightRide()
    {
        return new DashboardState
        {
            Speed = 55,
            Rpm = 4200,
            Coolant = 88,
            Battery = 13.9f,
            Hour = 22,
            Minute = 30,
            Fuel = 380,
            Neutral = 0,
            Oil = 0,
            HighBeam = 1,
            Left = 0,
            Right = 0,
            MenuState = 0,
            InfoMode = 0,
            Trip1 = 34.6f,
            Trip2 = 68.9f,
            Odo = 1324.5f,
            Km = 0,
            SpeedCorrection = 1.0f,
            Theme = 7,  // Night theme
            Ambient = 12,
            Gear = 4,
            Mpg = 48,
            Range = 95,
            MaxSpeed = 65,
            TripTimeHour = 1,
            TripTimeMin = 50,
            OilPressAvail = 0,
            OilPress = 0,
            OilTemp = 0,
            Fahrenheit = 0,
            Bar = 0,
            FrontSensor = 1,
            RearSensor = 2,
            FrontPressureLow = 30,
            RearPressureLow = 28,
            Nav = string.Empty
        };
    }

    /// <summary>
    /// Low fuel warning - fuel level critically low, range limited.
    /// </summary>
    public static DashboardState LowFuel()
    {
        return new DashboardState
        {
            Speed = 45,
            Rpm = 3500,
            Coolant = 87,
            Battery = 13.7f,
            Hour = 15,
            Minute = 15,
            Fuel = 50,  // Very low fuel reading
            Neutral = 0,
            Oil = 0,
            HighBeam = 0,
            Left = 0,
            Right = 0,
            MenuState = 0,
            InfoMode = 0,
            Trip1 = 89.4f,
            Trip2 = 178.8f,
            Odo = 1423.9f,
            Km = 0,
            SpeedCorrection = 1.0f,
            Theme = 4,  // Orange theme
            Ambient = 20,
            Gear = 4,
            Mpg = 42,
            Range = 15,  // Low range
            MaxSpeed = 55,
            TripTimeHour = 4,
            TripTimeMin = 25,
            OilPressAvail = 0,
            OilPress = 0,
            OilTemp = 0,
            Fahrenheit = 0,
            Bar = 0,
            FrontSensor = 1,
            RearSensor = 2,
            FrontPressureLow = 30,
            RearPressureLow = 28,
            Nav = string.Empty
        };
    }

    /// <summary>
    /// Oil warning - oil pressure warning light active.
    /// </summary>
    public static DashboardState OilWarning()
    {
        return new DashboardState
        {
            Speed = 25,
            Rpm = 2000,
            Coolant = 92,
            Battery = 13.5f,
            Hour = 9,
            Minute = 45,
            Fuel = 320,
            Neutral = 0,
            Oil = 1,  // Oil warning active
            HighBeam = 0,
            Left = 0,
            Right = 0,
            MenuState = 0,
            InfoMode = 0,
            Trip1 = 15.7f,
            Trip2 = 31.4f,
            Odo = 1265.2f,
            Km = 0,
            SpeedCorrection = 1.0f,
            Theme = 2,  // Red theme for warning visibility
            Ambient = 24,
            Gear = 2,
            Mpg = 35,
            Range = 72,
            MaxSpeed = 30,
            TripTimeHour = 0,
            TripTimeMin = 45,
            OilPressAvail = 0,
            OilPress = 0,
            OilTemp = 0,
            Fahrenheit = 0,
            Bar = 0,
            FrontSensor = 1,
            RearSensor = 2,
            FrontPressureLow = 30,
            RearPressureLow = 28,
            Nav = string.Empty
        };
    }

    /// <summary>
    /// Navigation active - with turn-by-turn directions.
    /// </summary>
    public static DashboardState WithNavigation()
    {
        return new DashboardState
        {
            Speed = 40,
            Rpm = 3200,
            Coolant = 86,
            Battery = 13.9f,
            Hour = 11,
            Minute = 15,
            Fuel = 420,
            Neutral = 0,
            Oil = 0,
            HighBeam = 0,
            Left = 0,
            Right = 0,
            MenuState = 0,
            InfoMode = 0,
            Trip1 = 23.4f,
            Trip2 = 46.8f,
            Odo = 1257.9f,
            Km = 0,
            SpeedCorrection = 1.0f,
            Theme = 3,  // Blue theme
            Ambient = 19,
            Gear = 3,
            Mpg = 47,
            Range = 105,
            MaxSpeed = 48,
            TripTimeHour = 1,
            TripTimeMin = 20,
            OilPressAvail = 0,
            OilPress = 0,
            OilTemp = 0,
            Fahrenheit = 0,
            Bar = 0,
            FrontSensor = 1,
            RearSensor = 2,
            FrontPressureLow = 30,
            RearPressureLow = 28,
            Nav = "%TNR%High Street%City Centre%A1%500%0.3%1%2.5%"
        };
    }

    /// <summary>
    /// Metric units - kilometres, Celsius, Bar.
    /// </summary>
    public static DashboardState MetricUnits()
    {
        return new DashboardState
        {
            Speed = 60,
            Rpm = 4500,
            Coolant = 88,
            Battery = 14.1f,
            Hour = 13,
            Minute = 20,
            Fuel = 400,
            Neutral = 0,
            Oil = 0,
            HighBeam = 0,
            Left = 0,
            Right = 0,
            MenuState = 0,
            InfoMode = 0,
            Trip1 = 45.6f,
            Trip2 = 91.2f,
            Odo = 2045.8f,
            Km = 1,  // Using kilometres
            SpeedCorrection = 1.0f,
            Theme = 1,  // Green theme
            Ambient = 21,
            Gear = 4,
            Mpg = 25,  // L/100km equivalent
            Range = 135,
            MaxSpeed = 75,
            TripTimeHour = 2,
            TripTimeMin = 5,
            OilPressAvail = 0,
            OilPress = 0,
            OilTemp = 0,
            Fahrenheit = 0,  // Celsius
            Bar = 1,  // Using Bar
            FrontSensor = 1,
            RearSensor = 2,
            FrontPressureLow = 2,  // Bar units
            RearPressureLow = 2,
            Nav = string.Empty
        };
    }

    /// <summary>
    /// Gets a scenario by name (case-insensitive).
    /// </summary>
    public static DashboardState GetScenario(string name)
    {
        return name.ToLowerInvariant() switch
        {
            "idle" => Idle(),
            "city" or "citycruise" => CityCruise(),
            "motorway" or "motorwayride" => MotorwayRide(),
            "spirited" or "spiritedride" => SpiritedRide(),
            "night" or "nightride" => NightRide(),
            "lowfuel" => LowFuel(),
            "oilwarning" => OilWarning(),
            "navigation" or "nav" => WithNavigation(),
            "metric" or "metricunits" => MetricUnits(),
            _ => throw new ArgumentException($"Unknown scenario: {name}")
        };
    }

    /// <summary>
    /// Gets a list of all available scenario names.
    /// </summary>
    public static string[] GetScenarioNames()
    {
        return new[]
        {
            "Idle",
            "CityCruise",
            "MotorwayRide",
            "SpiritedRide",
            "NightRide",
            "LowFuel",
            "OilWarning",
            "WithNavigation",
            "MetricUnits"
        };
    }
}
