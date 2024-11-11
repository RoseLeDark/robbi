public class Program
{
    public static void Main(string[] args)
    {
        byte[] groundPublicKey = new byte[32]; // Fügen Sie hier den tatsächlichen Public Key ein
        byte[] groundSecretKey = new byte[32]; // Fügen Sie hier den tatsächlichen Secret Key ein
        

        // Initialisiere die Groundstation mit dem Namen des seriellen Ports für die Kommunikation mit dem ESP32
        OrobiGroundstation groundstation = new OrobiGroundstation(groundPublicKey, groundSecretKey, "COM3", "GroundStation");

        // Beispielhafte IDs und Schlüssel
        OrobiGroundstation.Uint128 robotId = new OrobiGroundstation.Uint128 { Low = 0x12345678, High = 0x9abcdef0 };
        byte[] esp32PublicKey = new byte[32]; // Fügen Sie hier den tatsächlichen Public Key ein

        // Roboter hinzufügen
        groundstation.AddRobot(robotId, esp32PublicKey, "Robot1");

        // Command-String erstellen
        string commandStr = groundstation.CreateCommandString(360, 99, 1000);
        Console.WriteLine($"Command String: {commandStr}");

        // Sende den PC Public Key an den ESP32
        groundstation.SendPcPublicKey(groundPublicKey);
        Console.WriteLine("Ground Public Key sent to ESP32.");

        // Beispielantwort verarbeiten
        string responseStr = "S0010001636368000#S0020011636668000#S0030021636668001#";
        List<OrobiCommandStatus> statuses = groundstation.ProcessResponse(responseStr);

        foreach (var status in statuses)
        {
            Console.WriteLine($"SeqNr: {status.SeqNr}, Status: {status.Status}, Timestamp: {status.Timestamp}");
        }
    }
}
