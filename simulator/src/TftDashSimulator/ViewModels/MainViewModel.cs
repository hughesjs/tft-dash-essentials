using System;
using System.ComponentModel;
using System.Windows.Input;
using TftDashSimulator.Core;

namespace TftDashSimulator.ViewModels;

public class MainViewModel : INotifyPropertyChanged, IDisposable
{
    private readonly PipeWriter? _pipeWriter;

    public DashboardState DashboardState { get; }
    public PipeWriter? PipeWriter => _pipeWriter;

    public ICommand ApplyIdlePresetCommand { get; }
    public ICommand ApplyCruisingPresetCommand { get; }
    public ICommand ApplyHighSpeedPresetCommand { get; }
    public ICommand ApplyLowFuelPresetCommand { get; }
    public ICommand ExitCommand { get; }

    public event PropertyChangedEventHandler? PropertyChanged;

    public MainViewModel()
    {
        DashboardState = new DashboardState();

        // Check if --pipe argument was passed
        var args = Environment.GetCommandLineArgs();
        bool usePipe = Array.Exists(args, arg => arg == "--pipe");

        if (usePipe)
        {
            _pipeWriter = new PipeWriter();
            _pipeWriter.Start(DashboardState);

            // Update pipe writer whenever state changes
            DashboardState.StateChanged += (sender, e) =>
            {
                if (_pipeWriter is { IsRunning: true })
                {
                    _pipeWriter.UpdateState(DashboardState);
                }
            };
        }

        // Initialize commands
        ApplyIdlePresetCommand = new RelayCommand(ApplyIdlePreset);
        ApplyCruisingPresetCommand = new RelayCommand(ApplyCruisingPreset);
        ApplyHighSpeedPresetCommand = new RelayCommand(ApplyHighSpeedPreset);
        ApplyLowFuelPresetCommand = new RelayCommand(ApplyLowFuelPreset);
        ExitCommand = new RelayCommand(Exit);
    }

    private void ApplyIdlePreset()
    {
        PresetScenarios.ApplyIdle(DashboardState);
    }

    private void ApplyCruisingPreset()
    {
        PresetScenarios.ApplyCruise(DashboardState);
    }

    private void ApplyHighSpeedPreset()
    {
        PresetScenarios.ApplyWarning(DashboardState);
    }

    private void ApplyLowFuelPreset()
    {
        var s = PresetScenarios.LowFuel();
        DashboardState.Speed = s.Speed;
        DashboardState.Rpm = s.Rpm;
        DashboardState.Coolant = s.Coolant;
        DashboardState.Battery = s.Battery;
        DashboardState.Fuel = s.Fuel;
        DashboardState.Neutral = s.Neutral;
        DashboardState.Oil = s.Oil;
        DashboardState.HighBeam = s.HighBeam;
        DashboardState.Left = s.Left;
        DashboardState.Right = s.Right;
        DashboardState.Gear = s.Gear;
    }

    private void Exit()
    {
        Environment.Exit(0);
    }

    public void Dispose()
    {
        _pipeWriter?.Dispose();
    }
}

/// <summary>
/// Simple ICommand implementation for button bindings
/// </summary>
public class RelayCommand : ICommand
{
    private readonly Action _execute;
    private readonly Func<bool>? _canExecute;

    public event EventHandler? CanExecuteChanged;

    public RelayCommand(Action execute, Func<bool>? canExecute = null)
    {
        _execute = execute ?? throw new ArgumentNullException(nameof(execute));
        _canExecute = canExecute;
    }

    public bool CanExecute(object? parameter) => _canExecute?.Invoke() ?? true;

    public void Execute(object? parameter) => _execute();

    public void RaiseCanExecuteChanged()
    {
        CanExecuteChanged?.Invoke(this, EventArgs.Empty);
    }
}
