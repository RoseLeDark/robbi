using System;
using System.Runtime.InteropServices;

public static class OrobiSecure
{
    private const string DllName = "libopenrobi.so"; // Name deiner kompilierte Bibliothek

    [StructLayout(LayoutKind.Sequential)]
    public struct OrobiSecureNonce
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 24)] // crypto_box_NONCEBYTES
        public byte[] bytes;
        public uint counter;
        public long timestamp; // time_t is generally long in Unix systems
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct OrobiPacket
    {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4096)] // OROBI_MAXMESSAGESIZE
        public string message;
        public ushort message_size;
        public ulong api_key;
        public ulong packet_hash;
        public long timestamp;
        public OrobiSecureNonce nonce;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct OrobiCryptPacket
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4112)] // sizeof(packet_t) + crypto_box_ZEROBYTES
        public byte[] encrypted_data;
        public ulong crypt_hash;
        public OrobiSecureNonce nonce;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Uint128
    {
        public ulong high;
        public ulong low;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct OrobiSecureContext
    {
        public Uint128 id;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)] // crypto_box_PUBLICKEYBYTES
        public byte[] public_key;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)] // crypto_box_SECRETKEYBYTES
        public byte[] secret_key;
        public OrobiSecureNonce last_seen_nonce;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)] // OROBI_ERROR_BUFFER_SIZE
        public string last_error;
        public int last_status;
    }

    [DllImport(DllName)]
    public static extern ulong orobi_murmur3_64(IntPtr data, ulong len, ulong seed);

    [DllImport(DllName)]
    public static extern void orobi_secure_init(ref OrobiSecureContext ctx, Uint128 id, byte[] public_key, byte[] secret_key);

    [DllImport(DllName)]
    public static extern int orobi_secure_close(ref OrobiSecureContext ctx);

    [DllImport(DllName)]
    public static extern int orobi_create_packet(ref OrobiSecureContext ctx, ref OrobiPacket packet, string message, ushort size);

    [DllImport(DllName)]
    public static extern int orobi_encrypt_packet(ref OrobiSecureContext ctx, ref OrobiPacket packet, ref OrobiCryptPacket crypt_packet, byte[] their_public_key);

    [DllImport(DllName)]
    public static extern int orobi_decrypt_packet(ref OrobiSecureContext ctx, ref OrobiCryptPacket crypt_packet, ref OrobiPacket packet, byte[] their_public_key);
}
