using Xunit;
using TftDashSimulator.Core;

namespace TftDashSimulator.Tests;

/// <summary>
/// Comprehensive unit tests for MessageGenerator.
/// Verifies compliance with TFT Dash serial protocol specification (SERIAL-PROTOCOL.md).
/// </summary>
public class MessageGeneratorTests
{
    [Fact]
    public void GenerateLiveDataMessage_StartsWithOpenBrace()
    {
        // Arrange
        var state = new DashboardState();

        // Act
        var message = MessageGenerator.GenerateLiveDataMessage(state);

        // Assert
        Assert.StartsWith("{", message);
    }

    [Fact]
    public void GenerateLiveDataMessage_EndsWithCloseBrace()
    {
        // Arrange
        var state = new DashboardState();

        // Act
        var message = MessageGenerator.GenerateLiveDataMessage(state);

        // Assert
        Assert.EndsWith("}", message);
    }

    [Fact]
    public void GenerateLiveDataMessage_HasCorrectFieldCount()
    {
        // Arrange
        var state = new DashboardState();

        // Act
        var message = MessageGenerator.GenerateLiveDataMessage(state);

        // Assert - Should have 39 elements when split: "" + 37 data fields + ""
        // Format: {,field1,field2,...,field37,}
        // Hour and minute are separate fields (fields 5 and 6)
        var trimmed = message.Trim('{', '}');
        var fields = trimmed.Split(',');

        Assert.Equal(39, fields.Length);
        Assert.Equal("", fields[0]); // First field is empty (leading comma)
    }

    [Fact]
    public void GenerateLiveDataMessage_IsWithinValidLengthRange()
    {
        // Arrange
        var state = new DashboardState();

        // Act
        var message = MessageGenerator.GenerateLiveDataMessage(state);

        // Assert - Valid messages are between 90 and 400 characters
        Assert.InRange(message.Length, 90, 400);
    }

    [Fact]
    public void GenerateLiveDataMessage_ValidatesCorrectly()
    {
        // Arrange
        var state = new DashboardState();

        // Act
        var message = MessageGenerator.GenerateLiveDataMessage(state);

        // Assert
        Assert.True(MessageGenerator.IsValidLiveDataMessage(message));
    }

    [Fact]
    public void GenerateLiveDataMessage_FormatsFloatsWithOneDecimalPlace()
    {
        // Arrange
        var state = new DashboardState
        {
            Battery = 12.567f,  // Should become 12.6
            Trip1 = 123.456f,   // Should become 123.5
            Trip2 = 78.912f,    // Should become 78.9
            Odo = 12345.678f,   // Should become 12345.7
            SpeedCorrection = 1.23f  // Should become 1.2
        };

        // Act
        var message = MessageGenerator.GenerateLiveDataMessage(state);

        // Assert
        Assert.Contains(",12.6,", message);  // Battery
        Assert.Contains(",123.5,", message); // Trip1
        Assert.Contains(",78.9,", message);  // Trip2
        Assert.Contains(",12345.7,", message); // Odo
        Assert.Contains(",1.2,", message);   // SpeedCorrection
    }

    [Fact]
    public void GenerateLiveDataMessage_UsesInvariantCulture()
    {
        // Arrange
        var state = new DashboardState
        {
            Battery = 12.5f
        };

        // Act
        var message = MessageGenerator.GenerateLiveDataMessage(state);

        // Assert - Should use period as decimal separator, not comma
        Assert.Contains("12.5", message);
        Assert.DoesNotContain("12,5", message);
    }

    [Fact]
    public void GenerateLiveDataMessage_IncludesAllFieldsInCorrectOrder()
    {
        // Arrange
        var state = new DashboardState
        {
            Speed = 78,
            Rpm = 5200,
            Coolant = 89,
            Battery = 12.5f,
            Hour = 14,
            Minute = 35,
            Fuel = 477,
            Neutral = 0,
            Oil = 1,
            HighBeam = 0,
            Left = 0,
            Right = 0,
            MenuState = 0,
            InfoMode = 0,
            Trip1 = 12.5f,
            Trip2 = 24.8f,
            Odo = 1234.5f,
            Km = 0,
            SpeedCorrection = 1.0f,
            Theme = 3,
            Ambient = 22,
            Gear = 4,
            Mpg = 45,
            Range = 120,
            MaxSpeed = 78,
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
            Nav = ""
        };

        // Act
        var message = MessageGenerator.GenerateLiveDataMessage(state);

        // Assert - Hour and minute are separate integer fields (matching parser.c)
        var expected = "{,78,5200,89,12.5,14,35,477,0,1,0,0,0,0,0,12.5,24.8,1234.5,0,1.0,3,22,4,45,120,78,1,35,0,0,0,0,0,1,2,30,28,,}";
        Assert.Equal(expected, message);
    }

