using Avalonia.Controls;
using Avalonia.Input;
using TftDashSimulator.Core;
using TftDashSimulator.ViewModels;

namespace TftDashSimulator.Views;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        KeyDown += OnKeyDown;
    }

    private void OnKeyDown(object? sender, KeyEventArgs e)
    {
        if (DataContext is not MainViewModel vm) return;
        var state = vm.DashboardState;

        switch (e.Key)
        {
            case Key.F1:
                PresetScenarios.ApplyIdle(state);
                e.Handled = true;
                break;
            case Key.F2:
                PresetScenarios.ApplyCruise(state);
                e.Handled = true;
                break;
            case Key.F3:
                PresetScenarios.ApplyWarning(state);
                e.Handled = true;
                break;
            case Key.F4:
                var s = PresetScenarios.LowFuel();
                state.Speed = s.Speed;
                state.Rpm = s.Rpm;
                state.Coolant = s.Coolant;
                state.Battery = s.Battery;
                state.Fuel = s.Fuel;
                state.Neutral = s.Neutral;
                state.Oil = s.Oil;
                state.HighBeam = s.HighBeam;
                state.Left = s.Left;
                state.Right = s.Right;
                state.Gear = s.Gear;
                e.Handled = true;
                break;
            case Key.Escape:
                Close();
                e.Handled = true;
                break;
        }
    }

    protected override void OnClosing(WindowClosingEventArgs e)
    {
        if (DataContext is MainViewModel vm)
        {
            vm.Dispose();
        }
        base.OnClosing(e);
    }
}
