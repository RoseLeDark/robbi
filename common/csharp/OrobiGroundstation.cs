using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO.Ports;

public struct OrobiCommandStatus
{
    public byte SeqNr;
    public OrobiCommandStatusType Status;
    public DateTime Timestamp;
}

public enum OrobiCommandStatusType
{
    Ok,
    Error,
    NoData,
    InvalidCommand
}

public class OrobiGroundstation
{
    public struct Uint128
    {
        public ulong Low;
        public ulong High;
    }

    public struct OrobiRobot
    {
        public Uint128 Id;
        public byte[] PublicKey; // crypto_box_PUBLICKEYBYTES
        public string Name;
    }

    private List<OrobiRobot> robots;
    private byte[] groundPublicKey;
    private byte[] groundSecretKey;
    private SerialPort serialPort;
    private string name;

    public OrobiGroundstation(byte[] groundPublicKey, byte[] groundSecretKey, string serialPortName, string name)
    {
        robots = new List<OrobiRobot>();
        this.groundPublicKey = groundPublicKey;
        this.groundSecretKey = groundSecretKey;
        this.name = name;
        this.serialPort = new SerialPort(serialPortName, 115200, Parity.None, 8, StopBits.One);
        this.serialPort.Open();
    }

    public void AddRobot(Uint128 id, byte[] publicKey, string name)
    {
        OrobiRobot newRobot = new OrobiRobot
        {
            Id = id,
            PublicKey = publicKey,
            Name = name
        };
        robots.Add(newRobot);
    }

    public string CreateCommandString(ushort compass, byte motor, ushort durationMs)
    {
        return $"A{compass:000}{motor:00}{durationMs:0000}#";
    }

    public List<OrobiCommandStatus> ProcessResponse(string responseStr)
    {
        List<OrobiCommandStatus> statuses = new List<OrobiCommandStatus>();
        string[] statusStrings = responseStr.Split(new[] { '#' }, StringSplitOptions.RemoveEmptyEntries);

        foreach (var statusStr in statusStrings)
        {
            if (statusStr.StartsWith("S"))
            {
                string[] parts = statusStr.Substring(1).Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                if (parts.Length == 3)
                {
                    if (byte.TryParse(parts[0], NumberStyles.Integer, CultureInfo.InvariantCulture, out byte seqNr) &&
                        Enum.TryParse(parts[1], out OrobiCommandStatusType status) &&
                        long.TryParse(parts[2], NumberStyles.Integer, CultureInfo.InvariantCulture, out long timestamp))
                    {
                        statuses.Add(new OrobiCommandStatus
                        {
                            SeqNr = seqNr,
                            Status = status,
                            Timestamp = DateTimeOffset.FromUnixTimeSeconds(timestamp).DateTime
                        });
                    }
                }
            }
        }

        return statuses;
    }

    public void SendPcPublicKey(byte[] pcPublicKey)
    {
        string pcPublicKeyHex = BitConverter.ToString(pcPublicKey).Replace("-", "").ToLowerInvariant();
        string jsonStr = $"{{\"orob_ground_key\":\"{pcPublicKeyHex}\", \"Name\": \"{name}\"}}";
        serialPort.WriteLine(jsonStr);
    }

    public void Clear()
    {
        robots.Clear();
    }
}
