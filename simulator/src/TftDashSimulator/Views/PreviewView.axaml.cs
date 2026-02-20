using Avalonia.Controls;
using Avalonia.Threading;
using System;
using System.ComponentModel;
using System.Text;
using TftDashSimulator.Core;

namespace TftDashSimulator.Views;

public partial class PreviewView : UserControl
{
    private int _messageCount;
    private DashboardState? _currentState;

    public PreviewView()
    {
        InitializeComponent();
        DataContextChanged += OnDataContextChanged;
        Loaded += OnLoaded;
    }

    private void OnLoaded(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        if (_currentState != null)
        {
            UpdatePreview();
        }
    }

    private void OnDataContextChanged(object? sender, EventArgs e)
    {
        if (_currentState != null)
        {
            _currentState.PropertyChanged -= OnStateChanged;
        }

        if (DataContext is DashboardState state)
        {
            _currentState = state;
            _currentState.PropertyChanged += OnStateChanged;
            UpdatePreview();
        }
    }

    private void OnStateChanged(object? sender, PropertyChangedEventArgs e)
    {
        Dispatcher.UIThread.Post(UpdatePreview);
    }

    private void UpdatePreview()
    {
        if (_currentState == null) return;

        _messageCount++;

        string message = MessageGenerator.GenerateLiveDataMessage(_currentState);

        var preview = new StringBuilder();
        preview.AppendLine("═══ RAW MESSAGE ═══");
        preview.AppendLine(message);
        preview.AppendLine();
        preview.AppendLine("═══ PARSED FIELDS ═══");
        preview.AppendLine($"Speed:             {_currentState.Speed}");
        preview.AppendLine($"RPM:               {_currentState.Rpm}");
        preview.AppendLine($"Coolant:           {_currentState.Coolant}°C");
        preview.AppendLine($"Battery:           {_currentState.Battery:F1}V");
        preview.AppendLine($"Time:              {_currentState.Hour:D2}:{_currentState.Minute:D2}");
        preview.AppendLine($"Fuel:              {_currentState.Fuel}");
        preview.AppendLine($"Neutral:           {(_currentState.Neutral == 1 ? "ON" : "OFF")}");
        preview.AppendLine($"Oil Warning:       {(_currentState.Oil == 1 ? "ON" : "OFF")}");
        preview.AppendLine($"High Beam:         {(_currentState.HighBeam == 1 ? "ON" : "OFF")}");
        preview.AppendLine($"Left Indicator:    {(_currentState.Left == 1 ? "ON" : "OFF")}");
        preview.AppendLine($"Right Indicator:   {(_currentState.Right == 1 ? "ON" : "OFF")}");
        preview.AppendLine($"Menu State:        {_currentState.MenuState}");
        preview.AppendLine($"Info Mode:         {_currentState.InfoMode}");
        preview.AppendLine($"Trip 1:            {_currentState.Trip1:F1}");
        preview.AppendLine($"Trip 2:            {_currentState.Trip2:F1}");
        preview.AppendLine($"Odometer:          {_currentState.Odo:F1}");
        preview.AppendLine($"Units:             {(_currentState.Km == 1 ? "KM/H" : "MPH")}");
        preview.AppendLine($"Speed Correction:  {_currentState.SpeedCorrection:F1}");
        preview.AppendLine($"Theme:             {_currentState.Theme}");
        preview.AppendLine($"Ambient:           {_currentState.Ambient}°C");
        preview.AppendLine($"Gear:              {_currentState.Gear}");
        preview.AppendLine($"MPG:               {_currentState.Mpg}");
        preview.AppendLine($"Range:             {_currentState.Range}");
        preview.AppendLine($"Max Speed:         {_currentState.MaxSpeed}");
        preview.AppendLine($"Trip Time:         {_currentState.TripTimeHour}h {_currentState.TripTimeMin}m");
        preview.AppendLine($"Oil Press Avail:   {_currentState.OilPressAvail}");
        preview.AppendLine($"Oil Press:         {_currentState.OilPress}");
        preview.AppendLine($"Oil Temp:          {_currentState.OilTemp}");
        preview.AppendLine($"Fahrenheit:        {_currentState.Fahrenheit}");
        preview.AppendLine($"Bar:               {_currentState.Bar}");
        preview.AppendLine($"Front Sensor:      {_currentState.FrontSensor}");
        preview.AppendLine($"Rear Sensor:       {_currentState.RearSensor}");
        preview.AppendLine($"Front Press Low:   {_currentState.FrontPressureLow}");
        preview.AppendLine($"Rear Press Low:    {_currentState.RearPressureLow}");
        preview.AppendLine($"Navigation:        {(_currentState.Nav == string.Empty ? "(empty)" : _currentState.Nav)}");

        if (MessagePreview != null)
            MessagePreview.Text = preview.ToString();

        if (StatisticsText != null)
        {
            var now = DateTime.Now;
            StatisticsText.Text = $"Messages: {_messageCount} | Size: {message.Length} bytes | Last updated: {now:HH:mm:ss}";
        }
    }
}
