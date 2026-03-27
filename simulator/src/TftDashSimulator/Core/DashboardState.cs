using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace TftDashSimulator.Core;

/// <summary>
/// Represents the complete state of the TFT Dash dashboard.
/// Maps to the live data message fields from the serial protocol.
/// Implements INotifyPropertyChanged for thread-safe UI updates.
/// </summary>
public class DashboardState : INotifyPropertyChanged
{
    private readonly object _lock = new object();

    // Backing fields for all properties
    private uint _speed;
    private uint _rpm;
    private int _coolant;
    private float _battery;
    private int _hour;
    private int _minute;
    private int _fuel;
    private int _neutral;
    private int _oil;
    private int _highBeam;
    private int _left;
    private int _right;
    private int _menuState;
    private int _infoMode;
    private float _trip1;
    private float _trip2;
    private float _odo;
    private int _km;
    private float _speedCorrection;
    private int _theme;
    private int _ambient;
    private int _gear;
    private int _mpg;
    private int _range;
    private uint _maxSpeed;
    private int _tripTimeHour;
    private int _tripTimeMin;
    private int _oilPressAvail;
    private int _oilPress;
    private int _oilTemp;
    private int _fahrenheit;
    private int _bar;
    private int _frontSensor;
    private int _rearSensor;
    private int _frontPressureLow;
    private int _rearPressureLow;
    private string _nav = string.Empty;

    // Menu-specific backing fields
    private int _frontSprocket = 15;
    private int _rearSprocket = 45;
    private int _coolantFanTemp = 90;
    private int _controlLayout = 1;
    private int _dayTheme = 0;
    private int _nightTheme = 7;
    private int _currentLightLevel = 128;
    private int _lightSwitchValue = 64;
    private int _fanNeutralOption = 0;
    private int _gearRatioInterval = 100;
    private int _odoError = 0;

    public event PropertyChangedEventHandler? PropertyChanged;
    public event EventHandler? StateChanged;

    // Field 1: Speed (unsigned int)
    public uint Speed
    {
        get { lock (_lock) return _speed; }
        set { SetField(ref _speed, value); }
    }

    // Field 2: RPM (unsigned int)
    public uint Rpm
    {
        get { lock (_lock) return _rpm; }
        set { SetField(ref _rpm, value); }
    }

    // Field 3: Coolant temperature (int)
    public int Coolant
    {
        get { lock (_lock) return _coolant; }
        set { SetField(ref _coolant, value); }
    }

    // Field 4: Battery voltage (float)
    public float Battery
    {
        get { lock (_lock) return _battery; }
        set { SetField(ref _battery, value); }
    }

    // Field 5: Hour (0-23)
    public int Hour
    {
        get { lock (_lock) return _hour; }
        set { SetField(ref _hour, value); }
    }

    // Field 6: Minute (0-59)
    public int Minute
    {
        get { lock (_lock) return _minute; }
        set { SetField(ref _minute, value); }
    }

    // Field 7: Fuel float sensor value (raw integer)
    public int Fuel
    {
        get { lock (_lock) return _fuel; }
        set { SetField(ref _fuel, value); }
    }

    // Field 8: Neutral light (1 = engaged, 0 = off)
    public int Neutral
    {
        get { lock (_lock) return _neutral; }
        set { SetField(ref _neutral, value); }
    }

    // Field 9: Oil level warning (1 = warning, 0 = ok)
    public int Oil
    {
        get { lock (_lock) return _oil; }
        set { SetField(ref _oil, value); }
    }

    // Field 10: High beam indicator (1 = on, 0 = off)
    public int HighBeam
    {
        get { lock (_lock) return _highBeam; }
        set { SetField(ref _highBeam, value); }
    }

    // Field 11: Left indicator (1 = on, 0 = off)
    public int Left
    {
        get { lock (_lock) return _left; }
        set { SetField(ref _left, value); }
    }

    // Field 12: Right indicator (1 = on, 0 = off)
    public int Right
    {
        get { lock (_lock) return _right; }
        set { SetField(ref _right, value); }
    }

    // Field 13: Menu state (menulevel + choicestate; 0 = main dashboard)
    public int MenuState
    {
        get { lock (_lock) return _menuState; }
        set { SetField(ref _menuState, value); }
    }

    // Field 14: Info panel display mode
    public int InfoMode
    {
        get { lock (_lock) return _infoMode; }
        set { SetField(ref _infoMode, value); }
    }

    // Field 15: Trip 1 distance (float)
    public float Trip1
    {
        get { lock (_lock) return _trip1; }
        set { SetField(ref _trip1, value); }
    }

    // Field 16: Trip 2 distance (float)
    public float Trip2
    {
        get { lock (_lock) return _trip2; }
        set { SetField(ref _trip2, value); }
    }