    [Fact]
    public void GenerateLiveDataMessage_HandlesNavigationMessage()
    {
        // Arrange
        var state = new DashboardState
        {
            Nav = "%TNR%A1234%Edinburgh%Exit 5%500%0.3%1%12.5%"
        };

        // Act
        var message = MessageGenerator.GenerateLiveDataMessage(state);

        // Assert - Navigation should be field 37, before the closing }
        Assert.EndsWith(",%TNR%A1234%Edinburgh%Exit 5%500%0.3%1%12.5%,}", message);
    }

    [Fact]
    public void GenerateLiveDataMessage_HandlesEmptyNavigation()
    {
        // Arrange
        var state = new DashboardState
        {
            Nav = ""
        };

        // Act
        var message = MessageGenerator.GenerateLiveDataMessage(state);

        // Assert - Should have empty field before closing }
        Assert.EndsWith(",,}", message);
    }

    [Fact]
    public void GenerateLiveDataMessage_HandlesMaximumValues()
    {
        // Arrange
        var state = new DashboardState
        {
            Speed = uint.MaxValue,
            Rpm = uint.MaxValue,
            MaxSpeed = uint.MaxValue,
            Coolant = int.MaxValue,
            Battery = 99.9f,
            Trip1 = 99999.9f,
            Trip2 = 99999.9f,
            Odo = 999999.9f
        };

        // Act
        var message = MessageGenerator.GenerateLiveDataMessage(state);

        // Assert - Should generate valid message even with extreme values
        Assert.True(MessageGenerator.IsValidLiveDataMessage(message));
    }

    [Fact]
    public void GenerateMenuDataMessage_StartsWithOpenBracket()
    {
        // Arrange
        var odoDigits = new int[] { 1, 2, 3, 4, 5, 6 };
        var odo2Digits = new int[] { 1, 2, 3, 4, 5, 6 };
        var setTimeDigits = new int[] { 1, 4, 3, 5 };
        var spcDigits = new int[] { 1, 0, 0, 0 };

        // Act
        var message = MessageGenerator.GenerateMenuDataMessage(
            menuState: 4,
            odoDigits: odoDigits,
            odo2Digits: odo2Digits,
            odoError: 0,
            setTimeDigits: setTimeDigits,
            spcDigits: spcDigits,
            frontSprocket: 15,
            rearSprocket: 45,
            coolantFanTemp: 95,
            km: 0,
            fh: 0,
            bar: 0,
            frontSensor: 1,
            rearSensor: 2,
            frontPressureLow: 30,
            rearPressureLow: 28,
            controlLayout: 1,
            dayTheme: 0,
            nightTheme: 7,
            currentLightLevel: 500,
            lightSwitchValue: 300,
            fanNeutralOption: 0,
            gearRatioInterval: 100
        );

        // Assert
        Assert.StartsWith("[", message);
    }

    [Fact]
    public void GenerateMenuDataMessage_EndsWithCloseBracket()
    {
        // Arrange
        var odoDigits = new int[] { 0, 0, 0, 0, 0, 0 };
        var odo2Digits = new int[] { 0, 0, 0, 0, 0, 0 };
        var setTimeDigits = new int[] { 0, 0, 0, 0 };
        var spcDigits = new int[] { 0, 0, 0, 0 };

        // Act
        var message = MessageGenerator.GenerateMenuDataMessage(
            menuState: 0,
            odoDigits: odoDigits,
            odo2Digits: odo2Digits,
            odoError: 0,
            setTimeDigits: setTimeDigits,
            spcDigits: spcDigits,
            frontSprocket: 15,
            rearSprocket: 45,
            coolantFanTemp: 95,
            km: 0,
            fh: 0,
            bar: 0,
            frontSensor: 1,
            rearSensor: 2,
            frontPressureLow: 30,
            rearPressureLow: 28,
            controlLayout: 1,
            dayTheme: 0,
            nightTheme: 7,
            currentLightLevel: 500,
            lightSwitchValue: 300,
            fanNeutralOption: 0,
            gearRatioInterval: 100
        );

        // Assert
        Assert.EndsWith("]", message);
    }

