using Avalonia.Controls;
using Avalonia.Interactivity;
using System.ComponentModel;
using TftDashSimulator.ViewModels;
using TftDashSimulator.Core;

namespace TftDashSimulator.Views;

public partial class ControlsView : UserControl
{
    private DashboardState? _state;
    private bool _updatingFromModel;

    public ControlsView()
    {
        InitializeComponent();
        Loaded += OnLoaded;
    }

    private void OnLoaded(object? sender, RoutedEventArgs e)
    {
        if (DataContext is MainViewModel vm)
        {
            _state = vm.DashboardState;
            _state.PropertyChanged += OnModelChanged;
            SyncControlsFromModel();
        }
    }

    // === Model -> Controls ===

    private void OnModelChanged(object? sender, PropertyChangedEventArgs e)
    {
        Avalonia.Threading.Dispatcher.UIThread.Post(SyncControlsFromModel);
    }

    private void SyncControlsFromModel()
    {
        if (_state == null) return;
        _updatingFromModel = true;

        SpeedControl.Value = _state.Speed;
        RpmControl.Value = _state.Rpm;
        CoolantControl.Value = _state.Coolant;
        BatteryControl.Value = (decimal)_state.Battery;
        FuelControl.Value = _state.Fuel;
        AmbientControl.Value = _state.Ambient;
        HourControl.Value = _state.Hour;
        MinuteControl.Value = _state.Minute;

        NeutralCheck.IsChecked = _state.Neutral != 0;
        OilCheck.IsChecked = _state.Oil != 0;
        HighBeamCheck.IsChecked = _state.HighBeam != 0;
        LeftCheck.IsChecked = _state.Left != 0;
        RightCheck.IsChecked = _state.Right != 0;

        ThemeControl.Value = _state.Theme;
        UnitsControl.Value = _state.Km;

        _updatingFromModel = false;
    }

    // === Controls -> Model ===

    private void OnNumericChanged(object? sender, NumericUpDownValueChangedEventArgs e)
    {
        if (_updatingFromModel || _state == null || !e.NewValue.HasValue) return;

        if (sender == SpeedControl) _state.Speed = (uint)e.NewValue.Value;
        else if (sender == RpmControl) _state.Rpm = (uint)e.NewValue.Value;
        else if (sender == CoolantControl) _state.Coolant = (int)e.NewValue.Value;
        else if (sender == BatteryControl) _state.Battery = (float)e.NewValue.Value;
        else if (sender == FuelControl) _state.Fuel = (int)e.NewValue.Value;
        else if (sender == AmbientControl) _state.Ambient = (int)e.NewValue.Value;
        else if (sender == HourControl) _state.Hour = (int)e.NewValue.Value;
        else if (sender == MinuteControl) _state.Minute = (int)e.NewValue.Value;
        else if (sender == ThemeControl) _state.Theme = (int)e.NewValue.Value;
        else if (sender == UnitsControl) _state.Km = (int)e.NewValue.Value;
    }

    private void OnCheckChanged(object? sender, RoutedEventArgs e)
    {
        if (_updatingFromModel || _state == null) return;
        var cb = (CheckBox)sender!;
        int val = cb.IsChecked == true ? 1 : 0;

        if (sender == NeutralCheck) _state.Neutral = val;
        else if (sender == OilCheck) _state.Oil = val;
        else if (sender == HighBeamCheck) _state.HighBeam = val;
        else if (sender == LeftCheck) _state.Left = val;
        else if (sender == RightCheck) _state.Right = val;
    }

    // === Preset Buttons ===

    private void OnIdleClick(object? sender, RoutedEventArgs e)
    {
        if (_state != null) PresetScenarios.ApplyIdle(_state);
    }

    private void OnCruiseClick(object? sender, RoutedEventArgs e)
    {
        if (_state != null) PresetScenarios.ApplyCruise(_state);
    }

    private void OnWarningClick(object? sender, RoutedEventArgs e)
    {
        if (_state != null) PresetScenarios.ApplyWarning(_state);
    }

    private void OnLowFuelClick(object? sender, RoutedEventArgs e)
    {
        if (_state == null) return;
        var s = PresetScenarios.LowFuel();
        _state.Speed = s.Speed;
        _state.Rpm = s.Rpm;
        _state.Coolant = s.Coolant;
        _state.Battery = s.Battery;
        _state.Fuel = s.Fuel;
        _state.Neutral = s.Neutral;
        _state.Oil = s.Oil;
        _state.HighBeam = s.HighBeam;
        _state.Left = s.Left;
        _state.Right = s.Right;
        _state.Gear = s.Gear;
    }

    private void OnExitClick(object? sender, RoutedEventArgs e)
    {
        Environment.Exit(0);
    }
}