    // Field 17: Odometer reading (float)
    public float Odo
    {
        get { lock (_lock) return _odo; }
        set { SetField(ref _odo, value); }
    }

    // Field 18: Unit mode (1 = kilometres, 0 = miles)
    public int Km
    {
        get { lock (_lock) return _km; }
        set { SetField(ref _km, value); }
    }

    // Field 19: Speed correction factor (float)
    public float SpeedCorrection
    {
        get { lock (_lock) return _speedCorrection; }
        set { SetField(ref _speedCorrection, value); }
    }

    // Field 20: Current display theme ID
    public int Theme
    {
        get { lock (_lock) return _theme; }
        set { SetField(ref _theme, value); }
    }

    // Field 21: Ambient temperature (int)
    public int Ambient
    {
        get { lock (_lock) return _ambient; }
        set { SetField(ref _ambient, value); }
    }

    // Field 22: Current gear (calculated from sprocket ratios)
    public int Gear
    {
        get { lock (_lock) return _gear; }
        set { SetField(ref _gear, value); }
    }

    // Field 23: Miles per gallon
    public int Mpg
    {
        get { lock (_lock) return _mpg; }
        set { SetField(ref _mpg, value); }
    }

    // Field 24: Estimated range remaining
    public int Range
    {
        get { lock (_lock) return _range; }
        set { SetField(ref _range, value); }
    }

    // Field 25: Maximum speed recorded this trip (unsigned int)
    public uint MaxSpeed
    {
        get { lock (_lock) return _maxSpeed; }
        set { SetField(ref _maxSpeed, value); }
    }

    // Field 26: Trip time hours
    public int TripTimeHour
    {
        get { lock (_lock) return _tripTimeHour; }
        set { SetField(ref _tripTimeHour, value); }
    }

    // Field 27: Trip time minutes
    public int TripTimeMin
    {
        get { lock (_lock) return _tripTimeMin; }
        set { SetField(ref _tripTimeMin, value); }
    }

    // Field 28: Oil pressure hardware available (1 = yes, 0 = no)
    public int OilPressAvail
    {
        get { lock (_lock) return _oilPressAvail; }
        set { SetField(ref _oilPressAvail, value); }
    }

    // Field 29: Oil pressure sensor reading (averaged)
    public int OilPress
    {
        get { lock (_lock) return _oilPress; }
        set { SetField(ref _oilPress, value); }
    }

    // Field 30: Oil temperature sensor reading (averaged)
    public int OilTemp
    {
        get { lock (_lock) return _oilTemp; }
        set { SetField(ref _oilTemp, value); }
    }

    // Field 31: Temperature unit (1 = Fahrenheit, 0 = Celsius)
    public int Fahrenheit
    {
        get { lock (_lock) return _fahrenheit; }
        set { SetField(ref _fahrenheit, value); }
    }

    // Field 32: Pressure unit (1 = Bar, 0 = PSI)
    public int Bar
    {
        get { lock (_lock) return _bar; }
        set { SetField(ref _bar, value); }
    }

    // Field 33: TPMS front sensor ID
    public int FrontSensor
    {
        get { lock (_lock) return _frontSensor; }
        set { SetField(ref _frontSensor, value); }
    }

    // Field 34: TPMS rear sensor ID
    public int RearSensor
    {
        get { lock (_lock) return _rearSensor; }
        set { SetField(ref _rearSensor, value); }
    }

    // Field 35: Front tyre low pressure threshold
    public int FrontPressureLow
    {
        get { lock (_lock) return _frontPressureLow; }
        set { SetField(ref _frontPressureLow, value); }
    }

    // Field 36: Rear tyre low pressure threshold
    public int RearPressureLow
    {
        get { lock (_lock) return _rearPressureLow; }
        set { SetField(ref _rearPressureLow, value); }
    }

    // Field 37: Navigation message from phone via BLE
    public string Nav
    {
        get { lock (_lock) return _nav; }
        set { SetField(ref _nav, value ?? string.Empty); }
    }

    // Menu-specific properties
    public int FrontSprocket
    {
        get { lock (_lock) return _frontSprocket; }
        set { SetField(ref _frontSprocket, value); }
    }

    public int RearSprocket
    {
        get { lock (_lock) return _rearSprocket; }
        set { SetField(ref _rearSprocket, value); }
    }

    public int CoolantFanTemp
    {
        get { lock (_lock) return _coolantFanTemp; }
        set { SetField(ref _coolantFanTemp, value); }
    }

    public int ControlLayout
    {
        get { lock (_lock) return _controlLayout; }
        set { SetField(ref _controlLayout, value); }
    }

    public int DayTheme
    {
        get { lock (_lock) return _dayTheme; }
        set { SetField(ref _dayTheme, value); }
    }

    public int NightTheme
    {
        get { lock (_lock) return _nightTheme; }
        set { SetField(ref _nightTheme, value); }
    }

