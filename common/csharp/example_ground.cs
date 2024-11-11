public class Program
{
    public static void Main(string[] args)
    {
        byte[] ground_publicKey = new byte[32]; // Fügen Sie hier den tatsächlichen Public Key ein
        byte[] ground_secretKey = new byte[32]; // Fügen Sie hier den tatsächlichen Secret Key ein
        OrobiGroundstation groundstation = new OrobiGroundstation(ground_publicKey, ground_secretKey);

        // Beispielhafte IDs und Schlüssel
        OrobiGroundstation.Uint128 robotId = new OrobiGroundstation.Uint128 { Low = 0x12345678, High = 0x9abcdef0 };
        byte[] esp32_publicKey = new byte[32]; // Fügen Sie hier den tatsächlichen Public Key ein

        // Roboter hinzufügen
        groundstation.AddRobot(robotId, esp32_publicKey);

        // Command-String erstellen
        string commandStr = groundstation.CreateCommandString(360, 99, 1000);
        Console.WriteLine($"Command String: {commandStr}");

        // Beispielantwort verarbeiten
        string responseStr = "S0010001636368000#S0020011636668000#S0030021636668001#";
        List<OrobiCommandStatus> statuses = groundstation.ProcessResponse(responseStr);

        foreach (var status in statuses)
        {
            Console.WriteLine($"SeqNr: {status.SeqNr}, Status: {status.Status}, Timestamp: {status.Timestamp}");
        }
    }
}
