using System.Diagnostics;
using System.Text;

namespace TftDashSimulator.Core;

/// <summary>
/// Writes serial protocol messages to a Unix FIFO pipe for consumption by the TFT Dash display.
/// Simulates the USB serial connection between the firmware and the Raspberry Pi.
/// Creates a FIFO at /tmp/tft-dash-pipe and writes at 10 Hz (100ms intervals).
/// </summary>
public class PipeWriter : IDisposable
{
    private readonly string _pipePath;
    private FileStream? _pipeStream;
    private StreamWriter? _writer;
    private Thread? _writeThread;
    private CancellationTokenSource? _cancellationTokenSource;
    private DashboardState? _currentState;
    private bool _isRunning;
    private bool _paused;
    private readonly object _lock = new object();

    public bool IsRunning
    {
        get
        {
            lock (_lock)
            {
                return _isRunning;
            }
        }
    }

    public bool Paused
    {
        get { lock (_lock) { return _paused; } }
        set { lock (_lock) { _paused = value; } }
    }

    /// <summary>
    /// Creates a new PipeWriter with the specified pipe path.
    /// Default path is /tmp/tft-dash-pipe for Linux compatibility.
    /// </summary>
    public PipeWriter(string pipePath = "/tmp/tft-dash-pipe")
    {
        _pipePath = pipePath;
        _isRunning = false;
    }

    /// <summary>
    /// Creates the FIFO pipe using mkfifo command.
    /// Removes existing pipe if present.
    /// </summary>
    private void CreateFifo()
    {
        if (File.Exists(_pipePath))
        {
            File.Delete(_pipePath);
        }

        var process = new Process
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = "mkfifo",
                Arguments = _pipePath,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            }
        };

        Console.WriteLine($"Creating FIFO pipe: {_pipePath}");
        process.Start();
        process.WaitForExit();

        if (process.ExitCode != 0)
        {
            var error = process.StandardError.ReadToEnd();
            throw new IOException($"Failed to create FIFO pipe: {error}");
        }
    }

    /// <summary>
    /// Opens the FIFO pipe for writing. Blocks until a reader connects.
    /// </summary>
    private void OpenPipe()
    {
        Console.WriteLine("Waiting for TFT Dash display to connect...");

        _pipeStream = new FileStream(_pipePath, FileMode.Open, FileAccess.Write);
        _writer = new StreamWriter(_pipeStream, Encoding.ASCII)
        {
            AutoFlush = true
        };

        Console.WriteLine("TFT Dash display connected.");
    }

    /// <summary>
    /// Starts the background writer thread that continuously outputs messages at 10 Hz.
    /// </summary>
    public void Start(DashboardState initialState)
    {
        lock (_lock)
        {
            if (_isRunning)
            {
                throw new InvalidOperationException("PipeWriter is already running.");
            }

            _currentState = initialState ?? throw new ArgumentNullException(nameof(initialState));
            _cancellationTokenSource = new CancellationTokenSource();
            _isRunning = true;
        }

        CreateFifo();

        _writeThread = new Thread(WriteLoop)
        {
            Name = "PipeWriter Thread",
            IsBackground = true
        };
        _writeThread.Start();
    }

    /// <summary>
    /// Updates the current dashboard state that will be written to the pipe.
    /// Thread-safe - can be called from any thread.
    /// </summary>
    public void UpdateState(DashboardState newState)
    {
        lock (_lock)
        {
            _currentState = newState ?? throw new ArgumentNullException(nameof(newState));
        }
    }

    /// <summary>
    /// Background write loop that runs at 10 Hz (100ms intervals).
    /// </summary>
    private void WriteLoop()
    {
        var cancellationToken = _cancellationTokenSource!.Token;

        while (!cancellationToken.IsCancellationRequested)
        {
            try
            {
                OpenPipe();

                while (!cancellationToken.IsCancellationRequested)
                {
                    try
                    {
                        if (Paused)
                        {
                            Thread.Sleep(100);
                            continue;
                        }

                        DashboardState state;
                        lock (_lock)
                        {
                            state = _currentState!.Clone();
                        }

                        // Always send the live data message — the display reads
                        // choice_state (field 13) from it to decide menu vs dashboard.
                        _writer!.WriteLine(MessageGenerator.GenerateLiveDataMessage(state));

                        // When in a menu, also send the menu data message with field values.
                        if (state.MenuState != 0)
                        {
                            _writer!.WriteLine(GenerateDefaultMenuMessage(state));
                        }

                        _writer!.Flush();

                        Thread.Sleep(100);
                    }
                    catch (IOException)
                    {
                        break;
                    }
                    catch (Exception ex)
                    {
                        Console.Error.WriteLine($"PipeWriter error: {ex.GetType().Name}: {ex.Message}");
                        break;
                    }
                }

                _writer?.Dispose();
                _writer = null;
                _pipeStream?.Dispose();
                _pipeStream = null;

                if (!cancellationToken.IsCancellationRequested)
                {
                    Console.WriteLine("Connection lost. Reconnecting...");
                    CreateFifo();
                }
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"PipeWriter error: {ex.Message}");
                if (!cancellationToken.IsCancellationRequested)
                {
                    Thread.Sleep(1000);
                }
            }
        }
    }

    /// <summary>
    /// Generates a default menu data message using current state values.
    /// </summary>
    private string GenerateDefaultMenuMessage(DashboardState state)
    {
        var odoDigits = new int[6];
        var odo2Digits = new int[6];
        var setTimeDigits = new int[] { state.Hour / 10, state.Hour % 10, state.Minute / 10, state.Minute % 10 };
        var spcDigits = new int[4];

        return MessageGenerator.GenerateMenuDataMessage(
            menuState: state.MenuState,
            odoDigits: odoDigits,
            odo2Digits: odo2Digits,
            odoError: state.OdoError,
            setTimeDigits: setTimeDigits,
            spcDigits: spcDigits,
            frontSprocket: state.FrontSprocket,
            rearSprocket: state.RearSprocket,
            coolantFanTemp: state.CoolantFanTemp,
            km: state.Km,
            fh: state.Fahrenheit,
            bar: state.Bar,
            frontSensor: state.FrontSensor,
            rearSensor: state.RearSensor,
            frontPressureLow: state.FrontPressureLow,
            rearPressureLow: state.RearPressureLow,
            controlLayout: state.ControlLayout,
            dayTheme: state.DayTheme,
            nightTheme: state.NightTheme,
            currentLightLevel: state.CurrentLightLevel,
            lightSwitchValue: state.LightSwitchValue,
            fanNeutralOption: state.FanNeutralOption,
            gearRatioInterval: state.GearRatioInterval
        );
    }

    /// <summary>
    /// Stops the background writer thread and cleans up resources.
    /// </summary>
    public void Stop()
    {
        lock (_lock)
        {
            if (!_isRunning)
            {
                return;
            }

            _isRunning = false;
        }

        _cancellationTokenSource?.Cancel();
        _writeThread?.Join(TimeSpan.FromSeconds(2));

        _writer?.Dispose();
        _writer = null;
        _pipeStream?.Dispose();
        _pipeStream = null;

        if (File.Exists(_pipePath))
        {
            try
            {
                File.Delete(_pipePath);
            }
            catch
            {
                // Best effort cleanup
            }
        }

        _cancellationTokenSource?.Dispose();
        _cancellationTokenSource = null;
    }

    /// <summary>
    /// Disposes the pipe writer and associated resources.
    /// </summary>
    public void Dispose()
    {
        Stop();
        GC.SuppressFinalize(this);
    }

    ~PipeWriter()
    {
        Dispose();
    }
}
