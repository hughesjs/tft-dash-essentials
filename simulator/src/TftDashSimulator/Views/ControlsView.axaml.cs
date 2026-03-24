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

        // Sync nav controls from model
        SyncNavControlsFromModel();

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

    // === Navigation ===

    private void SyncNavControlsFromModel()
    {
        if (_state == null) return;
        // Parse the nav string back into controls if it's set by a preset
        if (string.IsNullOrEmpty(_state.Nav))
        {
            NavSymbolCombo.SelectedIndex = 0;
            return;
        }
        var parts = _state.Nav.Split('%', StringSplitOptions.RemoveEmptyEntries);
        if (parts.Length >= 1)
        {
            for (int i = 1; i < NavSymbolCombo.Items.Count; i++)
            {
                if (NavSymbolCombo.Items[i] is ComboBoxItem item && item.Tag?.ToString() == parts[0])
                {
                    NavSymbolCombo.SelectedIndex = i;
                    break;
                }
            }
        }
        if (parts.Length >= 2) NavRoadText.Text = parts[1];
        if (parts.Length >= 3) NavTowardsText.Text = parts[2];
        if (parts.Length >= 4) NavExitText.Text = parts[3];
        if (parts.Length >= 5 && decimal.TryParse(parts[4], out var yards)) NavYardsControl.Value = yards;
        if (parts.Length >= 6 && decimal.TryParse(parts[5], out var miles)) NavMilesControl.Value = miles;
        if (parts.Length >= 7) NavDrivingLeftCheck.IsChecked = parts[6] == "1";
        if (parts.Length >= 8 && decimal.TryParse(parts[7], out var destMiles)) NavDestMilesControl.Value = destMiles;
    }

    private void BuildNavMessage()
    {
        if (_updatingFromModel || _state == null) return;
        var selectedItem = NavSymbolCombo.SelectedItem as ComboBoxItem;
        var symbol = selectedItem?.Tag?.ToString();
        if (string.IsNullOrEmpty(symbol))
        {
            _state.Nav = "";
            _state.InfoMode = 0;
            return;
        }
        _state.InfoMode = 3; // Show nav overlay
        var road = NavRoadText.Text ?? "";
        var towards = NavTowardsText.Text ?? "";
        var exit = NavExitText.Text ?? "";
        var yards = NavYardsControl.Value?.ToString("0") ?? "0";
        var miles = NavMilesControl.Value?.ToString("0.0") ?? "0.0";
        var drivingLeft = NavDrivingLeftCheck.IsChecked == true ? "1" : "0";
        var destMiles = NavDestMilesControl.Value?.ToString("0.0") ?? "5.0";
        _state.Nav = $"%{symbol}%{road}%{towards}%{exit}%{yards}%{miles}%{drivingLeft}%{destMiles}%";
    }

    private void OnNavSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        BuildNavMessage();
    }

    private void OnNavTextChanged(object? sender, TextChangedEventArgs e)
    {
        BuildNavMessage();
    }

    private void OnNavCheckChanged(object? sender, RoutedEventArgs e)
    {
        BuildNavMessage();
    }

    private void OnNavNumericChanged(object? sender, NumericUpDownValueChangedEventArgs e)
    {
        BuildNavMessage();
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

    private void OnNavigationClick(object? sender, RoutedEventArgs e)
    {
        if (_state != null) PresetScenarios.ApplyNavigation(_state);
    }

    private void OnExitClick(object? sender, RoutedEventArgs e)
    {
        Environment.Exit(0);
    }
}