    [Fact]
    public void GenerateMenuDataMessage_HasCorrectFieldCount()
    {
        // Arrange
        var odoDigits = new int[] { 0, 0, 0, 0, 0, 0 };
        var odo2Digits = new int[] { 0, 0, 0, 0, 0, 0 };
        var setTimeDigits = new int[] { 0, 0, 0, 0 };
        var spcDigits = new int[] { 0, 0, 0, 0 };

        // Act
        var message = MessageGenerator.GenerateMenuDataMessage(
            menuState: 0,
            odoDigits: odoDigits,
            odo2Digits: odo2Digits,
            odoError: 0,
            setTimeDigits: setTimeDigits,
            spcDigits: spcDigits,
            frontSprocket: 15,
            rearSprocket: 45,
            coolantFanTemp: 95,
            km: 0,
            fh: 0,
            bar: 0,
            frontSensor: 1,
            rearSensor: 2,
            frontPressureLow: 30,
            rearPressureLow: 28,
            controlLayout: 1,
            dayTheme: 0,
            nightTheme: 7,
            currentLightLevel: 500,
            lightSwitchValue: 300,
            fanNeutralOption: 0,
            gearRatioInterval: 100
        );

        // Assert - Should have 41 elements when split: "" + 39 data fields + ""
        // Format: [,field1,field2,...,field39,]
        var trimmed = message.Trim('[', ']');
        var fields = trimmed.Split(',');

        Assert.Equal(41, fields.Length);
        Assert.Equal("", fields[0]); // First field is empty (leading comma)
    }

    [Fact]
    public void GenerateMenuDataMessage_IsWithinValidLengthRange()
    {
        // Arrange
        var odoDigits = new int[] { 1, 2, 3, 4, 5, 6 };
        var odo2Digits = new int[] { 1, 2, 3, 4, 5, 6 };
        var setTimeDigits = new int[] { 1, 4, 3, 5 };
        var spcDigits = new int[] { 1, 0, 5, 0 };

        // Act
        var message = MessageGenerator.GenerateMenuDataMessage(
            menuState: 401,
            odoDigits: odoDigits,
            odo2Digits: odo2Digits,
            odoError: 0,
            setTimeDigits: setTimeDigits,
            spcDigits: spcDigits,
            frontSprocket: 15,
            rearSprocket: 45,
            coolantFanTemp: 95,
            km: 1,
            fh: 0,
            bar: 0,
            frontSensor: 1,
            rearSensor: 2,
            frontPressureLow: 30,
            rearPressureLow: 28,
            controlLayout: 1,
            dayTheme: 0,
            nightTheme: 7,
            currentLightLevel: 500,
            lightSwitchValue: 300,
            fanNeutralOption: 0,
            gearRatioInterval: 100
        );

        // Assert - Valid messages are between 79 and 149 characters
        Assert.InRange(message.Length, 79, 149);
    }

    [Fact]
    public void GenerateMenuDataMessage_ValidatesCorrectly()
    {
        // Arrange
        var odoDigits = new int[] { 0, 0, 0, 0, 0, 0 };
        var odo2Digits = new int[] { 0, 0, 0, 0, 0, 0 };
        var setTimeDigits = new int[] { 0, 0, 0, 0 };
        var spcDigits = new int[] { 0, 0, 0, 0 };

        // Act
        var message = MessageGenerator.GenerateMenuDataMessage(
            menuState: 0,
            odoDigits: odoDigits,
            odo2Digits: odo2Digits,
            odoError: 0,
            setTimeDigits: setTimeDigits,
            spcDigits: spcDigits,
            frontSprocket: 15,
            rearSprocket: 45,
            coolantFanTemp: 95,
            km: 0,
            fh: 0,
            bar: 0,
            frontSensor: 1,
            rearSensor: 2,
            frontPressureLow: 30,
            rearPressureLow: 28,
            controlLayout: 1,
            dayTheme: 0,
            nightTheme: 7,
            currentLightLevel: 500,
            lightSwitchValue: 300,
            fanNeutralOption: 0,
            gearRatioInterval: 100
        );

        // Assert
        Assert.True(MessageGenerator.IsValidMenuDataMessage(message));
    }

