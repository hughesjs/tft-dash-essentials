# TFT Dash Simulator

A cross-platform desktop simulator for the TFT Dash motorcycle dashboard system, built with Avalonia UI.

## Features

- **Modern Desktop UI**: Built with Avalonia 11.2 for native look and feel on Linux, macOS, and Windows
- **Dark Theme**: VS Code Dark+ colour scheme for comfortable viewing
- **Live Preview**: Real-time serial protocol message generation with parsed field display
- **Interactive Controls**: All 36 dashboard data fields editable via NumericUpDown and CheckBox controls
- **Preset Scenarios**: Quick-access buttons for common riding states (Idle, Cruising, High Speed, Low Fuel)
- **Keyboard Shortcuts**: F1-F4 for presets, Esc to exit
- **Pipe Output**: Optional FIFO pipe output at 10 Hz for integration with TFT Dash display software

## Building

```bash
dotnet build
```

## Running

Normal mode (GUI only):
```bash
dotnet run --project src/TftDashSimulator/TftDashSimulator.csproj
```

With pipe output (GUI + /tmp/tft-dash-pipe at 10 Hz):
```bash
dotnet run --project src/TftDashSimulator/TftDashSimulator.csproj -- --pipe
```

## Testing

```bash
dotnet test
```

## Project Structure

- `src/TftDashSimulator/` - Main application
  - `Core/` - Core logic and serial protocol handling (UI-agnostic)
  - `Views/` - Avalonia AXAML views (MainWindow, ControlsView, PreviewView)
  - `ViewModels/` - MVVM ViewModels
  - `Converters/` - Value converters for data binding
  - `Styles/` - Dark theme AXAML styles
- `tests/TftDashSimulator.Tests/` - Unit tests

## Architecture

The simulator uses the MVVM pattern:
- **DashboardState** (Model): Thread-safe state with INotifyPropertyChanged
- **MainViewModel**: Exposes DashboardState and preset commands
- **Views**: Avalonia UserControls with two-way data binding
- **MessageGenerator**: Converts DashboardState to TFT Dash serial protocol format
- **PipeWriter**: Background thread writing to FIFO pipe at 10 Hz (when --pipe flag is used)
