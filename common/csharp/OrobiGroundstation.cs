using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public class OrobiGroundstation
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Uint128
    {
        public ulong Low;
        public ulong High;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct OrobiRobot
    {
        public Uint128 Id;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
        public byte[] PublicKey; // crypto_box_PUBLICKEYBYTES
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
        public byte[] SecretKey; // crypto_box_SECRETKEYBYTES
    }

    private List<OrobiRobot> robots;

    public OrobiGroundstation()
    {
        robots = new List<OrobiRobot>();
    }

    public void AddRobot(Uint128 id, byte[] publicKey, byte[] secretKey)
    {
        OrobiRobot newRobot = new OrobiRobot
        {
            Id = id,
            PublicKey = publicKey,
            SecretKey = secretKey
        };
        robots.Add(newRobot);
    }

    public string CreateCommandString(ushort compass, byte motor, ushort durationMs)
    {
        return $"A{compass:000}{motor:00}{durationMs:0000}#";
    }

    public OrobiCommandStatus ProcessResponse(string responseStr)
    {
        // Implementiere die Logik zum Verarbeiten der Antwort und zum Aktualisieren des Status
        return new OrobiCommandStatus();
    }

    public void Clear()
    {
        robots.Clear();
    }
}

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