    [Fact]
    public void GenerateMenuDataMessage_IncludesAllFieldsInCorrectOrder()
    {
        // Arrange
        var odoDigits = new int[] { 1, 2, 3, 4, 5, 6 };
        var odo2Digits = new int[] { 1, 2, 3, 4, 5, 6 };
        var setTimeDigits = new int[] { 1, 4, 3, 5 };
        var spcDigits = new int[] { 1, 0, 5, 0 };

        // Act
        var message = MessageGenerator.GenerateMenuDataMessage(
            menuState: 401,
            odoDigits: odoDigits,
            odo2Digits: odo2Digits,
            odoError: 0,
            setTimeDigits: setTimeDigits,
            spcDigits: spcDigits,
            frontSprocket: 15,
            rearSprocket: 45,
            coolantFanTemp: 95,
            km: 1,
            fh: 0,
            bar: 0,
            frontSensor: 1,
            rearSensor: 2,
            frontPressureLow: 30,
            rearPressureLow: 28,
            controlLayout: 1,
            dayTheme: 0,
            nightTheme: 7,
            currentLightLevel: 500,
            lightSwitchValue: 300,
            fanNeutralOption: 0,
            gearRatioInterval: 100
        );

        // Assert
        var expected = "[,401,1,2,3,4,5,6,1,2,3,4,5,6,0,1,4,3,5,1,0,5,0,15,45,95,1,0,0,1,2,30,28,1,0,7,500,300,0,100,]";
        Assert.Equal(expected, message);
    }

    [Fact]
    public void GenerateMenuDataMessage_HandlesMenuStateConstants()
    {
        // Arrange - Test various menu states from SERIAL-PROTOCOL.md
        var odoDigits = new int[] { 0, 0, 0, 0, 0, 0 };
        var odo2Digits = new int[] { 0, 0, 0, 0, 0, 0 };
        var setTimeDigits = new int[] { 0, 0, 0, 0 };
        var spcDigits = new int[] { 0, 0, 0, 0 };

        // Act & Assert - MAINDASHBOARD = 0
        var message0 = MessageGenerator.GenerateMenuDataMessage(
            menuState: 0,
            odoDigits, odo2Digits, 0, setTimeDigits, spcDigits,
            15, 45, 95, 0, 0, 0, 1, 2, 30, 28, 1, 0, 7, 500, 300, 0, 100
        );
        Assert.Contains("[,0,", message0);

        // Act & Assert - SETUNITSMILES = 401
        var message401 = MessageGenerator.GenerateMenuDataMessage(
            menuState: 401,
            odoDigits, odo2Digits, 0, setTimeDigits, spcDigits,
            15, 45, 95, 0, 0, 0, 1, 2, 30, 28, 1, 0, 7, 500, 300, 0, 100
        );
        Assert.Contains("[,401,", message401);

        // Act & Assert - ODOSETUP = 12
        var message12 = MessageGenerator.GenerateMenuDataMessage(
            menuState: 12,
            odoDigits, odo2Digits, 0, setTimeDigits, spcDigits,
            15, 45, 95, 0, 0, 0, 1, 2, 30, 28, 1, 0, 7, 500, 300, 0, 100
        );
        Assert.Contains("[,12,", message12);
    }

    [Fact]
    public void IsValidLiveDataMessage_ReturnsTrueForValidMessage()
    {
        // Arrange
        var validMessage = "{,78,5200,89,12.5,14,35,477,0,1,0,0,0,0,0,12.5,24.8,1234.5,0,1.0,3,22,4,45,120,78,1,35,0,0,0,0,0,1,2,30,28,,}";

        // Act
        var result = MessageGenerator.IsValidLiveDataMessage(validMessage);

        // Assert
        Assert.True(result);
    }

    [Fact]
    public void IsValidLiveDataMessage_ReturnsFalseForTooShort()
    {
        // Arrange
        var shortMessage = "{,0,0,0,}";

        // Act
        var result = MessageGenerator.IsValidLiveDataMessage(shortMessage);

        // Assert
        Assert.False(result);
    }

    [Fact]
    public void IsValidLiveDataMessage_ReturnsFalseForTooLong()
    {
        // Arrange
        var longMessage = "{," + new string('x', 500) + ",}";

        // Act
        var result = MessageGenerator.IsValidLiveDataMessage(longMessage);

        // Assert
        Assert.False(result);
    }