    public int CurrentLightLevel
    {
        get { lock (_lock) return _currentLightLevel; }
        set { SetField(ref _currentLightLevel, value); }
    }

    public int LightSwitchValue
    {
        get { lock (_lock) return _lightSwitchValue; }
        set { SetField(ref _lightSwitchValue, value); }
    }

    public int FanNeutralOption
    {
        get { lock (_lock) return _fanNeutralOption; }
        set { SetField(ref _fanNeutralOption, value); }
    }

    public int GearRatioInterval
    {
        get { lock (_lock) return _gearRatioInterval; }
        set { SetField(ref _gearRatioInterval, value); }
    }

    public int OdoError
    {
        get { lock (_lock) return _odoError; }
        set { SetField(ref _odoError, value); }
    }

    /// <summary>
    /// Creates a new DashboardState with default/idle values.
    /// </summary>
    public DashboardState()
    {
        // Set sensible defaults for idle/parked state
        _speed = 0;
        _rpm = 0;
        _coolant = 20;  // 20°C ambient
        _battery = 12.5f;
        _hour = 12;
        _minute = 0;
        _fuel = 500;  // Mid-range fuel reading
        _neutral = 1;  // In neutral when idle
        _oil = 0;  // No warning
        _highBeam = 0;
        _left = 0;
        _right = 0;
        _menuState = 0;  // Main dashboard
        _infoMode = 0;
        _trip1 = 0.0f;
        _trip2 = 0.0f;
        _odo = 1234.5f;
        _km = 0;  // Default to miles
        _speedCorrection = 1.0f;
        _theme = 0;  // Default theme
        _ambient = 20;
        _gear = 0;  // Neutral
        _mpg = 0;
        _range = 0;
        _maxSpeed = 0;
        _tripTimeHour = 0;
        _tripTimeMin = 0;
        _oilPressAvail = 0;
        _oilPress = 0;
        _oilTemp = 0;
        _fahrenheit = 0;  // Default to Celsius
        _bar = 0;  // Default to PSI
        _frontSensor = 1;
        _rearSensor = 2;
        _frontPressureLow = 30;
        _rearPressureLow = 28;
        _nav = string.Empty;
    }

    /// <summary>
    /// Creates a copy of this DashboardState.
    /// </summary>
    public DashboardState Clone()
    {
        lock (_lock)
        {
            return new DashboardState
            {
                _speed = _speed,
                _rpm = _rpm,
                _coolant = _coolant,
                _battery = _battery,
                _hour = _hour,
                _minute = _minute,
                _fuel = _fuel,
                _neutral = _neutral,
                _oil = _oil,
                _highBeam = _highBeam,
                _left = _left,
                _right = _right,
                _menuState = _menuState,
                _infoMode = _infoMode,
                _trip1 = _trip1,
                _trip2 = _trip2,
                _odo = _odo,
                _km = _km,
                _speedCorrection = _speedCorrection,
                _theme = _theme,
                _ambient = _ambient,
                _gear = _gear,
                _mpg = _mpg,
                _range = _range,
                _maxSpeed = _maxSpeed,
                _tripTimeHour = _tripTimeHour,
                _tripTimeMin = _tripTimeMin,
                _oilPressAvail = _oilPressAvail,
                _oilPress = _oilPress,
                _oilTemp = _oilTemp,
                _fahrenheit = _fahrenheit,
                _bar = _bar,
                _frontSensor = _frontSensor,
                _rearSensor = _rearSensor,
                _frontPressureLow = _frontPressureLow,
                _rearPressureLow = _rearPressureLow,
                _nav = _nav,
                _frontSprocket = _frontSprocket,
                _rearSprocket = _rearSprocket,
                _coolantFanTemp = _coolantFanTemp,
                _controlLayout = _controlLayout,
                _dayTheme = _dayTheme,
                _nightTheme = _nightTheme,
                _currentLightLevel = _currentLightLevel,
                _lightSwitchValue = _lightSwitchValue,
                _fanNeutralOption = _fanNeutralOption,
                _gearRatioInterval = _gearRatioInterval,
                _odoError = _odoError
            };
        }
    }

    /// <summary>
    /// Thread-safe property setter with change notification.
    /// </summary>
    private void SetField<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
    {
        lock (_lock)
        {
            if (EqualityComparer<T>.Default.Equals(field, value))
                return;

            field = value;
        }

        OnPropertyChanged(propertyName);
        OnStateChanged();
    }

    /// <summary>
    /// Raises the PropertyChanged event.
    /// </summary>
    protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }

    /// <summary>
    /// Raises the StateChanged event.
    /// </summary>
    protected virtual void OnStateChanged()
    {
        StateChanged?.Invoke(this, EventArgs.Empty);
    }
}
