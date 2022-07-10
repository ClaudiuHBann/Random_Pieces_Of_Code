using System.Runtime.CompilerServices;
using System.Security.Cryptography;
using System.Xml.Serialization;
using System.Text;

namespace E2EE
{
    class RSA
    {
        public RSACryptoServiceProvider? RSACSP { get; set; }
        public RSAParameters? RSAPrivateKey { get; set; }
        public RSAParameters? RSAPublicKey { get; set; }
        public Encoding DefaultEncoding { get; set; }

        public RSA(int keySize = 4096, Encoding? encoding = null)
        {
            DefaultEncoding = encoding ?? Encoding.Unicode;

            try
            {
                RSACSP = new(keySize);

                RSAPrivateKey = RSACSP.ExportParameters(true);
                RSAPublicKey = RSACSP.ExportParameters(false);
            }
            catch (CryptographicException ce)
            {
                Console.WriteLine(ce.Message);
            }
        }

        public byte[]? Encrypt(byte[] data, bool doOAEPPadding = true)
        {
            try
            {
                if (RSACSP != null && RSAPublicKey != null)
                {
                    RSACSP.ImportParameters((RSAParameters)RSAPublicKey);
                    return RSACSP.Encrypt(data, doOAEPPadding);
                }
            }
            catch (CryptographicException ce)
            {
                Console.WriteLine(ce.ToString());
            }

            return null;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public byte[]? EncryptAsString(string data, bool doOAEPPadding = true)
        {
            return Encrypt(DefaultEncoding.GetBytes(data), doOAEPPadding);
        }

        public byte[]? Decrypt(byte[] data, bool doOAEPPadding = true)
        {
            try
            {
                if (RSACSP != null && RSAPrivateKey != null)
                {
                    RSACSP.ImportParameters((RSAParameters)RSAPrivateKey);
                    return RSACSP.Decrypt(data, doOAEPPadding);
                }
            }
            catch (CryptographicException ce)
            {
                Console.WriteLine(ce.ToString());
            }

            return null;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public string DecryptAsString(byte[] data, bool doOAEPPadding = true)
        {
            var dataAsBytes = Decrypt(data, doOAEPPadding);
            if (dataAsBytes == null)
            {
                return "";
            }

            return DefaultEncoding.GetString(dataAsBytes);
        }

        private string GetKey(bool publicKey = true)
        {
            StringWriter stringWriter = new();
            XmlSerializer xmlSerializer = new(typeof(RSAParameters));

            xmlSerializer.Serialize(stringWriter, (publicKey ? RSAPublicKey : RSAPrivateKey));

            return stringWriter.ToString();
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public string GetKeyPublicAsString()
        {
            return GetKey();
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public string GetKeyPrivateAsString()
        {
            return GetKey(false);
        }

        public static RSAParameters? StringToKeys(string str)
        {
            StringReader stringReader = new(str);
            XmlSerializer xmlSerializer = new(typeof(RSAParameters));

            try
            {
                return (RSAParameters?)xmlSerializer.Deserialize(stringReader);
            }
            catch (InvalidOperationException ioe)
            {
                Console.WriteLine(ioe.Message);
            }

            return null;
        }
    }
}