    [Fact]
    public void IsValidLiveDataMessage_ReturnsFalseForWrongStartMarker()
    {
        // Arrange
        var wrongStart = "[,78,5200,89,12.5,14:35,477,0,1,0,0,0,0,0,12.5,24.8,1234.5,0,1.0,3,22,4,45,120,78,1,35,0,0,0,0,0,1,2,30,28,,}";

        // Act
        var result = MessageGenerator.IsValidLiveDataMessage(wrongStart);

        // Assert
        Assert.False(result);
    }

    [Fact]
    public void IsValidLiveDataMessage_ReturnsFalseForWrongEndMarker()
    {
        // Arrange
        var wrongEnd = "{,78,5200,89,12.5,14:35,477,0,1,0,0,0,0,0,12.5,24.8,1234.5,0,1.0,3,22,4,45,120,78,1,35,0,0,0,0,0,1,2,30,28,,]";

        // Act
        var result = MessageGenerator.IsValidLiveDataMessage(wrongEnd);

        // Assert
        Assert.False(result);
    }

    [Fact]
    public void IsValidMenuDataMessage_ReturnsTrueForValidMessage()
    {
        // Arrange
        var validMessage = "[,401,1,2,3,4,5,6,1,2,3,4,5,6,0,1,4,3,5,1,0,5,0,15,45,95,1,0,0,1,2,30,28,1,0,7,500,300,0,100,]";

        // Act
        var result = MessageGenerator.IsValidMenuDataMessage(validMessage);

        // Assert
        Assert.True(result);
    }

    [Fact]
    public void IsValidMenuDataMessage_ReturnsFalseForTooShort()
    {
        // Arrange
        var shortMessage = "[,0,0,0,]";

        // Act
        var result = MessageGenerator.IsValidMenuDataMessage(shortMessage);

        // Assert
        Assert.False(result);
    }

    [Fact]
    public void IsValidMenuDataMessage_ReturnsFalseForTooLong()
    {
        // Arrange
        var longMessage = "[," + new string('x', 150) + ",]";

        // Act
        var result = MessageGenerator.IsValidMenuDataMessage(longMessage);

        // Assert
        Assert.False(result);
    }

    [Fact]
    public void IsValidMenuDataMessage_ReturnsFalseForWrongStartMarker()
    {
        // Arrange
        var wrongStart = "{,401,1,2,3,4,5,6,1,2,3,4,5,6,0,1,4,3,5,1,0,5,0,15,45,95,1,0,0,1,2,30,28,1,0,7,500,300,0,100,]";

        // Act
        var result = MessageGenerator.IsValidMenuDataMessage(wrongStart);

        // Assert
        Assert.False(result);
    }

    [Fact]
    public void IsValidMenuDataMessage_ReturnsFalseForWrongEndMarker()
    {
        // Arrange
        var wrongEnd = "[,401,1,2,3,4,5,6,1,2,3,4,5,6,0,1,4,3,5,1,0,5,0,15,45,95,1,0,0,1,2,30,28,1,0,7,500,300,0,100,}";

        // Act
        var result = MessageGenerator.IsValidMenuDataMessage(wrongEnd);

        // Assert
        Assert.False(result);
    }

    [Fact]
    public void GenerateLiveDataMessage_HandlesZeroValues()
    {
        // Arrange
        var state = new DashboardState
        {
            Speed = 0,
            Rpm = 0,
            Coolant = 0,
            Battery = 0.0f,
            Trip1 = 0.0f,
            Trip2 = 0.0f,
            Odo = 0.0f,
            SpeedCorrection = 0.0f
        };

        // Act
        var message = MessageGenerator.GenerateLiveDataMessage(state);

        // Assert
        Assert.True(MessageGenerator.IsValidLiveDataMessage(message));
        Assert.Contains(",0,", message);
        Assert.Contains(",0.0,", message);
    }

    [Fact]
    public void GenerateLiveDataMessage_DefaultStateProducesValidMessage()
    {
        // Arrange
        var state = new DashboardState(); // Uses constructor defaults

        // Act
        var message = MessageGenerator.GenerateLiveDataMessage(state);

        // Assert
        Assert.True(MessageGenerator.IsValidLiveDataMessage(message));
        Assert.StartsWith("{,", message);
        Assert.EndsWith(",}", message);
    }
}
